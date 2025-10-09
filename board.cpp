#include <iostream>
#include "board.h"
#include "buckets.h"

using namespace std;

uint64_t bb_x = 0;
uint64_t bb_o = 0;
int num_moves = 0;
int heights[7] = { 0, 7, 14, 21, 28, 35, 42 };

void print_board()
{
	for (int row = 5; row >= 0; --row)
	{
		cout << "|";
		for (int col = 0; col < 7; ++col)
		{
			int index = col * 7 + row;
			if (bb_o & (1ULL << index))
			{
				cout << "\033[1;31mO\033[39m|";
			}
			else if (bb_x & (1ULL << index))
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

bool place_piece(bool is_x, int col)
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

void remove_piece(bool is_x, int col)
{
	--heights[col];
	--num_moves;

	if (is_x)
		bb_x ^= (1ULL << heights[col]);
	else
		bb_o ^= (1ULL << heights[col]);
}

int check_endgame()
{
	if (is_winning(bb_x)) return 1;
	if (is_winning(bb_o)) return -1;
	if ((bb_x | bb_o) == FULL_MASK) return 2;
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

bool is_full(int col)
{
	return heights[col] == (col + 1) * 7 - 1;
}