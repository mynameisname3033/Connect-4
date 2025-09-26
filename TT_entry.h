#pragma once

#include <cstdint>

struct TT_entry
{
	uint64_t key;
	int value;
	int best_move;
	int depth_remaining;
	uint8_t bound;
};

uint64_t hash_board(uint64_t bb1, uint64_t bb2);