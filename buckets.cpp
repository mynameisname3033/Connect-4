#include <array>
#include "buckets.h"

using namespace std;

static constexpr float CELL_EFFECT = 4;
static constexpr int CELL_WEIGHTS[6][7] =
{
	{ 3, 4, 5, 7, 5, 4, 3 },
	{ 4, 6, 8, 11, 8, 6, 4 },
	{ 5, 8, 10, 13, 10, 8, 5 },
	{ 5, 8, 10, 13, 10, 8, 5 },
	{ 4, 6, 8, 11, 8, 6, 4 },
	{ 3, 4, 5, 7, 5, 4, 3 }
};

constexpr uint64_t FULL_MASK = (1ULL << 42) - 1;
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