cmake_minimum_required( VERSION 3.2.0 )
project( snappysense
         VERSION 0.1.0
	 LANGUAGES C )

add_executable( snappysense snappysense.c configfile.c sensors.c )
set( CMAKE_C_STANDARD 90 )
set( CMAKE_C_STANDARD_REQUIRED True )

set( AWS "/users/lth/p/iotcore/aws-iot-device-sdk-embedded-C" )
set( STDLIB "${AWS}/libraries/standard" )

target_include_directories( snappysense
  PRIVATE "${STDLIB}/coreMQTT/source/include"
  PRIVATE "${STDLIB}/coreMQTT/source/interface"
  PRIVATE "${STDLIB}/coreJSON/source/include"
  PRIVATE "${STDLIB}/backoffAlgorithm/source/include"
  PRIVATE "${AWS}/demos/logging-stack"
  PRIVATE "${AWS}/platform/posix/transport/include"
  PRIVATE "${AWS}/platform/include" )
