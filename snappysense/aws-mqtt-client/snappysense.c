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

/* Readings sent from devices.  The device ID is encoded in the payload, for now. */
static const char* DEVICE_READING_TOPIC = "snappy/device_reading";

/* Here the last segment can be either "all", a device class, or a device ID.
 * These are always sent with QoS1; those messages persist on the server until
 * they are replaced, which is exactly what we want.  The client must be aware
 * that they may receive duplicates.
 *
 * A client subscribes with "all", its own device class, and its own device ID.
 */
static const char* DEVICE_CONFIG_TOPIC = "snappy/device_config/%s";

/* Location strings.  If LAT or LON is present the other one should be, and in that
   case they are used, and ALT is reported if present.  If neither LAT or LON is present
   then there should be a LOC string. */
static const char* MAYBE_LAT;
static const char* MAYBE_LON;
static const char* MAYBE_ALT;
static const char* MAYBE_LOC;
static const char* DEVICE_CLASS;
static const char* DEVICE_ID;

/* TODO: Read these from config file, optionally */
static int DEVICE_ENABLED = 1;
static time_t DEVICE_MIN_SECONDS_BETWEEN_REPORTS = 5;
static time_t DEVICE_MAX_SECONDS_BETWEEN_REPORTS = 5*60;

static int safe_json_string(const char* p);
static int safe_json_number(const char* p);
static int emit(char** bufp, size_t* availp, const char* fmt, ...);
static int read_temperature();
static int read_humidity();

/* Since this is a toy, let's say we want to allow reconfiguring the reading
 * intervals, and to completely disable readings.  We use the same config
 * procedure for all cases.
 *
 * JSON properties:
 *  "enable": <nonnegative integer, 0 or not-0>
 *  "min_interval_between_reports": <positive integer, represents seconds>
 *  "max_interval_between_reports": <positive integer, represents seconds>
 */

static void config(const char* topic, uint8_t* payload, size_t payloadLen) {
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
      if (pair.jsonType == JSONNumber) {
	char tmp[128];
	if (pair.valueLength >= sizeof(tmp) || pair.keyLength >= sizeof(tmp)) {
	  /* TODO: Log */
	  continue;
	}
	memcpy(tmp, pair.value, pair.valueLength);
	tmp[pair.valueLength] = 0;
	if (!safe_json_number(tmp)) {
	  /* TODO: Log */
	  continue;
	}
	double v = strtod(tmp, NULL);
	if (v < 0 || floor(v) != v || v > INT_MAX) {
	  /* TODO: Log */
	  continue;
	}
	int val = (int)v;
	memcpy(tmp, pair.key, pair.keyLength);
	tmp[pair.keyLength] = 0;
	if (strcmp(tmp, "enable") == 0) {
	  DEVICE_ENABLED = (val != 0);
	} else if (strcmp(tmp, "min_interval_between_reports") == 0) {
	  if (val > 0) {
	    DEVICE_MIN_SECONDS_BETWEEN_REPORTS = val;
	  }
	  /* TODO: Otherwise log */
	} else if (strcmp(tmp, "max_interval_between_reports") == 0) {
	  if (val > 0) {
	    DEVICE_MAX_SECONDS_BETWEEN_REPORTS = val;
	  }
	  /* TODO: Otherwise log */
	}
	/* TODO: Otherwise log */
      }
    }
  }
}

int snappysense_init(config_file_t* cfg, subscription_t** subscriptions, size_t *nSubscriptions) {
  MAYBE_LAT = lookup_config(cfg, "LAT");
  if (!safe_json_number(MAYBE_LAT)) {
    return 0;
  }
  MAYBE_LON = lookup_config(cfg, "LON");
  if (!safe_json_number(MAYBE_LAT)) {
    return 0;
  }
  MAYBE_ALT = lookup_config(cfg, "ALT");
  if (!safe_json_number(MAYBE_ALT)) {
    return 0;
  }
  MAYBE_LOC = lookup_config(cfg, "LOC");
  if (!safe_json_string(MAYBE_LOC)) {
#ifndef NDEBUG
    fprintf(stderr, "Illegal character in LOC string");
#endif
    return 0;
  }
  DEVICE_CLASS = lookup_config(cfg, "DEVICE_CLASS");
  if (!safe_json_string(DEVICE_CLASS)) {
    return 0;
  }
  DEVICE_ID = lookup_config(cfg, "DEVICE_ID");
  if (!safe_json_string(DEVICE_ID)) {
    return 0;
  }

  /* Either LAT and LON, or neither and LOC */
  if (!!MAYBE_LAT != !!MAYBE_LON) {
#ifndef NDEBUG
    fprintf(stderr, "Need both LATitude and LONgitude, or neither");
#endif
    return 0;
  }
  if (MAYBE_LOC == NULL && MAYBE_LAT == NULL) {
#ifndef NDEBUG
    fprintf(stderr, "Need both LATitude and LONgitude, or LOCation");
#endif
    return 0;
  }

  /* All device characteristics */
  if (DEVICE_CLASS == NULL || DEVICE_ID == NULL) {
#ifndef NDEBUG
    fprintf(stderr, "Need valid DEVICE_CLASS and DEVICE_ID");
#endif
    return 0;
  }

  char all_devices[256];
  char my_class[256];
  char my_device[256];
  if (snprintf(all_devices, sizeof(all_devices), DEVICE_CONFIG_TOPIC, "all") >= sizeof(all_devices)) {
    return 0;
  }
  if (snprintf(my_class, sizeof(my_class), DEVICE_CONFIG_TOPIC, DEVICE_CLASS) >= sizeof(my_class)) {
    return 0;
  }
  if (snprintf(my_device, sizeof(my_device), DEVICE_CONFIG_TOPIC, DEVICE_ID) >= sizeof(my_device)) {
    return 0;
  }
  static subscription_t subs[3];
  subs[0].topic = all_devices;
  subs[0].callback = config;
  subs[1].topic = my_class;
  subs[1].callback = config;
  subs[2].topic = my_device;
  subs[2].callback = config;
  *subscriptions = subs;
  *nSubscriptions = 3;
  return 1;
}

int snappysense_get_message(char* topic_buf, size_t topic_bufsiz, uint8_t* payload_buf, size_t payload_bufsiz, size_t* payloadLen) {  
  static time_t last_time;
  static int last_temperature;
  static int last_humidity;

  if (!DEVICE_ENABLED) {
    return 0;
  }

  time_t t = time(NULL);
  time_t t_delta = t - last_time;
  if (t_delta < DEVICE_MIN_SECONDS_BETWEEN_REPORTS) {
    /* Too soon */
    return 0;
  }

  int temp = read_temperature();
  int hum = read_humidity();
  if (temp == last_temperature && hum == last_humidity && t_delta < DEVICE_MAX_SECONDS_BETWEEN_REPORTS) {
    /* No change and no need to report */
    return 0;
  }

  if (strlen(DEVICE_READING_TOPIC) >= topic_bufsiz) {
    /* Overflow */
    return 0;
  }
  strcpy(topic_buf, DEVICE_READING_TOPIC);

  size_t avail = payload_bufsiz;
  char* buf = (char*)payload_buf;
  if (!emit(&buf, &avail, "{\"device\": \"rpi2\"")) {
    return 0;
  }
  if (MAYBE_LAT != NULL) {
    assert(MAYBE_LON != NULL);
    if (!emit(&buf, &avail, ", \"lat\": %s, \"lon\": %s", MAYBE_LAT, MAYBE_LON)) {
      return 0;
    }
    if (MAYBE_ALT != NULL) {
      if (!emit(&buf, &avail, ", \"alt\": %s", MAYBE_ALT)) {
	return 0;
      }
    }
  } else {
    assert(MAYBE_LOC != NULL);
    if (!emit(&buf, &avail, ", \"loc\": \"%s\"", MAYBE_LOC)) {
      return 0;
    }
  }
  if (!emit(&buf, &avail, ", \"time\": %llu, \"temperature\": %d, \"humidity\": %d}", (unsigned long long)t, temp, hum)) {
    return 0;
  }

  /* Committed */
  last_time = t;
  last_temperature = temp;
  last_humidity = hum;
  return 1;
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

/* Sensor readings */

static int read_temperature() {
  /* TODO: Read an actual sensor */
  static int temp = 1;
  int t = temp;
  temp += 1;
  return t;
}

static int read_humidity() {
  /* TODO: Read an actual sensor */
  static int hum = 100;
  int h = hum;
  hum -= 1;
  return h;
}

