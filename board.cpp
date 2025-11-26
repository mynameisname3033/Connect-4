#include <iostream>
#include <cstdlib>
#include <ctime>
#include "board.h"

using namespace std;

static uint64_t FULL_BOARD_MASK = 0;
static constexpr uint64_t TOP_MASK[7] =
{
	1ULL << 5,
	1ULL << 12,
	1ULL << 19,
	1ULL << 26,
	1ULL << 33,
	1ULL << 40,
	1ULL << 47
};

uint64_t bb_x = 0;
uint64_t bb_o = 0;
int num_moves = 0;
static int heights[7] = { 0, 7, 14, 21, 28, 35, 42 };

void init_full_board_mask()
{
	for (int col = 0; col < 7; ++col)
	{
		for (int row = 0; row < 6; ++row)
		{
			int index = col * 7 + row;
			FULL_BOARD_MASK |= (1ULL << index);
		}
	}
}

void print_board()
{
	for (int row = 5; row >= 0; --row)
	{
		cout << "|";
		for (int col = 0; col < 7; ++col)
		{
			int index = col * 7 + row;
			uint64_t bit = 1ULL << index;
			if (bb_o & bit)
			{
				cout << "\033[1;31mO\033[0m|";
			}
			else if (bb_x & bit)
			{
				cout << "\033[1;34mX\033[0m|";
			}
			else
			{
				cout << "\033[0m |";
			}
		}
		cout << endl;
	}

	cout << "\033[0m" << endl;
}

bool place_piece(bool is_x, int col)
{
	if (col_is_full(col)) return false;

	if (is_x)
		bb_x |= (1ULL << heights[col]);
	else
		bb_o |= (1ULL << heights[col]);

	++heights[col];
	++num_moves;

	return true;
}

void remove_piece(bool is_x, int col)
{
	--heights[col];
	--num_moves;

	if (is_x)
		bb_x ^= (1ULL << heights[col]);
	else
		bb_o ^= (1ULL << heights[col]);
}

void reset_board()
{
	bb_x = 0;
	bb_o = 0;
	num_moves = 0;
	for (int col = 0; col < 7; ++col)
	{
		heights[col] = col * 7;
	}
}

static bool is_winning(uint64_t bb)
{
	uint64_t m = bb & (bb >> 7);
	if (m & (m >> 14)) return true;

	m = bb & (bb >> 1);
	if (m & (m >> 2)) return true;

	m = bb & (bb >> 6);
	if (m & (m >> 12)) return true;

	m = bb & (bb >> 8);
	if (m & (m >> 16)) return true;

	return false;
}

inline bool col_is_full(int col)
{
	return (bb_x | bb_o) & TOP_MASK[col];
}

int check_endgame()
{
	if (is_winning(bb_x)) return 1;
	if (is_winning(bb_o)) return -1;
	if ((bb_x | bb_o) == FULL_BOARD_MASK) return 2;
	return 0;
}

static inline uint64_t splitmix64(uint64_t x)
{
	x += 0x9e3779b97f4a7c15ULL;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
	return x ^ (x >> 31);
}

uint64_t hash_board()
{
	uint64_t h1 = splitmix64(bb_x + 0x9e3779b97f4a7c15ULL);
	uint64_t h2 = splitmix64(bb_o + 0x6a09e667f3bcc909ULL);
	return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
}