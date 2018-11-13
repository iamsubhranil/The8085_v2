#pragma once

#include "common.h"

typedef struct {
	i64 *arr;
	siz  size;
} DumpData;

void dump_init();
void dump_data(const char *name);
void dump_data_append(
    const char *name); // automatically append date-time to name
void     dump_load(const char *name);
void     dump_load_last();
DumpData dump_get();
void     dump_set(i64 *arr, siz n);
