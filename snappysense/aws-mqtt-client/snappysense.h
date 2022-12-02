/* -*- fill-column: 80 -*- */

#ifndef snappysense_h_included
#define snappysense_h_included

#include "configfile.h"

/*
 * The MQTT main loop will poll for new traffic every n seconds, configured in
 * the config file.  At that point, it will call snappysense_get_message to
 * retrieve an outgoing message.  If there is one, it will post it and then call
 * snappysense_get_message again, in case there's any other outgoing traffic;
 * this repeats until there's no traffic.
 *
 * At this time it will also look for incoming traffic, and if there is any it
 * will post it to the appropriate callbacks for the topic.
 */

typedef struct subscription_t {
  const char* topic;
  void (*callback)(const char* topic, uint8_t* payload, size_t payload_length);
} subscription_t;

/**
 * Initialize variables from the config file and obtain subscriptions.
 *
 * The callee may set *n_subscriptions (its default value is 0).  If
 * *n_subscriptions > 0, then the callee *must* set *subscriptions to an array
 * of length at least *n_subscriptions that has topics and callbacks.
 *
 * Returns 1 if everything is OK, 0 on fatal error.
 */
int snappysense_init(config_file_t* cfg, subscription_t** subscriptions, size_t* n_subscriptions);

/**
 * Get a payload if there is one.  Will not block for long but may block while 
 * obtaining sensor readings.
 *
 * Return 1 if there is a new payload that fits in the buffer and 0 if not.
 */
int snappysense_get_message(char* topicBuf, size_t topicBufsiz, uint8_t* payloadBuf, size_t payloadBufsiz, size_t* payloadLen);

#endif /* snappysense_h_included */
