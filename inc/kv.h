#ifndef KV_H
#define KV_H

#include <stdlib.h>
#include <stdio.h>

#define TOMBSTONE ((char *)0x1)

typedef struct {
    char *key;
    char *value;
} kv_entry_t;

typedef struct {
    kv_entry_t *entries;
    size_t      capacity;
    size_t      count;
} kv_t;

kv_t  *kv_init(size_t capacity);

#endif