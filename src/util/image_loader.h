#pragma once

#include <stdint.h>

uint8_t* read_tga_file_grayscale(const char* path, void* pd_api, int* out_w, int* out_h);
