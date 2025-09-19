#include <array>
#include "tools.h"

using namespace std;

static constexpr float CELL_EFFECT = 5;
constexpr int CELL_WEIGHTS[6][7] =
{
	{ 3, 4, 5, 7, 5, 4, 3 },
	{ 4, 6, 8, 11, 8, 6, 4 },
	{ 5, 8, 10, 13, 10, 8, 5 },
	{ 5, 8, 10, 13, 10, 8, 5 },
	{ 4, 6, 8, 11, 8, 6, 4 },
	{ 3, 4, 5, 7, 5, 4, 3 }
};

static uint64_t CELL_MASKS[42];
static int CELL_VALUES[42];
uint64_t BUCKET_MASKS[5];

void init_cell_data()
{
	int idx = 0;
	for (int col = 0; col < 7; ++col)
	{
		for (int row = 0; row < 6; ++row)
		{
			int bit = col * 7 + row;
			CELL_MASKS[idx] = 1ULL << bit;
			CELL_VALUES[idx] = CELL_WEIGHTS[row][col] * CELL_EFFECT;
			idx++;
		}
	}
}

void init_buckets()
{
	constexpr int NUM_BUCKETS = 5;
	memset(BUCKET_MASKS, 0, sizeof(BUCKET_MASKS));
	for (int i = 0; i < 42; ++i)
	{
		int w = CELL_VALUES[i];
		int b = (w - 3) / 3;
		if (b < 0) b = 0;
		if (b >= 5) b = 5 - 1;
		BUCKET_MASKS[b] |= CELL_MASKS[i];
	}
}

constexpr uint64_t FULL_MASK = (1ULL << 42) - 1;
constexpr uint64_t COL_MASKS[7] =
{
	0b1111111ULL << (0 * 7),
	0b1111111ULL << (1 * 7),
	0b1111111ULL << (2 * 7),
	0b1111111ULL << (3 * 7),
	0b1111111ULL << (4 * 7),
	0b1111111ULL << (5 * 7),
	0b1111111ULL << (6 * 7)
};

static inline uint64_t splitmix64(uint64_t x)
{
	x += 0x9e3779b97f4a7c15ULL;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
	return x ^ (x >> 31);
}

uint64_t hash_board(uint64_t bb_x, uint64_t bb_o)
{
	uint64_t h1 = splitmix64(bb_x + 0x9e3779b97f4a7c15ULL);
	uint64_t h2 = splitmix64(bb_o + 0x6a09e667f3bcc909ULL);
	return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
}