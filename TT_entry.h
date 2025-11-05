#pragma once

#include <cstdint>

struct TT_entry
{
	int value;
	uint8_t depth_remaining;
	uint8_t best_move;
	uint8_t bound;
};

uint64_t hash_board(uint64_t bb1, uint64_t bb2);