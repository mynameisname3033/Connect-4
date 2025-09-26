#pragma once

#include <cstdint>

extern uint64_t BUCKET_MASKS[5];
extern const uint64_t FULL_MASK;

void init_cell_data();
void init_buckets();