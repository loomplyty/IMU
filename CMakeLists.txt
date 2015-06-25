cmake_minimum_required(VERSION 2.8)

# add prefix to every string in a variable
FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

FUNCTION(PREPEND_CPP var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}.cpp")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND_CPP)

FUNCTION(PREPEND_H var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}.h")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND_H)

project(ROBOTS)

# set compile options
 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
 

# set binary and library folders
set(LIBRARY_OUTPUT_PATH lib)
set(EXECUTABLE_OUTPUT_PATH bin)

# set include and lib folders
 
include_directories("${PROJECT_SOURCE_DIR}/include") 
include_directories("${PROJECT_SOURCE_DIR}") 
link_directories(${PROJECT_SOURCE_DIR}/lib)
 
# build HexapodIII lib

set(SOURCE_FILES deviceclass.cpp conio.c)
add_library(IMU STATIC ${SOURCE_FILES})
target_link_libraries(IMU xcommunication xstypes) 
set(HEADER_FILES deviceclass.h conio.h)

add_executable(demo_IMU demo_IMU/demo_IMU.cpp)
target_link_libraries(demo_IMU IMU xcommunication xstypes) 

############################################ install ############################################
set(CMAKE_INSTALL_PREFIX "/usr/IMU")
install(TARGETS IMU DESTINATION lib)
install(FILES ${PROJECT_SOURCE_DIR}/lib/libxcommunication.a ${PROJECT_SOURCE_DIR}/lib/libxstypes.a  DESTINATION lib)
install(FILES ${HEADER_FILES}  DESTINATION include)
install(FILES ${SOURCE_FILES}  DESTINATION src)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/xcommunication  ${PROJECT_SOURCE_DIR}/include/xsens DESTINATION include)

############################################ uninstall ############################################
add_custom_target(uninstall rm -r /usr/IMU)
 
