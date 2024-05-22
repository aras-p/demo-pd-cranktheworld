// SPDX-License-Identifier: Unlicense

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "wav_ima_adpcm.h"

#define WAV_FOURCC(a,b,c,d) (uint32_t)(a | (b << 8) | (c << 16) | (d << 24))

typedef struct wav_chunk {
	uint32_t id;
	uint32_t size;
} wav_chunk;

typedef struct wav_riff {
	uint32_t format;
} wav_riff;

typedef struct wav_format {
	uint16_t format;
	uint16_t channel_count;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_size;
	uint16_t bits_per_sample;
} wav_format;

void wav_decode_state_init(const wav_file_desc* desc, wav_decode_state* state)
{
	state->block = (float*)malloc(desc->samples_per_block * sizeof(float));
	state->block_index = -1;
	state->samples_per_block = desc->samples_per_block;
	state->block_size_bytes = desc->block_size;
}

static const int kImaIndexTable[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static const int kImaStepTable[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static inline int clamp_step_index(int v) {
	if (v < 0) v = 0;
	else if (v > 88) v = 88;
	return v;
}

static inline int clamp_predict(int v) {
	if (v < -32768) v = -32768;
	else if (v > 32767) v = 32767;
	return v;
}

static inline float short_to_float(int v)
{
	return v * (1.0f / 32767.0f);
}

static inline float decode_sample(int nibble, int* step_index, int* predict)
{
	int step = kImaStepTable[*step_index];
	int diff = step >> 3;
	if (nibble & 1) diff += step >> 2;
	if (nibble & 2) diff += step >> 1;
	if (nibble & 4) diff += step;
	if (nibble & 8) diff = -diff;
	*predict = clamp_predict(*predict + diff);
	*step_index = clamp_step_index(*step_index + kImaIndexTable[nibble]);
	return short_to_float(*predict);
}

static void wav_ima_adpcm_decode_block(float * __restrict output, const uint8_t *data, uint64_t sample_count)
{
	uint64_t i;

	int predict = data[0] | (data[1] << 8);
	if (predict & 0x8000)
		predict -= 0x10000;
	int step_index = clamp_step_index(data[2]);
	assert(data[3] == 0);

	output[0] = short_to_float(predict);
	output++;
	data += 4;

	for (i = 1; i < sample_count - 2; i += 2)
	{
		*output++ = decode_sample(data[0] & 0xf, &step_index, &predict);
		*output++ = decode_sample(data[0] >> 4, &step_index, &predict);
		++data;
	}

	int nib[2] = { data[0] & 0xf, data[0] >> 4 };
	for (int j = 0; (j < 2) && ((i + j) < sample_count); ++j) {
		*output++ = decode_sample(nib[j], &step_index, &predict);
	}
}

void wav_ima_adpcm_decode(float* __restrict output, int sample_pos, int sample_count, const void* data, wav_decode_state* state)
{
	while (sample_count > 0)
	{
		// decode a block if needed
		int block_index = sample_pos / state->samples_per_block;
		if (block_index != state->block_index)
		{
			const uint8_t* block_ptr = (const uint8_t*)data + block_index * state->block_size_bytes;
			wav_ima_adpcm_decode_block(state->block, block_ptr, state->samples_per_block);
			state->block_index = block_index;
		}

		// copy the needed chunk of decoded block into output
		int pos_in_block = sample_pos - block_index * state->samples_per_block;
		int samples_to_copy = min(sample_count, state->samples_per_block - pos_in_block);
		memcpy(output, state->block + pos_in_block, samples_to_copy * sizeof(float));

		// advance
		sample_pos += samples_to_copy;
		sample_count -= samples_to_copy;
		output += samples_to_copy;
	}
}

bool wav_parse_header(const void* data, size_t data_size, wav_file_desc* res)
{
	if (data == NULL || res == NULL || data_size < 44)
		return false;

	const wav_chunk* chunk = (const wav_chunk*)data;
	if (chunk->id != WAV_FOURCC('R', 'I', 'F', 'F'))
		return false;

	const wav_riff* riff = (const wav_riff*)&chunk[1];
	if (riff->format != WAV_FOURCC('W', 'A', 'V', 'E'))
		return false;

	res->sample_count = 0;
	res->samples_per_block = 0;
	res->block_size = 0;

	const wav_format* format = NULL;

	chunk = (const wav_chunk*)&riff[1];

	while (true)
	{
		if (chunk->id == WAV_FOURCC('f', 'm', 't', ' '))
		{
			format = (const wav_format*)&chunk[1];
			if (format->format == 0x11 && chunk->size == 20) // IMA ADPCM
			{
				res->samples_per_block = ((const uint16_t*)&format[1])[1];
			}
		}
		else if (chunk->id == WAV_FOURCC('f', 'a', 'c', 't'))
		{
			if (chunk->size != 4)
				return false;
			res->sample_count = *(const uint32_t*)&chunk[1];
		}
		else if (chunk->id == WAV_FOURCC('d', 'a', 't', 'a'))
		{
			res->sample_data_size = chunk->size;
			break;
		}

		chunk = (const wav_chunk*)((const uint8_t*)&chunk[1] + chunk->size);
	}
	if (format == NULL)
		return false;

	res->sample_data = &chunk[1];
	res->sample_rate = format->sample_rate;
	res->channel_count = format->channel_count;
	res->sample_format = format->format;
	res->block_size = format->block_size;

	if (res->sample_count == 0) {
		// no 'fact' chunk, calculate sample count manually
		if (res->block_size != 0 && res->samples_per_block != 0)
		{
			uint32_t num_blocks = res->sample_data_size / res->block_size + 1;
			res->sample_count = res->samples_per_block * num_blocks;
		}
		else
		{
			res->sample_count = (uint64_t)(res->sample_data_size / res->channel_count) * 8 / format->bits_per_sample;
		}
	}

	return true;
}
