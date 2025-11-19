#pragma once

#include <cstdint>

struct TT_entry
{
	uint64_t hash;
	int value;
	uint8_t depth_remaining;
	uint8_t best_move;
	uint8_t bound;
};