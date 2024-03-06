#pragma once

#include <stddef.h>

extern void* (*pd_realloc)(void* ptr, size_t size);

#define pd_malloc(s) pd_realloc(NULL, (s))

static inline void pd_free(void* ptr) {
	if (ptr != NULL)
		pd_realloc(ptr, 0);
}
