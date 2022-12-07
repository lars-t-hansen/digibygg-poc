/* -*- fill-column: 80 -*- */

/* Copyright 2022 KnowIt ObjectNet AS */
/* Author Lars T Hansen */

/* Snappysense application logic.  The mqtt client code (see mqtt-client.c) is
 * generic.  The application logic is polled by the client for outgoing
 * messages, and it can subscribe and unsubscribe to topics with the client,
 * registering callbacks for the topics.
 */

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "configfile.h"
#include "snappysense.h"
#include "core_json.h"

/* Device -> broker.  Parameters are device_class, device_id. */
static const char* const snappy_startup_fmt = "snappy/startup/%s/%s";

/* Device -> broker.  Parameters are device_class, device_id. */
static const char* const snappy_reading_fmt = "snappy/reading/%s/%s";

/* Broker -> device.  Parameter is device_class, device_id, or "all". */
static const char* const snappy_control_fmt = "snappy/control/%s";

/* Broker -> device.  Parameter is device_id. */
static const char* const snappy_command_fmt = "snappy/command/%s";

static const char* DEVICE_CLASS;
static const char* DEVICE_ID;

/* Runtime configuration.  Could be read from config file (but is currently
 * not); can be configured by server.
 */
static int DEVICE_ENABLED = 1;
static time_t READING_INTERVAL = 5; /* Seconds */

/* Fixed maximum reporting interval even if no new sensor values. */
static time_t CALL_HOME_INTERVAL = 5*60; /* Seconds */

static int safe_json_string(const char* p);
static int safe_json_number(const char* p);
static int emit(char** bufp, size_t* availp, const char* fmt, ...);

static void control_callback(const char* topic, const uint8_t* payload, size_t payloadLen);
static void command_callback(const char* topic, const uint8_t* payload, size_t payloadLen);

static int get_json_integer(JSONPair_t* pair, const char* key, int* val);
static int get_json_string(JSONPair_t* pair, const char* key, char* buf, size_t bufsiz);

int snappysense_init(config_file_t* cfg, subscription_t** subscriptions, size_t *nSubscriptions) {
  DEVICE_CLASS = lookup_config(cfg, "DEVICE_CLASS");
  if (!safe_json_string(DEVICE_CLASS)) {
    return 0;
  }
  DEVICE_ID = lookup_config(cfg, "DEVICE_ID");
  if (!safe_json_string(DEVICE_ID)) {
    return 0;
  }

  /* All device characteristics */
  if (DEVICE_CLASS == NULL || DEVICE_ID == NULL) {
#ifndef NDEBUG
    fprintf(stderr, "Need valid DEVICE_CLASS and DEVICE_ID");
#endif
    return 0;
  }

#define SIZE 256

#if 0
  static char control_all_devices[SIZE];
  static char control_my_class[SIZE];
  static char control_my_device[SIZE];
  static char command_my_device[SIZE];
  static subscription_t subs[4];
  if (snprintf(control_all_devices, SIZE, snappy_control_fmt, "all") >= SIZE) {
    return 0;
  }
  if (snprintf(control_my_class, SIZE, snappy_control_fmt, DEVICE_CLASS) >= SIZE) {
    return 0;
  }
  if (snprintf(control_my_device, SIZE, snappy_control_fmt, DEVICE_ID) >= SIZE) {
    return 0;
  }
  if (snprintf(command_my_device, SIZE, snappy_command_fmt, DEVICE_ID) >= SIZE) {
    return 0;
  }
  subs[0].topic = control_all_devices;
  subs[0].callback = control_callback;
  subs[1].topic = control_my_class;
  subs[1].callback = control_callback;
  subs[2].topic = control_my_device;
  subs[2].callback = control_callback;
  subs[3].topic = command_my_device;
  subs[3].callback = command_callback;
  *subscriptions = subs;
  *nSubscriptions = 4;
#endif
#undef SIZE

  configure_sensors();

  return 1;
}

int snappysense_get_startup(char* topic_buf, size_t topic_bufsiz, uint8_t* payload_buf, size_t payload_bufsiz, size_t* payloadLen) {
  time_t t = time(NULL);
  if (snprintf(topic_buf, topic_bufsiz, snappy_startup_fmt, DEVICE_CLASS, DEVICE_ID) >= topic_bufsiz) {
    /* overflow */
    return 0;
  }
  if ((*payloadLen = snprintf(payload_buf, payload_bufsiz, "{\"time\": %llu, \"reading_interval\": %lld}",
			      (unsigned long long)t,
			      (unsigned long long)READING_INTERVAL) >= payload_bufsiz)) {
    /* overflow */
    return 0;
  }
  *payloadLen = strlen(payload_buf);
  return 1;
}

int snappysense_get_reading(char* topic_buf, size_t topic_bufsiz, uint8_t* payload_buf, size_t payload_bufsiz, size_t* payloadLen) {  
  static time_t last_time;
  static int last_temperature;
  static int last_humidity;

  if (!DEVICE_ENABLED) {
    return 0;
  }

  time_t t = time(NULL);
  time_t t_delta = t - last_time;
  if (t_delta < READING_INTERVAL) {
    /* Too soon */
    return 0;
  }

  int temp = has_temperature() ? read_temperature() : last_temperature;
  int hum = has_humidity() ? read_humidity() : last_humidity;
  if (temp == last_temperature && hum == last_humidity && t_delta < CALL_HOME_INTERVAL) {
    /* No change and no need to report */
    return 0;
  }

  if (snprintf(topic_buf, topic_bufsiz, snappy_reading_fmt, DEVICE_CLASS, DEVICE_ID) >= topic_bufsiz) {
    /* Overflow */
#ifndef NDEBUG
    fprintf(stderr, "Topic buffer too small for reading topic\n");
#endif
    return 0;
  }

  int success = 1;
  size_t avail = payload_bufsiz;
  char* buf = (char*)payload_buf;

  success && (success = emit(&buf, &avail, "{\"time\": %lld", (unsigned long long)t));
  has_temperature() && success && (success = emit(&buf, &avail, ", \"temperature\": %d", temp));
  has_humidity() && success && (success = emit(&buf, &avail, ", \"humidity\": %d", hum));
  success && (success = emit(&buf, &avail, "}"));

  if (!success) {
#ifndef NDEBUG
    fprintf(stderr, "Payload buffer too small for reading\n");
#endif
    return 0;
  }
  *payloadLen = strlen(payload_buf);

  /* Committed */
  last_time = t;
  last_temperature = temp;
  last_humidity = hum;
  return 1;
}

/* Callbacks for subscriptions */

/* Control message.  All fields are optional:
 *
 *    { enable: <integer>, reading_interval: <integer> }
 */
static void control_callback(const char* topic, const uint8_t* payload, size_t payloadLen) {
  fprintf(stderr, "Received control message: %*.s\n", payloadLen, payload);
  JSONPair_t pair = { 0 };
  JSONStatus_t result;
  size_t start = 0, next = 0;
  result = JSON_Validate((const char*)payload, payloadLen);
  if (result == JSONSuccess) {
    for (;;) {
      result = JSON_Iterate((const char*)payload, payloadLen, &start, &next, &pair);
      if (result != JSONSuccess) {
	break;
      }
      int v;
      if (get_json_integer(&pair, "enable", &v) && (v == 0 || v == 1)) {
	DEVICE_ENABLED = (v != 0);
      } else if (get_json_integer(&pair, "reading_interval", &v) && v > 0) {
	READING_INTERVAL = v;
      } else {
	/* bogus */
      }
    }
  }
}

/* Command message.  All fields are mandatory:
 *
 *   { sensor: <string>, reading: <number>, ideal: <number> }
 *
 * where `sensor` is one of the known types:
 *   "temperature"
 *   "humidity"
 */
static void command_callback(const char* topic, const uint8_t* payload, size_t payloadLen) {
  fprintf(stderr, "Received command message: %*.s\n", payloadLen, payload);
  JSONPair_t pair = { 0 };
  JSONStatus_t result;
  size_t start = 0, next = 0;
  int reading_value, ideal_value;
  int got_reading = 0, got_ideal = 0, got_sensor = 0, is_temperature = 0, is_humidity = 0;
  result = JSON_Validate((const char*)payload, payloadLen);
  if (result == JSONSuccess) {
    for (;;) {
      result = JSON_Iterate((const char*)payload, payloadLen, &start, &next, &pair);
      if (result != JSONSuccess) {
	break;
      }
      char sensor_buf[128];
      if (!got_reading && get_json_integer(&pair, "reading", &reading_value)) {
	got_reading = 1;
      } else if (!got_ideal && get_json_integer(&pair, "ideal", &ideal_value)) {
	got_ideal = 1;
      } else if (!got_sensor && get_json_string(&pair, "sensor", sensor_buf, sizeof(sensor_buf))) {
	got_sensor =1;
	if (strcmp(sensor_buf, "temperature") == 0) {
	  is_temperature = 1;
	} else if (strcmp(sensor_buf, "humidity") == 0) {
	  is_humidity = 1;
	} else {
	  /* bogus */
	}
      } else {
	/* bogus */
      }
    }
  }
  if (got_reading && got_ideal) {
    if (has_temperature() && is_temperature) {
      adjust_temperature(reading_value, ideal_value);
    } else if (has_humidity() && is_humidity) {
      adjust_humidity(reading_value, ideal_value);
    }
  }
}

/* String manipulation */

static int safe_json_string(const char* p) {
  if (p == NULL) {
    return 1;
  }
  for (; *p != 0 ; p++ ) {
    if (*p == '"' || *p < ' ' || *p > '~') {
      return 0;
    }
  }
  return 1;
}

static int safe_json_number(const char* p) {
  if (p == NULL) {
    return 1;
  }
  const char* start = p;
  while (*p && isdigit(*p)) {
    p++;
  }
  if (p == start) {
    return 0;
  }
  if (*p == '.') {
    p++;
    start = p;
    while (*p && isdigit(*p)) {
      p++;
    }
    if (p == start) {
      return 0;
    }
  }
  return *p == 0;
}

static int emit(char** bufp, size_t* availp, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t k;
  if ((k = vsnprintf(*bufp, *availp, fmt, args)) >= *availp) {
    return 0;
  }
  *bufp += k;
  *availp -= k;
  return 1;
}

static int get_json_integer(JSONPair_t* pair, const char* key, int* val) {
  if (pair->jsonType != JSONNumber) {
    return 0;
  }
  if (strlen(key) != pair->keyLength || strncmp(key, pair->key, pair->keyLength) != 0) {
    return 0;
  }
  char tmp[128];
  if (pair->valueLength >= sizeof(tmp)) {
    return 0;
  }
  memcpy(tmp, pair->value, pair->valueLength);
  tmp[pair->valueLength] = 0;
  if (!safe_json_number(tmp)) {
    return 0;
  }
  double v = strtod(tmp, NULL);
  if (v < 0 || floor(v) != v || v > INT_MAX) {
    return 0;
  }
  *val = (int)v;
  return 1;
}

static int get_json_string(JSONPair_t* pair, const char* key, char* buf, size_t bufsiz) {
  if (pair->jsonType != JSONString) {
    return 0;
  }
  if (strlen(key) != pair->keyLength || strncmp(key, pair->key, pair->keyLength) != 0) {
    return 0;
  }
  if (pair->valueLength >= bufsiz) {
    return 0;
  }
  memcpy(buf, pair->value, pair->valueLength);
  buf[pair->valueLength] = 0;
  return 1;
}
