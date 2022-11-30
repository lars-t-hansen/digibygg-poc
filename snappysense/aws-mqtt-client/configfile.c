/* Copyright 2022 KnowIt ObjectNet AS */
/* Author: Lars T Hansen */

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "configfile.h"

struct config_node {
  char* key;
  char* value;
  struct config_node* next;
};

void init_configfile(config_file_t* cfg) {
  cfg->valid = 1;
  cfg->data = NULL;
}

void destroy_configfile(config_file_t* cfg) {
  if (cfg->valid) {
    struct config_node* n = cfg->data;
    while (n != NULL) {
      free(n->key);
      free(n->value);
      struct config_node* tmp = n;
      n = n->next;
      free(tmp);
    }
  }
  cfg->data = NULL;
  cfg->valid = 1;
}

enum {
  GOT_LINE,
  END_OF_FILE,
  READ_ERROR
};

static int read_complete_line(FILE* f, char* buf, size_t bufsiz) {
  if (fgets(buf, bufsiz, f) == NULL) {
    *buf = 0;
    /* error or eof */
    if (feof(f)) {
      return END_OF_FILE;
    }
    fprintf(stderr, "Failed to read from config file\n");
    return READ_ERROR;
  }
  /* Make sure we got a complete, terminated line.  Terminator can be newline or EOF */
  char* p = buf;
  while (*p && *p != '\n') {
    p++;
  }
  if (!*p) {
    if (!feof(f)) {
      fprintf(stderr, "Failed to read a complete line from config file\n");
      fprintf(stderr, "%s\n", buf);
      *buf = 0;
      return READ_ERROR;
    }
    return GOT_LINE;
  }
  assert(*p == '\n' && *(p+1) == 0);
  *p = 0;
  return GOT_LINE;
}

int read_configfile(const char* filename, config_file_t* cfg) {
  FILE* f = fopen(filename, "r");
  if (f == NULL) {
    cfg->valid = 0;
    fprintf(stderr, "Failed to open config file\n");
    return 0;
  }
  char buf[1024];
  struct config_node* first = NULL;
  struct config_node* last = NULL;
  for (;;) {
    int res = read_complete_line(f, buf, sizeof(buf));
    if (res == READ_ERROR) {
      goto failure;
    }
    if (res == END_OF_FILE) {
      break;
    }
    /* buf has a complete NUL-terminated line and the newline has been removed */
    size_t l = strlen(buf);
    char* p;
    /* Discard comments */
    if (buf[0] == '#') {
      /* Comment */
      continue;
    }
    /* Discard blank lines */
    p = buf;
    while (*p && isspace(*p)) {
      p++;
    }
    if (!*p) {
      /* Blank line */
      continue;
    }
    /* Must be a definition */
    p = buf;
    while (*p && *p != '=') {
      p++;
    }
    if (!*p) {
      /* no = sign */
      fprintf(stderr, "Failed to find equal sign\n");
      goto failure;
    }
    /* TODO: Validate that the variable name uses only legal characters */
    ptrdiff_t keylen = p - buf;
    if (keylen == 0) {
      /* no variable name */
      fprintf(stderr, "Failed to find variable name\n");
      goto failure;
    }
    char* keybuf = malloc((size_t)keylen+1);
    if (keybuf == NULL) {
      /* OOM */
      goto failure;
    }
    *p = 0;
    strcpy(keybuf, buf);
    ptrdiff_t vallen = (buf + l) - (p + 1);
    char* valbuf = malloc((size_t)vallen+1);
    if (valbuf == NULL) {
      free(keybuf);
      goto failure;
    }
    strcpy(valbuf, p+1);
    struct config_node* new = malloc(sizeof(struct config_node));
    if (new == NULL) {
      free(keybuf);
      free(valbuf);
      goto failure;
    }
    new->key = keybuf;
    new->value = valbuf;
    new->next = NULL;
    if (first == NULL) {
      first = new;
    } else {
      last->next = new;
    }
    last = new;
  }
  fclose(f);
  cfg->data = first;
  cfg->valid = 1;
  return 1;

 failure:
  fclose(f);
  cfg->valid = 0;
  while (first != NULL) {
    free(first->key);
    free(first->value);
    struct config_node* tmp = first;
    first = first->next;
    free(tmp);
  }
  return 0;
}

const char* lookup_config(const config_file_t* cfg, const char* key) {
  if (!cfg->valid) {
    abort();
  }
  struct config_node* n = cfg->data;
  while (n != NULL && strcmp(n->key, key) != 0) {
    n = n->next;
  }
  if (n == NULL) {
    return NULL;
  }
  return n->value;
}

#ifndef NDEBUG
void dump_config(const config_file_t* cfg, FILE* out) {
  if (!cfg->valid) {
    fprintf(out, "Invalid config\n");
    return;
  }
  struct config_node* n = cfg->data;
  while (n != NULL) {
    fprintf(out, "%s %s\n", n->key, n->value);
    n = n->next;
  }
}
#endif
