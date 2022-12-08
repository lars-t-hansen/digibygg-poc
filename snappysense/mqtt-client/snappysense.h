/* -*- fill-column: 100 -*- */

#ifndef snappysense_h_included
#define snappysense_h_included

#include <stdint.h>
#include "configfile.h"

/*
 * The MQTT main loop will poll for new traffic every n seconds, configured in the config file.  At
 * that point, it will call snappysense_get_message to retrieve an outgoing message.  If there is
 * one, it will post it and then call snappysense_get_message again, in case there's any other
 * outgoing traffic; this repeats until there's no traffic.
 *
 * At this time it will also look for incoming traffic, and if there is any it will post it to the
 * appropriate callbacks for the topic.
 */

typedef struct subscription_t {
  const char* topic;
  void (*callback)(const char* topic, const uint8_t* payload, size_t payload_length);
} subscription_t;

/*
 * The generic client loop.  main() calls this, passing in the parsed config file and the
 * information about the subscriptions we want.  (Subscriptions are not dynamic in this setup.)
 */
int mqttClientMainLoop(config_file_t* cfg,
		       size_t numSubscriptions,
		       subscription_t* subscriptions);

/* Get a startup payload if there is one.  Will not block for long but may block while 
 * obtaining startup data.
 *
 * Return 1 if there is a new payload that fits in the buffer and 0 if not.
 */
int snappysense_get_startup(char* topicBuf, size_t topicBufsiz,
			    uint8_t* payloadBuf, size_t payloadBufsiz,
			    size_t* payloadLen);

/* Get a reading payload if there is one.  Will not block for long but may block while 
 * obtaining sensor readings.
 *
 * Return 1 if there is a new payload that fits in the buffer and 0 if not.
 */
int snappysense_get_reading(char* topicBuf, size_t topicBufsiz,
			    uint8_t* payloadBuf, size_t payloadBufsiz,
			    size_t* payloadLen);

/* Sensor layer.  The *types* of known factors are baked into the code but whether a device has a
 * working sensor for a factor is configured / detected at run-time.  The code reporting readings is
 * sensitive to this.
 *
 * Each adjust_factor() function takes the current reading as observed by the server and the ideal
 * computed by the server, and is expected to "do something" to bring the current state closer to
 * the ideal.  It may not be the best API; discuss.
 */

/* Call once to configure the sensors */
void configure_sensors();

/* Temperature in integer degrees Celsius */
int has_temperature_sensor();
int read_temperature();

int has_temperature_actuator();
void adjust_temperature(int reading, int ideal);

/* Humidity in integer percent relative */
int has_humidity_sensor();
int read_humidity();

int has_humidity_actuator();
void adjust_humidity(int reading, int ideal);

#endif /* snappysense_h_included */
