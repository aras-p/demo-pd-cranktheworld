// SPDX-License-Identifier: Unlicense

#pragma once

#include <stdbool.h>

typedef struct wav_file_desc {
	const void* sample_data;
	int sample_data_size;
	int sample_count;
	int sample_rate;
	int channel_count;
	int sample_format;

	int block_size;
	int samples_per_block;
} wav_file_desc;

typedef struct wav_decode_state {
	float* block;
	int block_index;
	int block_size_bytes;
	int samples_per_block;
} wav_decode_state;

bool wav_parse_header(const void* data, size_t data_size, wav_file_desc* res);

void wav_decode_state_init(const wav_file_desc* desc, wav_decode_state* state);

void wav_ima_adpcm_decode(float* __restrict output, int sample_pos, int sample_count, const void* data, wav_decode_state* state);
