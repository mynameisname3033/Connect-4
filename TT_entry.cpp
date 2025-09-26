#include "TT_entry.h"

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