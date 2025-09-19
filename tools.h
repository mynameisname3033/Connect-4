#pragma once

#include <cstdint>

extern const int CELL_WEIGHTS[6][7];
extern uint64_t BUCKET_MASKS[5];

void init_cell_data();
void init_buckets();

extern const uint64_t FULL_MASK;
extern const uint64_t COL_MASKS[7];

uint64_t hash_board(uint64_t bb1, uint64_t bb2);