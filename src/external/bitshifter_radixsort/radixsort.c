/*
 * Copyright (c) 2014 Cameron Hart
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include "radixsort.h"

#define kRadixBits 8
#define kHistBuckets (1 + (((sizeof(uint32_t) * 8) - 1) / kRadixBits))
#define kHistSize (1 << kRadixBits)

/**
  * Initialise each histogram bucket with the key value
  */
static void init_histograms_u32(uint32_t* restrict hist, const uint32_t* restrict keys_in, uint32_t size)
{
    const uint32_t kHistMask = kHistSize - 1;
    for (uint32_t i = 0; i < size; ++i)
    {
        const uint32_t key = keys_in[i];
        for (uint32_t bucket = 1; bucket < kHistBuckets; ++bucket)
        {
            const uint32_t shift = bucket * kRadixBits;
            const uint32_t pos = (key >> shift) & kHistMask;
            uint32_t* offset = hist + (bucket * kHistSize);
            ++offset[pos];
        }
    }
}

/**
 * Update the histogram data so each entry sums the previous entries.
 */
static void sum_histograms(uint32_t* restrict hist)
{
    uint32_t sum[kHistBuckets];
    for (uint32_t bucket = 1; bucket < kHistBuckets; ++bucket)
    {
        uint32_t* restrict offset = hist + (bucket * kHistSize);
        sum[bucket] = offset[0];
        offset[0] = 0;
    }

    uint32_t tsum;
    for (uint32_t i = 1; i < kHistSize; ++i)
    {
        for (uint32_t bucket = 1; bucket < kHistBuckets; ++bucket)
        {
            uint32_t* restrict offset = hist + (bucket * kHistSize);
            tsum = offset[i] + sum[bucket];
            offset[i] = sum[bucket];
            sum[bucket] = tsum;
        }
    }
}


/**
 * Perform a radix sort pass for the given bit shift and mask.
 */
static inline void radixpass_u32(uint32_t* restrict hist, uint32_t shift, uint32_t mask,
    const uint32_t* restrict keys_in, uint32_t* restrict keys_out, uint32_t size)
{
    for (uint32_t i = 0; i < size; ++i)
    {
        const uint32_t key = keys_in[i];
        const uint32_t pos = (key >> shift) & mask;
        const uint32_t index = hist[pos]++;
        keys_out[index] = key;
    }
}

static inline uint32_t radixsort_u32(uint32_t* restrict keys_in, uint32_t* restrict keys_temp, uint32_t size)
{
    uint32_t hist[kHistBuckets * kHistSize];
    memset(hist, 0, sizeof(uint32_t) * kHistBuckets * kHistSize);

    init_histograms_u32(hist, keys_in, size);

    sum_histograms(hist);

    // alternate input and output buffers on each radix pass
    uint32_t* restrict keys[2] = {keys_temp, keys_in};

    uint32_t out = 0;
    const uint32_t kHistMask = kHistSize - 1;
    for (uint32_t bucket = 1; bucket < kHistBuckets; ++bucket)
    {
        const uint32_t in = bucket & 1;
        out = !in;
        uint32_t* restrict offset = hist + (bucket * kHistSize);
        radixpass_u32(offset, bucket * kRadixBits, kHistMask, keys[in], keys[out], size);
    }

    return out;
}


uint32_t radix8sort_u32(uint32_t* restrict keys_in_out, uint32_t* restrict keys_temp, uint32_t size)
{
    return radixsort_u32(keys_in_out, keys_temp, size);
}
