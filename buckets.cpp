#include <stdlib.h>
#include "buckets.h"

static constexpr int CELL_WEIGHTS[49] =
{
    3, 4, 5, 5, 4, 3, 0,
	4, 6, 8, 8, 6, 4, 0,
	5, 8, 10, 10, 8, 5, 0,
	7, 11, 13, 13, 11, 7, 0,
	5, 8, 10, 10, 8, 5, 0,
	4, 6, 8, 8, 6, 4, 0,
	3, 4, 5, 5, 4, 3, 0
};

uint64_t BUCKET_MASKS[4] = {};
int BUCKET_WEIGHTS[4] = {};

inline void init_buckets()
{
    for (int b = 0; b < 4; ++b)
    {
        BUCKET_MASKS[b] = 0;
        BUCKET_WEIGHTS[b] = 0;
    }

    for (int col = 0; col < 7; ++col)
    {
        int bucket = abs(col - 3);
        for (int row = 0; row < 6; ++row)
        {
            int idx = col * 7 + row;
            BUCKET_MASKS[bucket] |= (1ULL << idx);
            BUCKET_WEIGHTS[bucket] += CELL_WEIGHTS[idx];
        }
    }
}