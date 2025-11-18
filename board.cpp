#include <iostream>
#include "board.h"

using namespace std;

uint64_t FULL_BOARD_MASK = 0;

uint64_t bb_x = 0;
uint64_t bb_o = 0;
uint8_t num_moves = 0;
uint8_t heights[7] = { 0, 7, 14, 21, 28, 35, 42 };

void init_full_board_mask()
{
	for (int8_t col = 0; col < 7; ++col)
	{
		for (int8_t row = 0; row < 6; ++row)
		{
			uint8_t index = col * 7 + row;
			FULL_BOARD_MASK |= (1ULL << index);
		}
	}
}

void print_board()
{
	for (int8_t row = 5; row >= 0; --row)
	{
		cout << "|";
		for (int8_t col = 0; col < 7; ++col)
		{
			uint8_t index = col * 7 + row;
			uint64_t bit = 1ULL << index;
			if (bb_o & bit)
			{
				cout << "\033[1;31mO\033[39m|";
			}
			else if (bb_x & bit)
			{
				cout << "\033[1;34mX\033[39m|";
			}
			else
			{
				cout << "\033[39m |";
			}
		}
		cout << endl;
	}

	cout << "\033[0m" << endl;
}

bool place_piece(bool is_x, uint8_t col)
{
	if (is_full(col)) return false;

	if (is_x)
		bb_x |= (1ULL << heights[col]);
	else
		bb_o |= (1ULL << heights[col]);

	++heights[col];
	++num_moves;

	return true;
}

void remove_piece(bool is_x, uint8_t col)
{
	--heights[col];
	--num_moves;

	if (is_x)
		bb_x ^= (1ULL << heights[col]);
	else
		bb_o ^= (1ULL << heights[col]);
}

int8_t check_endgame()
{
	if (is_winning(bb_x)) return 1;
	if (is_winning(bb_o)) return -1;
	if ((bb_x | bb_o) == FULL_BOARD_MASK) return 2;
	return 0;
}

bool is_winning(uint64_t bb)
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

bool is_full(uint8_t col)
{
	return heights[col] == (col + 1) * 7 - 1;
}