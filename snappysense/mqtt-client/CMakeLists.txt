cmake_minimum_required( VERSION 3.2.0 )
project( snappysense
         VERSION 0.1.0
	 LANGUAGES C )

set( CMAKE_C_STANDARD 90 )
set( CMAKE_C_STANDARD_REQUIRED True )

set( AWS "/home/debian/aws-iot-device-sdk-embedded-C" )
set( STDLIB "${AWS}/libraries/standard" )
set( LIBDIR "${AWS}/build/lib" )

add_executable( snappysense snappysense.c configfile.c sensors.c mqtt-client.c )
target_compile_options( snappysense PRIVATE -O2 )

target_include_directories( snappysense
  PRIVATE "."
  PRIVATE "${STDLIB}/coreMQTT/source/include"
  PRIVATE "${STDLIB}/coreMQTT/source/interface"
  PRIVATE "${STDLIB}/coreJSON/source/include"
  PRIVATE "${STDLIB}/backoffAlgorithm/source/include"
  PRIVATE "${AWS}/demos/logging-stack"
  PRIVATE "${AWS}/platform/posix/transport/include"
  PRIVATE "${AWS}/platform/include" )

target_link_options( snappysense PRIVATE -lm )
target_link_libraries( snappysense
  "${LIBDIR}/libaws_iot_backoff_algorithm.so"
  "${LIBDIR}/libaws_iot_http.so"
  "${LIBDIR}/libaws_iot_json.so"
  "${LIBDIR}/libaws_iot_mqtt.so"
  "${LIBDIR}/libclock_posix.so"
  "${LIBDIR}/libopenssl_posix.so" )

