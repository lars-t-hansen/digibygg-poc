# -*- fill-column: 128 -*-
#
# (This makefile is a pretty gross hack, with the extra .o files, but it works.)
#
# For this to work, you must have configured and built the libraries for the AWS IOT embedded C SDK.  Follow the steps in the
# instructions for configuring, and then just type 'make' in the build/ directory.  (This builds the demos too; maybe there is
# some narrower target just to build the libs.)

.PHONY: all

AWS=/home/pi/p/aws-iot-device-sdk-embedded-C
STDLIB=$(AWS)/libraries/standard

COMPILE=$(CC) -I$(STDLIB)/coreMQTT/source/include -I$(STDLIB)/coreMQTT/source/interface -I$(STDLIB)/coreJSON/source/include -I$(STDLIB)/backoffAlgorithm/source/include -I$(AWS)/demos/logging-stack -I$(AWS)/platform/posix/transport/include -I$(AWS)/platform/include -I. -O2 -c $<

OBJECTS=mqtt-client.o configfile.o snappysense.o backoff_algorithm.o core_mqtt.o core_mqtt_state.o core_mqtt_serializer.o

.c.o:
	$(COMPILE)

all: snappysense

clean:
	rm -f *.o *~ snappysense

snappysense: $(OBJECTS)
	$(CC) -o snappysense $(OBJECTS) -lm $(AWS)/build/lib/*.so 

core_mqtt_state.o: $(STDLIB)/coreMQTT/source/core_mqtt_state.c
	$(COMPILE)

core_mqtt.o: $(STDLIB)/coreMQTT/source/core_mqtt.c
	$(COMPILE)

core_mqtt_serializer.o: $(STDLIB)/coreMQTT/source/core_mqtt_serializer.c
	$(COMPILE)

backoff_algorithm.o: $(STDLIB)/backoffAlgorithm/source/backoff_algorithm.c
	$(COMPILE)

*.o: Makefile

