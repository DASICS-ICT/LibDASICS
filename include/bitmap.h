#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Bitmap allocation function
static inline uint8_t *bitmap_alloc(size_t bits) {
    size_t bytes = (bits + 7) / 8;  // Calculate required bytes
    return (uint8_t *)calloc(bytes, sizeof(uint8_t));  // Initialize to 0 using calloc
}

// Query if a bit is set to 1 (returns true for 1, false for 0)
static inline bool bitmap_query(const uint8_t *bitmap, size_t pos) {
    if (!bitmap) return false; // Safety check
    size_t byte_idx = pos / 8;
    uint8_t bit_mask = 1 << (pos % 8);
    return (bitmap[byte_idx] & bit_mask) != 0;
}

// Set a bit to 1 or 0
static inline void bitmap_set(uint8_t *bitmap, size_t pos, bool value) {
    if (!bitmap) return; // Safety check
    size_t byte_idx = pos / 8;
    uint8_t bit_mask = 1 << (pos % 8);

    if (value) {
        bitmap[byte_idx] |= bit_mask;  // Set to 1
    } else {
        bitmap[byte_idx] &= ~bit_mask; // Set to 0
    }
}

// Free bitmap memory
static inline void bitmap_free(uint8_t *bitmap) {
    free(bitmap);
}

#endif // __BITMAP_H__