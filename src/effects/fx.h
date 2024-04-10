#pragma once

#include "../globals.h"

typedef struct Effect {
	int (*update)();
} Effect;
