//
//  mini3d.h
//  Extension
//
//  Created by Dave Hayden on 10/20/15.
//  Copyright © 2015 Panic, Inc. All rights reserved.
//

#pragma once

#include <stddef.h>

extern void* (*m3d_realloc)(void* ptr, size_t size);

#define m3d_malloc(s) m3d_realloc(NULL, (s))
#define m3d_free(ptr) m3d_realloc((ptr), 0)

void mini3d_setRealloc(void* (*realloc)(void* ptr, size_t size));
