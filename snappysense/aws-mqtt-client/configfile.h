/* Copyright 2022 KnowIt ObjectNet AS */
/* Author Lars T Hansen */

/* Format of the cfg file:
 *
 *   File ::= Line*
 *   Line ::= Comment Eol | Blank Eol | Definition Eol
 *   Eol ::= End of line or End of file
 *   Comment ::= "#" Anything
 *   Blank ::= Whitespace*
 *   Definition ::= Variable"="Anything
 *   Variable ::= [A-Z_][A-Z0-9_]*
 *   Anything ::= .*
 *   Whitespace ::= whatever isspace() says is a whitespace
 *
 * NOTE that in Definitions, whitespace is *significant* *everywhere*.
 */
     
#ifndef configfile_h_included
#define configfile_h_included

#ifndef NDEBUG
#include <stdio.h>
#endif

struct config_node;

typedef struct config_file {
  int valid;
  struct config_node* data;
} config_file_t;

/** 
 * Init the config before reading into it; makes it `valid`.  Do not call this
 * on a config that has data.
 *
 * NOTE: Currently this used dynamic memory allocation and there can be multiple
 * config files live at the same time.  destroy_configfile() is required to properly
 * clean up.
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

#endif /* ifndef configfile_h_included */
