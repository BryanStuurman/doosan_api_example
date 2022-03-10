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

void OnTpInitializingCompleted() {
  // Tp �ʱ�ȭ ���� ����� ��û.
  g_TpInitailizingComplted = TRUE;
  Drfl.ManageAccessControl(MANAGE_ACCESS_CONTROL_FORCE_REQUEST);
}

void OnHommingCompleted() {
  // 50msec �̳� �۾��� ������ ��.
  cout << "homming completed" << endl;
}

void OnProgramStopped(const PROGRAM_STOP_CAUSE) {
  assert(Drfl.PlayDrlStop(STOP_TYPE_SLOW));
  // 50msec �̳� �۾��� ������ ��.
  // assert(Drfl.SetRobotMode(ROBOT_MODE_MANUAL));
  cout << "program stopped" << endl;
}

void OnMonitoringDataCB(const LPMONITORING_DATA pData) {
  // 50msec �̳� �۾��� ������ ��.

  return;
  cout << "# monitoring 0 data " << pData->_tCtrl._tTask._fActualPos[0][0]
       << pData->_tCtrl._tTask._fActualPos[0][1]
       << pData->_tCtrl._tTask._fActualPos[0][2]
       << pData->_tCtrl._tTask._fActualPos[0][3]
       << pData->_tCtrl._tTask._fActualPos[0][4]
       << pData->_tCtrl._tTask._fActualPos[0][5] << endl;
}

void OnMonitoringDataExCB(const LPMONITORING_DATA_EX pData) {
  return;
  cout << "# monitoring 1 data " << pData->_tCtrl._tWorld._fTargetPos[0]
       << pData->_tCtrl._tWorld._fTargetPos[1]
       << pData->_tCtrl._tWorld._fTargetPos[2]
       << pData->_tCtrl._tWorld._fTargetPos[3]
       << pData->_tCtrl._tWorld._fTargetPos[4]
       << pData->_tCtrl._tWorld._fTargetPos[5] << endl;
}

void OnMonitoringCtrlIOCB(const LPMONITORING_CTRLIO pData) {
  return;
  cout << "# monitoring ctrl 0 data" << endl;
  for (int i = 0; i < 16; i++) {
    cout << (int)pData->_tInput._iActualDI[i] << endl;
  }
}

void OnMonitoringCtrlIOExCB(const LPMONITORING_CTRLIO_EX pData) {
  return;
  cout << "# monitoring ctrl 1 data" << endl;
  for (int i = 0; i < 16; i++) {
    cout << (int)pData->_tInput._iActualDI[i] << endl;
  }
  for (int i = 0; i < 16; i++) {
    cout << (int)pData->_tOutput._iTargetDO[i] << endl;
  }
}

void OnMonitoringStateCB(const ROBOT_STATE eState) {
  // 50msec �̳� �۾��� ������ ��.
  switch ((unsigned char)eState) {
#if 0  // TP �ʱ�ȭ�� ����ϴ� ���������� API ���������� ������� ����.(TP����
       // �ܵ� ����� ���, ���)
    case STATE_NOT_READY:
        if (g_bHasControlAuthority) Drfl.SetRobotControl(CONTROL_INIT_CONFIG);
        break;
    case STATE_INITIALIZING:
        // add initalizing logic
        if (g_bHasControlAuthority) Drfl.SetRobotControl(CONTROL_ENABLE_OPERATION);
        break;
#endif
    case STATE_EMERGENCY_STOP:
      // popup
      break;
    case STATE_STANDBY:
    case STATE_MOVING:
    case STATE_TEACHING:
      break;
    case STATE_SAFE_STOP:
      if (g_bHasControlAuthority) {
        Drfl.SetSafeStopResetType(SAFE_STOP_RESET_TYPE_DEFAULT);
        Drfl.SetRobotControl(CONTROL_RESET_SAFET_STOP);
      }
      break;
    case STATE_SAFE_OFF:
      // cout << "STATE_SAFE_OFF1" << endl;
      if (g_bHasControlAuthority) {
        // cout << "STATE_SAFE_OFF2" << endl;
        Drfl.SetRobotControl(CONTROL_SERVO_ON);
      }
      break;
    case STATE_SAFE_STOP2:
      if (g_bHasControlAuthority)
        Drfl.SetRobotControl(CONTROL_RECOVERY_SAFE_STOP);
      break;
    case STATE_SAFE_OFF2:
      if (g_bHasControlAuthority) {
        Drfl.SetRobotControl(CONTROL_RECOVERY_SAFE_OFF);
      }
      break;
    case STATE_RECOVERY:
      // Drfl.SetRobotControl(CONTROL_RESET_RECOVERY);
      break;
    default:
      break;
  }
  return;
  cout << "current state: " << (int)eState << endl;
}

void OnMonitroingAccessControlCB(
    const MONITORING_ACCESS_CONTROL eTrasnsitControl) {
  // 50msec �̳� �۾��� ������ ��.

  switch (eTrasnsitControl) {
    case MONITORING_ACCESS_CONTROL_REQUEST:
      assert(Drfl.ManageAccessControl(MANAGE_ACCESS_CONTROL_RESPONSE_NO));
      // Drfl.ManageAccessControl(MANAGE_ACCESS_CONTROL_RESPONSE_YES);
      break;
    case MONITORING_ACCESS_CONTROL_GRANT:
      g_bHasControlAuthority = TRUE;
      // cout << "GRANT1" << endl;
      // cout << "MONITORINGCB : " << (int)Drfl.GetRobotState() << endl;
      OnMonitoringStateCB(Drfl.GetRobotState());
      // cout << "GRANT2" << endl;
      break;
    case MONITORING_ACCESS_CONTROL_DENY:
    case MONITORING_ACCESS_CONTROL_LOSS:
      g_bHasControlAuthority = FALSE;
      if (g_TpInitailizingComplted) {
        // assert(Drfl.ManageAccessControl(MANAGE_ACCESS_CONTROL_REQUEST));
        Drfl.ManageAccessControl(MANAGE_ACCESS_CONTROL_FORCE_REQUEST);
      }
      break;
    default:
      break;
  }
}

void OnLogAlarm(LPLOG_ALARM tLog) {
  g_mStat = true;
  cout << "Alarm Info: "
       << "group(" << (unsigned int)tLog->_iGroup << "), index("
       << tLog->_iIndex << "), param(" << tLog->_szParam[0] << "), param("
       << tLog->_szParam[1] << "), param(" << tLog->_szParam[2] << ")" << endl;
}

void OnTpPopup(LPMESSAGE_POPUP tPopup) {
  cout << "Popup Message: " << tPopup->_szText << endl;
  cout << "Message Level: " << tPopup->_iLevel << endl;
  cout << "Button Type: " << tPopup->_iBtnType << endl;
}

void OnTpLog(const char* strLog) { cout << "Log Message: " << strLog << endl; }

void OnTpProgress(LPMESSAGE_PROGRESS tProgress) {
  cout << "Progress cnt : " << (int)tProgress->_iTotalCount << endl;
  cout << "Current cnt : " << (int)tProgress->_iCurrentCount << endl;
}

void OnTpGetuserInput(LPMESSAGE_INPUT tInput) {
  cout << "User Input : " << tInput->_szText << endl;
  cout << "Data Type : " << (int)tInput->_iType << endl;
}

void OnRTMonitoringData(LPRT_OUTPUT_DATA_LIST tData)
{
//    static int td = 0;
//    if (td++ == 1000) {
//    	td = 0;
//    	printf("timestamp : %.3f\n", tData->time_stamp);
//    	printf("joint : %f %f %f %f %f %f\n", tData->actual_joint_position[0], tData->actual_joint_position[1], tData->actual_joint_position[2], tData->actual_joint_position[3], tData->actual_joint_position[4], tData->actual_joint_position[5]);
//		printf("q = %7.4f, %7.4f, %7.4f, %7.4f, %7.4f, %7.4f\n",
//				tData->actual_joint_position[0], tData->actual_joint_position[1], tData->actual_joint_position[2],
//				tData->actual_joint_position[3], tData->actual_joint_position[4], tData->actual_joint_position[5]);
//		printf("q_dot = %7.4f, %7.4f, %7.4f, %7.4f, %7.4f, %7.4f\n",
//				tData->actual_joint_velocity[0], tData->actual_joint_velocity[1], tData->actual_joint_velocity[2],
//				tData->actual_joint_velocity[3], tData->actual_joint_velocity[4], tData->actual_joint_velocity[5]);
//		printf("trq_g = %7.4f, %7.4f, %7.4f, %7.4f, %7.4f, %7.4f\n",
//				tData->gravity_torque[0], tData->gravity_torque[1], tData->gravity_torque[2],
//				tData->gravity_torque[3], tData->gravity_torque[4], tData->gravity_torque[5]);
//    }
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
  sprintf(main_task_name, "drfl_main_t");
  if (rt_task_self() == 0) {
    rt_task_shadow(&main_task, main_task_name, 55, T_CPU(3));
  }
#endif  // __XENO__

  // �ݹ� ���(// �ݹ� �Լ� �������� 50msec �̳� �۾��� ������ ��)
  Drfl.set_on_homming_completed(OnHommingCompleted);
  Drfl.set_on_monitoring_data(OnMonitoringDataCB);
  Drfl.set_on_monitoring_data_ex(OnMonitoringDataExCB);
  Drfl.set_on_monitoring_ctrl_io(OnMonitoringCtrlIOCB);
  Drfl.set_on_monitoring_ctrl_io_ex(OnMonitoringCtrlIOExCB);
  Drfl.set_on_monitoring_state(OnMonitoringStateCB);
  Drfl.set_on_monitoring_access_control(OnMonitroingAccessControlCB);
  Drfl.set_on_tp_initializing_completed(OnTpInitializingCompleted);
  Drfl.set_on_log_alarm(OnLogAlarm);
  Drfl.set_on_tp_popup(OnTpPopup);
  Drfl.set_on_tp_log(OnTpLog);
  Drfl.set_on_tp_progress(OnTpProgress);
  Drfl.set_on_tp_get_user_input(OnTpGetuserInput);
  Drfl.set_on_rt_monitoring_data(OnRTMonitoringData);

  Drfl.set_on_program_stopped(OnProgramStopped);
  Drfl.set_on_disconnected(OnDisConnected);

  // ���� ����
  assert(Drfl.open_connection("192.168.137.100"));

  // ���� ���� ȹ��
  SYSTEM_VERSION tSysVerion = {
      '\0',
  };
  Drfl.get_system_version(&tSysVerion);
  // ����͸� ������ ���� ����
  assert(Drfl.setup_monitoring_version(1));
  Drfl.set_robot_control(CONTROL_SERVO_ON);
  Drfl.set_digital_output(GPIO_CTRLBOX_DIGITAL_INDEX_10, TRUE);
  cout << "System version: " << tSysVerion._szController << endl;
  cout << "Library version: " << Drfl.get_library_version() << endl;

  while ((Drfl.get_robot_state() != STATE_STANDBY) || !g_bHasControlAuthority)
    // Sleep(1000);
    this_thread::sleep_for(std::chrono::milliseconds(1000));

  // ���� ��� ����

  assert(Drfl.set_robot_mode(ROBOT_MODE_AUTONOMOUS));
  assert(Drfl.set_robot_system(ROBOT_SYSTEM_REAL));

  // Drfl.ConfigCreateModbus("mr1", "192.168.137.70", 552,
  // MODBUS_REGISTER_TYPE_HOLDING_REGISTER, 3, 5);

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
      case 'q':
        bLoop = FALSE;
        break;
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
              Drfl.connect_rt_control();
          }
          break;
      case '2':
          {
              string version = "v1.0";
              float period = 0.001;
              int losscount = 4;
//              Drfl.set_rt_control_input(version, hz, losscount);
              Drfl.set_rt_control_output(version, period, losscount);
          }
          break;
      case '3':
          {
        	  Drfl.start_rt_control();
          }
          break;
      case '4':
		  {
			  Drfl.stop_rt_control();
		  }
      break;
      case '5':
      {
			float vel[6] = {10, 10, 10, 10, 10, 10};
			float acc[6] = {100, 100, 100, 100, 100, 100};
			Drfl.set_velj_rt(vel);
			Drfl.set_accj_rt(acc);
			Drfl.set_velx_rt(100, 10);
			Drfl.set_accx_rt(200, 20);

			const float st=0.001; // sampling time
			const float ratio=1;

			const float None=-10000;
			float count=0;
			static float time=0;

			float home[6] = {0, 0, 0, 0, 0, 0};
			Drfl.movej(home, 60, 30);

			Drfl.set_safety_mode(SAFETY_MODE_AUTONOMOUS, SAFETY_MODE_EVENT_MOVE);
			TraParam tra;

			// Plan1
			PlanParam plan1;
			plan1.time=5;
			plan1.ps[0]=0; plan1.ps[1]=0; plan1.ps[2]=0; plan1.ps[3]=0; plan1.ps[4]=0; plan1.ps[5]=0;
			plan1.pf[0]=40; plan1.pf[1]=40; plan1.pf[2]=40; plan1.pf[3]=40; plan1.pf[4]=40; plan1.pf[5]=40;
			plan1.vs[0]=0; plan1.vs[1]=0; plan1.vs[2]=0; plan1.vs[3]=0; plan1.vs[4]=0; plan1.vs[5]=0;
			plan1.vf[0]=0; plan1.vf[1]=0; plan1.vf[2]=0; plan1.vf[3]=0; plan1.vf[4]=0; plan1.vf[5]=0;
			plan1.as[0]=0; plan1.as[1]=0; plan1.as[2]=0; plan1.as[3]=0; plan1.as[4]=0; plan1.as[5]=0;
			plan1.af[0]=0; plan1.af[1]=0; plan1.af[2]=0; plan1.af[3]=0; plan1.af[4]=0; plan1.af[5]=0;
			TrajectoryPlan(&plan1);

			while(1)
			{
				time=(++count)*st;
				tra.time=time;

				TrajectoryGenerator(&plan1,&tra);

				for(int i=0; i<6; i++)
				{
					tra.vel[i]=None;
					tra.acc[i]=None;
				}

				if(time > plan1.time)
				{
					time=0;
					tra.pos[0]=10; tra.pos[1]=10; tra.pos[2]=10; tra.pos[3]=10; tra.pos[4]=10; tra.pos[5]=10;
					for(int i=0; i<6; i++)
					{
						tra.vel[i]=0.0;
						tra.acc[i]=0.0;
					}
				}

				Drfl.servoj_rt(tra.pos, tra.vel, tra.acc, st*ratio);

				rt_task_wait_period(NULL);
			}
      }

      break;
      case '6':
      {
			float vel[6] = {10, 10, 10, 10, 10, 10};
			float acc[6] = {100, 100, 100, 100, 100, 100};
          	Drfl.set_velj_rt(vel);
          	Drfl.set_accj_rt(acc);
			Drfl.set_velx_rt(100, 10);
			Drfl.set_accx_rt(200, 20);

			const float st=0.001; // sampling time
			const float ratio=1;

			const float None=-10000;
			float count=0;
			static float time=0;

			float home[6] = {0, 0, 90, 0, 90, 0};
			Drfl.movej(home, 60, 30);
			Drfl.set_safety_mode(SAFETY_MODE_AUTONOMOUS, SAFETY_MODE_EVENT_MOVE);

			TraParam tra;

			// Plan1
			PlanParam plan1;
			plan1.time=5;
			plan1.ps[0]=368.0; plan1.ps[1]=34.5; plan1.ps[2]=442.5; plan1.ps[3]=180; plan1.ps[4]=180; plan1.ps[5]=180;
			plan1.pf[0]=450.0; plan1.pf[1]=34.5; plan1.pf[2]=442.5; plan1.pf[3]=180; plan1.pf[4]=180; plan1.pf[5]=230;
			plan1.vs[0]=0; plan1.vs[1]=0; plan1.vs[2]=0; plan1.vs[3]=0; plan1.vs[4]=0; plan1.vs[5]=0;
			plan1.vf[0]=0; plan1.vf[1]=0; plan1.vf[2]=0; plan1.vf[3]=0; plan1.vf[4]=0; plan1.vf[5]=0;
			plan1.as[0]=0; plan1.as[1]=0; plan1.as[2]=0; plan1.as[3]=0; plan1.as[4]=0; plan1.as[5]=0;
			plan1.af[0]=0; plan1.af[1]=0; plan1.af[2]=0; plan1.af[3]=0; plan1.af[4]=0; plan1.af[5]=0;
			TrajectoryPlan(&plan1);

			while(1)
			{
				time=(++count)*st;
				tra.time=time;

				TrajectoryGenerator(&plan1,&tra);

				for(int i=0; i<6; i++)
				{
					tra.vel[i]=None;
					tra.acc[i]=None;
				}

				if(time > plan1.time)
				{
					time=0;
					tra.pos[0]=450.0; tra.pos[1]=34.5; tra.pos[2]=442.5; tra.pos[3]=180; tra.pos[4]=180; tra.pos[5]=230;
					for(int i=0; i<6; i++)
					{
						tra.vel[i]=0.0;
						tra.acc[i]=0.0;
					}
				}

				Drfl.servol_rt(tra.pos, tra.vel, tra.acc, st*ratio);

				rt_task_wait_period(NULL);
			}
      } break;
      case '7': // speedj
		  {
				float vel_limit[6] = {100, 100, 100, 100, 100, 100};
				float acc_limit[6] = {100, 100, 100, 100, 100, 100};
				float vel[6] = {30, 0, 0, 0, 0, 0};
				float acc[6] = {100, 100, 100, 100, 100, 100};
				float vel0[6] = {0, 0, 0, 0, 0, 0};
				float acc0[6] = {0, 0, 0, 0, 0, 0};
				Drfl.set_velj_rt(vel_limit);
				Drfl.set_accj_rt(acc_limit);
				Drfl.set_velx_rt(100, 10);
				Drfl.set_accx_rt(200, 20);

				float home[6] = {0, 0, 90, 0, 90, 0};
				Drfl.movej(home, 30, 30);
				Drfl.set_safety_mode(SAFETY_MODE_AUTONOMOUS, SAFETY_MODE_EVENT_MOVE);

				const float st=0.001; // sampling time
				const float None=-10000;
				float count=0;
				static float time=0;

				TraParam tra;

				// Plan1
				PlanParam plan1, plan2;
				plan1.time=5;
				plan1.ps[0]=0; plan1.ps[1]=0; plan1.ps[2]=0; plan1.ps[3]=0; plan1.ps[4]=0; plan1.ps[5]=0;
				plan1.pf[0]=10; plan1.pf[1]=0; plan1.pf[2]=0; plan1.pf[3]=0; plan1.pf[4]=0; plan1.pf[5]=0;
				plan1.vs[0]=0; plan1.vs[1]=0; plan1.vs[2]=0; plan1.vs[3]=0; plan1.vs[4]=0; plan1.vs[5]=0;
				plan1.vf[0]=0; plan1.vf[1]=0; plan1.vf[2]=0; plan1.vf[3]=0; plan1.vf[4]=0; plan1.vf[5]=0;
				plan1.as[0]=0; plan1.as[1]=0; plan1.as[2]=0; plan1.as[3]=0; plan1.as[4]=0; plan1.as[5]=0;
				plan1.af[0]=0; plan1.af[1]=0; plan1.af[2]=0; plan1.af[3]=0; plan1.af[4]=0; plan1.af[5]=0;
				TrajectoryPlan(&plan1);

				while(1)
				{
					time=(++count)*st;
					tra.time=time;

					TrajectoryGenerator(&plan1,&tra);

					Drfl.speedj_rt(tra.vel, tra.acc, st);

					if(time > plan1.time)
					{
						time=0;
						Drfl.stop(STOP_TYPE_SLOW);
						break;
					}

					rt_task_wait_period(NULL);
				}
		  }
		  break;
      case '8': // speedl
		  {
				float vel_limit[6] = {100, 100, 100, 100, 100, 100};
				float acc_limit[6] = {100, 100, 100, 100, 100, 100};
				float vel[6] = {50, 50, 50, 10, 10, 10};
				float acc[6] = {100, 100, 100, 100, 100, 100};
				float vel0[6] = {0, 0, 0, 0, 0, 0};
				float acc0[6] = {0, 0, 0, 0, 0, 0};

				Drfl.set_velj_rt(vel_limit);
				Drfl.set_accj_rt(acc_limit);
				Drfl.set_velx_rt(100, 10);
				Drfl.set_accx_rt(200, 20);

				const float st=0.001; // sampling time

				const float None=-10000;
				float count=0;
				static float time=0;

				float home[6] = {0, 0, 90, 0, 90, 0};
				Drfl.movej(home, 60, 30);
				Drfl.set_safety_mode(SAFETY_MODE_AUTONOMOUS, SAFETY_MODE_EVENT_MOVE);

				TraParam tra;

				// Plan1
				PlanParam plan1;
				plan1.time=5;
				plan1.ps[0]=368.0; plan1.ps[1]=34.5; plan1.ps[2]=442.5; plan1.ps[3]=180; plan1.ps[4]=180; plan1.ps[5]=180;
				plan1.pf[0]=468.0; plan1.pf[1]=34.5; plan1.pf[2]=442.5; plan1.pf[3]=180; plan1.pf[4]=180; plan1.pf[5]=230;
				plan1.vs[0]=0; plan1.vs[1]=0; plan1.vs[2]=0; plan1.vs[3]=0; plan1.vs[4]=0; plan1.vs[5]=0;
				plan1.vf[0]=0; plan1.vf[1]=0; plan1.vf[2]=0; plan1.vf[3]=0; plan1.vf[4]=0; plan1.vf[5]=0;
				plan1.as[0]=0; plan1.as[1]=0; plan1.as[2]=0; plan1.as[3]=0; plan1.as[4]=0; plan1.as[5]=0;
				plan1.af[0]=0; plan1.af[1]=0; plan1.af[2]=0; plan1.af[3]=0; plan1.af[4]=0; plan1.af[5]=0;
				TrajectoryPlan(&plan1);

				while(1)
				{
					//Drfl.read_data_rt();
					time=(++count)*st;
					tra.time=time;

					TrajectoryGenerator(&plan1,&tra);

					Drfl.speedl_rt(tra.vel, tra.acc, st);

					if(time > plan1.time)
					{
						time=0;
						break;
					}

					rt_task_wait_period(NULL);
				}
		  }
		  break;
      case '9': // torque
		  {
				float vel[6] = {10, 10, 10, 10, 10, 10};
				float acc[6] = {100, 100, 100, 100, 100, 100};
				float vel0[6] = {0, 0, 0, 0, 0, 0};
				float acc0[6] = {0, 0, 0, 0, 0, 0};
				Drfl.set_velj_rt(vel);
				Drfl.set_accj_rt(acc);
				Drfl.set_velx_rt(100, 10);
				Drfl.set_accx_rt(200, 20);

				float trq_d[6] = {20.0, -23.6934, -23.6913, 4.2867, -0.0022, 0.01};

				float home[6] = {0, 0, 90, 0, 90, 0};
				Drfl.movej(home, 60, 30);
				Drfl.set_safety_mode(SAFETY_MODE_AUTONOMOUS, SAFETY_MODE_EVENT_MOVE);

				const float st=0.001; // sampling time

				const float None=-10000;
				float count=0;
				static float time=0;

				PlanParam plan1;
				plan1.time = 20;

				float q[NUMBER_OF_JOINT] = {0.0, };
				float q_dot[NUMBER_OF_JOINT] = {0.0, };
				float trq_g[NUMBER_OF_JOINT] = {0.0, };
				while (1)
				{
					time=(++count)*st;

					memcpy(q, Drfl.read_data_rt()->actual_joint_position, sizeof(float)*NUMBER_OF_JOINT);
					memcpy(q_dot, Drfl.read_data_rt()->actual_joint_velocity, sizeof(float)*NUMBER_OF_JOINT);
					memcpy(trq_g, Drfl.read_data_rt()->gravity_torque, sizeof(float)*NUMBER_OF_JOINT);
					memcpy(trq_d, trq_g, sizeof(float)*NUMBER_OF_JOINT);

					Drfl.torque_rt(trq_d, 0);

					if(time > plan1.time)
					{
						time=0;
						Drfl.stop(STOP_TYPE_SLOW);
						break;
					}

					rt_task_wait_period(NULL);
				}
		  }
		  break;
      default:
        break;
    }
    // Sleep(100);
    this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  Drfl.CloseConnection();

#ifdef __XENO__
  rt_task_join(&sub_task);
#endif // __XENO__

  return 0;
}
