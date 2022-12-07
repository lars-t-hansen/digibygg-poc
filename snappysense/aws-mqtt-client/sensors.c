/* -*- fill-column: 100 -*- */

/* Copyright 2022 KnowIt ObjectNet AS */
/* Author Lars T Hansen */

/* Snappysense sensor layer.  This is just a shim for now. */

#include "snappysense.h"

void configure_sensors() {
}

int has_temperature_sensor() {
  return 1;
}

int read_temperature() {
  /* TODO: Read an actual sensor */
  static int temp = 1;
  int t = temp;
  temp += 1;
  return t;
}

int has_temperature_actuator() {
  return 1;
}

void adjust_temperature(int reading, int ideal) {
  /* TODO: Control an actual energy source */
  if (reading > ideal) {
    fprintf(stderr, "Adjusting temperature DOWN");
  } else if (ideal > reading) {
    fprintf(stderr, "Adjusting temperature UP");
  }
}

int has_humidity_sensor() {
  return 1;
}

int read_humidity() {
  /* TODO: Read an actual sensor */
  static int hum = 100;
  int h = hum;
  hum -= 1;
  return h;
}

int has_humidity_actuator() {
  return 1;
}

void adjust_humidity(int reading, int ideal) {
  /* TODO: Control an actual humidifyer */
  if (reading > ideal) {
    /* Iffy */
    fprintf(stderr, "Adjusting humidity DOWN");
  } else if (ideal > reading) {
    fprintf(stderr, "Adjusting humidity UP");
  }
}
