cmake_minimum_required (VERSION 2.8.11)
project (doosan_api_example)

#https://stackoverflow.com/questions/8774593/cmake-link-to-external-library
# Your-external "mylib", add GLOBAL if the imported library is located in directories above the current.
add_library( drfl SHARED IMPORTED )
# You can define two import-locations: one for debug and one for release.
set_target_properties( drfl PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/libDRFL.a )

add_library( poconet SHARED IMPORTED )
set_target_properties( poconet PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/libPocoNet.so.62 )

add_library( pocofoundation SHARED IMPORTED )
set_target_properties( pocofoundation PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/libPocoFoundation.so.62 )

add_executable (doosan_api_example main.cpp)

target_link_libraries(doosan_api_example drfl poconet pocofoundation)