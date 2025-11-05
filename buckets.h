#pragma once

#include <cstdint>

extern uint64_t BUCKET_MASKS[4];
extern int BUCKET_WEIGHTS[4];

extern inline void init_buckets();