// DRFTWin32.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#ifdef __XENO__
#include <rtdk.h>
#include <native/task.h>
#include <native/timer.h>
#else
#include "stdafx.h"
#include <Windows.h>
#include <process.h>
#include <conio.h>
#endif // __XENO__
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <ctime>
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;
#include "drfl/DRFLEx.h"
using namespace DRAFramework;

#undef NDEBUG
#include <assert.h>

CDRFLEx Drfl;
bool g_bHasControlAuthority = FALSE;
bool g_TpInitailizingComplted = FALSE;
bool g_mStat = FALSE;
bool g_Stop = FALSE;
bool moving = FALSE;
string strDrl =
    "\r\n\
loop = 0\r\n\
while loop < 1003:\r\n\
 movej(posj(10,10.10,10,10.10), vel=60, acc=60)\r\n\
 movej(posj(00,00.00,00,00.00), vel=60, acc=60)\r\n\
 loop+=1\r\n";

bool bAlterFlag = FALSE;

int linux_kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;

	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );


	ch = getchar();

	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return ch;
}

int getch()
{
    int c;
    struct termios oldattr, newattr;

    tcgetattr(STDIN_FILENO, &oldattr);           // ���� �͹̳� ���� ����
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);         // CANONICAL�� ECHO ��
    newattr.c_cc[VMIN] = 1;                      // �ּ� �Է� ���� ���� 1�� ����
    newattr.c_cc[VTIME] = 0;                     // �ּ� �б� ��� �ð��� 0���� ����
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);  // �͹̳ο� ���� �Է�
    c = getchar();                               // Ű���� �Է� ����
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);  // ������ �������� ����
    return c;
}

void OnRTMonitoringData(LPRT_OUTPUT_DATA_LIST tData)
{
    printf("timestamp : %.3f\n", tData->time_stamp);
    printf("joint : %f %f %f %f %f %f\n", tData->actual_joint_position[0], tData->actual_joint_position[1], tData->actual_joint_position[2], tData->actual_joint_position[3], tData->actual_joint_position[4], tData->actual_joint_position[5]);
}


uint32_t ThreadFunc(void* arg) {
	printf("start ThreadFunc\n");

	while (true) {
		if(linux_kbhit()){
			char ch = getch();
			switch (ch) {
				case 's': {
					printf("Stop!\n");
					g_Stop = true;
					Drfl.MoveStop(STOP_TYPE_SLOW);
				} break;
				case 'p': {
					printf("Pause!\n");
					Drfl.MovePause();
				} break;
				case 'r': {
					printf("Resume!\n");
					Drfl.MoveResume();
				} break;
			}
		}

		//Sleep(100);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	std::cout << "exit ThreadFunc" << std::endl;

	return 0;
}

void OnDisConnected() {
  while (!Drfl.open_connection("192.168.137.100")) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

struct PlanParam
{
	float time;

	float ps[6];
	float vs[6];
	float as[6];
	float pf[6];
	float vf[6];
	float af[6];

	float A0[6];
	float A1[6];
	float A2[6];
	float A3[6];
	float A4[6];
	float A5[6];
};

struct TraParam
{
	float time;

	float pos[6];
	float vel[6];
	float acc[6];
};

void TrajectoryPlan(PlanParam* plan)
{
    float ps[6],vs[6],as[6];
    float pf[6],vf[6],af[6];
    float tf;

	tf = plan->time;

    for(int i=0; i<6; i++)
    {
        ps[i] = plan->ps[i];
        vs[i] = plan->vs[i];
        as[i] = plan->as[i];
        pf[i] = plan->pf[i];
        vf[i] = plan->vf[i];
        af[i] = plan->af[i];
    }

    for(int i=0; i<6; i++)
    {
        plan->A0[i] = ps[i];
        plan->A1[i] = vs[i];
        plan->A2[i] = as[i]/2;
        plan->A3[i] = (20*pf[i]-20*ps[i]-(8*vf[i]+12*vs[i])*tf-(3*as[i]-af[i])*tf*tf)/(2*tf*tf*tf);
        plan->A4[i] = (30*ps[i]-30*pf[i]+(14*vf[i]+16*vs[i])*tf+(3*as[i]-2*af[i])*tf*tf)/(2*tf*tf*tf*tf);
        plan->A5[i] = (12*pf[i]-12*ps[i]-(6*vf[i]+6*vs[i])*tf-(as[i]-af[i])*tf*tf)/(2*tf*tf*tf*tf*tf);
    }
}

void TrajectoryGenerator(PlanParam *plan, TraParam *tra)
{
    double A0[6],A1[6],A2[6],A3[6],A4[6],A5[6];
	double t = tra->time;

    for(int i=0; i<6; i++)
    {
        A0[i] = plan->A0[i];
        A1[i] = plan->A1[i];
        A2[i] = plan->A2[i];
        A3[i] = plan->A3[i];
        A4[i] = plan->A4[i];
        A5[i] = plan->A5[i];
    }

    for(int i=0; i<6; i++)
    {
        tra->pos[i] = A0[i] + A1[i]*t + A2[i]*t*t + A3[i]*t*t*t + A4[i]*t*t*t*t + A5[i]*t*t*t*t*t;
        tra->vel[i] = A1[i] + 2*A2[i]*t + 3*A3[i]*t*t + 4*A4[i]*t*t*t + 5*A5[i]*t*t*t*t;
        tra->acc[i] = 2*A2[i] + 6*A3[i]*t + 12*A4[i]*t*t + 20*A5[i]*t*t*t;
    }
}

int main(int argc, char** argv) {
#ifdef __XENO__
  RT_TASK main_task;
  char main_task_name[256] = {
      '\0',
  };
  printf("abcdabcd");
  sprintf(main_task_name, "drfl_main_t");
  if (rt_task_self() == 0) {
    rt_task_shadow(&main_task, main_task_name, 55, T_CPU(3));
  }
#endif

  // �ݹ� ���(// �ݹ� �Լ� �������� 50msec �̳� �۾��� ������ ��)
  Drfl.set_on_rt_monitoring_data(OnRTMonitoringData);

  typedef enum {
    EXAMPLE_JOG,
    EXAMPLE_HOME,
    EXAMPLE_MOVEJ_ASYNC,
    EXAMPLE_MOVEL_SYNC,
    EXAMPLE_MOVEJ_SYNC,
    EXAMPLE_DRL_PROGRAM,
    EXAMPLE_GPIO,
    EXAMPLE_MODBUS,
    EXAMPLE_LAST,
    EXAMPLE_SERVO_OFF
  } EXAMPLE;

  EXAMPLE eExample = EXAMPLE_LAST;

#ifdef __XENO__
  RT_TASK sub_task;
  char sub_task_name[256] = {
      '\0',
  };
  sprintf(sub_task_name, "drfl_sub_t");
  uint32_t stack_size = 1024 * 64;
  uint32_t prio = 50;
  if (rt_task_spawn(&sub_task, sub_task_name, stack_size, prio,
                    T_CPU(3) | /*T_SUSP |*/ T_JOINABLE,
                    (void (*)(void*)) & ThreadFunc, nullptr) != 0) {
    cout << "Can not create sub task" << endl;
  }
#else
  HANDLE hThread;
  DWORD dwThreadID;
  hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &dwThreadID);
  if (hThread == 0) {
    printf("Thread Error\n");
    return 0;
  }
#endif

  bool bLoop = TRUE;
  while (bLoop) {
    g_mStat = false;
    g_Stop = false;
#ifdef __XENO__
    unsigned long overrun = 0;
    const double tick = 1000000;  // 1ms
    rt_task_set_periodic(nullptr, TM_NOW, tick);
    if (rt_task_wait_period(&overrun) == -ETIMEDOUT) {
      std::cout << __func__ << ": \x1B[37m\x1B[41mover-runs: " << overrun
                << "\x1B[0m\x1B[0K" << std::endl;
    }
#else
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
#endif  // __XENO__
#if 0
        static char ch = '0';
        if (ch == '7') ch = '0';
        else if (ch == '0') ch = '7';
#else
    cout << "\ninput key : ";
    // char ch = _getch();
    char ch;
    cin >> ch;
    cout << ch << endl;
#endif
    switch (ch) {
      case '0': {
        switch ((int)eExample) {
          case EXAMPLE_JOG:
            assert(Drfl.Jog(JOG_AXIS_JOINT_1, MOVE_REFERENCE_BASE, 0.f));
            cout << "jog stop" << endl;
            break;
          case EXAMPLE_HOME:
            assert(Drfl.Home((unsigned char)0));
            cout << "home stop" << endl;
            break;
          case EXAMPLE_MOVEJ_ASYNC:
            assert(Drfl.MoveStop(STOP_TYPE_SLOW));
            cout << "movej async stop" << endl;
            break;
          case EXAMPLE_MOVEL_SYNC:
          case EXAMPLE_MOVEJ_SYNC:
            break;
          case EXAMPLE_DRL_PROGRAM:
            assert(Drfl.PlayDrlStop(STOP_TYPE_SLOW));
            // assert(Drfl.SetRobotMode(ROBOT_MODE_MANUAL));
            // assert(Drfl.SetRobotSystem(ROBOT_SYSTEM_REAL));
            cout << "drl player stop" << endl;
            break;
          case EXAMPLE_GPIO:
            cout << "reset gpio" << endl;
            for (int i = 0; i < NUM_DIGITAL; i++) {
              assert(Drfl.SetCtrlBoxDigitalOutput((GPIO_CTRLBOX_DIGITAL_INDEX)i,
                                                  FALSE));
            }
            break;
          case EXAMPLE_MODBUS:
            cout << "reset modbus" << endl;
            assert(Drfl.SetModbusValue("mr1", 0));
            break;
          default:
            break;
        }
      } break;
      case '1':
                {
                    //Drfl.connect_rt_control("127.0.0.1", 12348);
                    Drfl.connect_rt_control("192.168.137.100",12347);
                }
                break;
            case '2':
                {
                    string version = "v1.0";
                    float period = 0.001;
                    int losscount = 4;
                    Drfl.set_rt_control_output(version, period, losscount);
                }
                break;
            case '3':
                {
              	  Drfl.start_rt_control();
                }
                break;
            case 'q':
            		  {
      			  Drfl.disconnect_rt_control();

            		  }
            case '4':
      		  {
      			  Drfl.stop_rt_control();
      		  }
            break;
      default:
        break;
    }
    // Sleep(100);
    this_thread::sleep_for(std::chrono::milliseconds(100));
  }

#ifdef __XENO__
  rt_task_join(&sub_task);
#endif // __XENO__

  return 0;
}
