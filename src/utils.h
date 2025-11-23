#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>


static inline uint32_t get_bits(uint32_t x, int high, int low) {
   return (x >> low) & ((1u << (high - low + 1)) - 1u);
}

static inline int32_t sign_extend(uint32_t val, int bits) {
   uint32_t m = 1 << (bits - 1);
   return (int32_t)((val^m) - m);
}

#endif