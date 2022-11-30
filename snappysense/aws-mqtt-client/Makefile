# This is a pretty gross hack.
#
# For this to work, you probably must have configured and built a demo
# that builds the necessary libraries.  For sure, mqtt_demo_mutual_auth would work.

AWS=/home/pi/p/aws-iot-device-sdk-embedded-C
STDLIB=$(AWS)/libraries/standard

COMPILE=$(CC) -I$(STDLIB)/coreMQTT/source/include -I$(STDLIB)/coreMQTT/source/interface -I$(STDLIB)/backoffAlgorithm/source/include -I$(AWS)/demos/logging-stack -I$(AWS)/platform/posix/transport/include -I$(AWS)/platform/include -I. -O2 -c $<

OBJECTS=snappysense.o backoff_algorithm.o core_mqtt.o core_mqtt_state.o core_mqtt_serializer.o

.c.o:
	$(COMPILE)

snappysense: $(OBJECTS)
	$(CC) -o snappysense $(OBJECTS) $(AWS)/build/lib/*.so 

core_mqtt_state.o: $(STDLIB)/coreMQTT/source/core_mqtt_state.c
	$(COMPILE)

core_mqtt.o: $(STDLIB)/coreMQTT/source/core_mqtt.c
	$(COMPILE)

core_mqtt_serializer.o: $(STDLIB)/coreMQTT/source/core_mqtt_serializer.c
	$(COMPILE)

backoff_algorithm.o: $(STDLIB)/backoffAlgorithm/source/backoff_algorithm.c
	$(COMPILE)

*.o: Makefile

