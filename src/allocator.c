// SPDX-License-Identifier: Unlicense

#include "allocator.h"

void* (*pd_realloc)(void* ptr, size_t size);
