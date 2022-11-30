/* Copyright 2022 KnowIt ObjectNet AS */

#include <stdio.h>

struct config_node;

typedef struct config_file {
  int valid;
  struct config_node* data;
} config_file_t;

/** 
 * Init the config before reading into it; makes it `valid`.  Do not call this
 * on a config that has data.
 */
void init_configfile(config_file_t* cfg);

/**
 * Destroy the config; make it valid if it was not.
 */
void destroy_configfile(config_file_t* cfg);

/**
 * Read from a file into the config.  The config must be valid.
 *
 * Returns 1 on success, 0 on failure.  On success, config_file_t has all the settings.
 */
int read_configfile(const char* filename, config_file_t* cfg);

/**
 * Lookup a value in the config.
 *
 * Returns a NUL-terminated string on success, NULL on failure
 */
const char* lookup_config(const config_file_t* cfg, const char* key);

#ifndef NDEBUG
/**
 * Dump the contents of the config onto the given stream.
 */
void dump_config(const config_file_t* cfg, FILE* out);
#endif