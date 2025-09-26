#include <iostream>
#include <unordered_map>
#include <chrono>
#include <random>
#include <intrin.h>

#include "board.h"
#include "bot.h"
#include "buckets.h"
#include "TT_entry.h"

#ifdef _MSC_VER
	static inline int popcount64(uint64_t x) { return (int)__popcnt64(x); }
	static inline unsigned lsb_index(uint64_t x) { return (unsigned)_tzcnt_u64(x); }
#else
	static inline int popcount64(uint64_t x) { return __builtin_popcountll(x); }
	static inline unsigned lsb_index(uint64_t x) { return (unsigned)__builtin_ctzll(x); }
#endif

using namespace std;

static unordered_map<uint64_t, TT_entry> transposition_table;

constexpr float MAX_SEARCH_TIME = 1;
constexpr int MAX_DEPTH = 42;
constexpr int MAX_INT = numeric_limits<int>::max();
constexpr int MIN_INT = -MAX_INT;

static inline int check_fork(uint64_t two_in_row, uint64_t empties, int shift, int heights[7])
{
	uint64_t left_open = (two_in_row << shift) & empties;
	uint64_t right_open = (two_in_row >> shift) & empties;
	uint64_t both_open = left_open & right_open;

	int score = 0;
	uint64_t mask = both_open;
	while (mask)
	{
		int idx = lsb_index(mask);
		mask &= (mask - 1);

		int col = idx / 7;
		int row = idx % 7;

		if (row >= 6)
			continue;

		int needed = row - heights[col];
		score += 120 - (needed * 20);
	}
	return score;
}

static int get_threat_score(uint64_t bb1, uint64_t bb2, int heights[7])
{
	uint64_t empties = ~(bb1 | bb2);
	int score = 0;

	uint64_t h2 = bb1 & (bb1 >> 7);
	uint64_t h3 = h2 & (bb1 >> 14);
	score += 10 * popcount64((h2 << 7 & empties) | (h2 >> 7 & empties));
	score += 30 * popcount64((h3 << 7 & empties) | (h3 >> 7 & empties));

	uint64_t v2 = bb1 & (bb1 >> 1);
	uint64_t v3 = v2 & (bb1 >> 2);
	score += 10 * popcount64((v2 << 1 & empties) | (v2 >> 1 & empties));
	score += 30 * popcount64((v3 << 1 & empties) | (v3 >> 1 & empties));

	uint64_t d2 = bb1 & (bb1 >> 6);
	uint64_t d3 = d2 & (bb1 >> 12);
	score += 10 * popcount64((d2 << 6 & empties) | (d2 >> 6 & empties));
	score += 30 * popcount64((d3 << 6 & empties) | (d3 >> 6 & empties));

	uint64_t a2 = bb1 & (bb1 >> 8);
	uint64_t a3 = a2 & (bb1 >> 16);
	score += 10 * popcount64((a2 << 8 & empties) | (a2 >> 8 & empties));
	score += 30 * popcount64((a3 << 8 & empties) | (a3 >> 8 & empties));

	score += check_fork(h2, empties, 7, heights);
	score += check_fork(v2, empties, 1, heights);
	score += check_fork(d2, empties, 6, heights);
	score += check_fork(a2, empties, 8, heights);

	return score;
}

static int bot_evaluate_board(int depth, int endgame)
{
	if (endgame == -1)
	{
		return 100000000 - depth;
	}
	else if (endgame == 1)
	{
		return -100000000 + depth;
	}
	else if (endgame == 2)
	{
		return 0;
	}

	int score = 0;

	int center_score = 0;
	for (int b = 0; b < 5; b++)
	{
		int w = 3 + b * 3;
		center_score += w * popcount64(bb_o & BUCKET_MASKS[b]);
		center_score -= w * popcount64(bb_x & BUCKET_MASKS[b]);
	}
	int moves_played = num_moves;
	float phase = 1.0f - (moves_played / 42.0f);
	score += (int)(center_score * phase);

	score += get_threat_score(bb_o, bb_x, heights);
	score -= get_threat_score(bb_x, bb_o, heights);

	return score;
}

static int bot_minimax(int depth, bool is_maximizing, int alpha, int beta, int depth_limit)
{
	int endgame = check_endgame();
	if (depth >= depth_limit || endgame != 0)
	{
		return bot_evaluate_board(depth, endgame);
	}

	int remaining = depth_limit - depth;
	uint64_t hash = hash_board(bb_x, bb_o);
	int move_order[7] = { 3, 2, 4, 1, 5, 0, 6 };

	auto it = transposition_table.find(hash);
	if (it != transposition_table.end())
	{
		const TT_entry& entry = it->second;

		if (entry.depth_remaining >= remaining)
		{
			if (entry.bound == 0 ||
				(entry.bound == 1 && entry.value >= beta) ||
				(entry.bound == 2 && entry.value <= alpha))
			{
				return entry.value;
			}
		}
		if (entry.best_move != -1)
		{
			for (int i = 0; i < 7; ++i)
			{
				if (move_order[i] == entry.best_move)
				{
					swap(move_order[0], move_order[i]);
					break;
				}
			}
		}
	}

	int alpha_orig = alpha;
	int beta_orig = beta;

	if (is_maximizing)
	{
		int best_score = MIN_INT;
		int best_col = -1;
		for (int col : move_order)
		{
			if (place_piece(false, col))
			{
				int score = bot_minimax(depth + 1, false, alpha, beta, depth_limit);
				remove_piece(false, col);

				if (score > best_score)
				{
					best_score = score;
					best_col = col;
				}

				alpha = max(alpha, best_score);
				if (beta <= alpha)
					break;
			}
		}

		TT_entry entry = {};
		entry.value = best_score;
		entry.best_move = best_col;
		entry.depth_remaining = remaining;
		if (best_score <= alpha_orig)
			entry.bound = 2;
		else if (best_score >= beta_orig)
			entry.bound = 1;
		else
			entry.bound = 0;
		transposition_table[hash] = entry;

		return best_score;
	}
	else
	{
		int best_score = MAX_INT;
		int best_col = -1;
		for (int col : move_order)
		{
			if (place_piece(true, col))
			{
				int score = bot_minimax(depth + 1, true, alpha, beta, depth_limit);
				remove_piece(true, col);

				if (score < best_score)
				{
					best_score = score;
					best_col = col;
				}

				beta = min(beta, best_score);
				if (beta <= alpha)
					break;
			}
		}

		TT_entry entry = {};
		entry.value = best_score;
		entry.best_move = best_col;
		entry.depth_remaining = remaining;
		if (best_score <= alpha_orig)
			entry.bound = 2;
		else if (best_score >= beta_orig)
			entry.bound = 1;
		else
			entry.bound = 0;
		transposition_table[hash] = entry;

		return best_score;
	}
}

void bot_turn()
{
	auto start = chrono::high_resolution_clock::now();

	if (num_moves == 0)
	{
		place_piece(false, 3);
		auto end = chrono::high_resolution_clock::now();
		chrono::duration<double> duration = end - start;
		cout << "Bot played column " << 3 << " in " << duration.count() << " seconds" << endl;
		cout << "Transposition table size: " << transposition_table.size() << endl;
		return;
	}

	static mt19937 rng(random_device{}());

	int move_order[7] = { 3, 2, 4, 1, 5, 0, 6 };

	int pv_move = -1;
	vector<int> best_cols;
	int final_best_col = -1;
	int depth;

	for (depth = 1; depth <= MAX_DEPTH; ++depth)
	{
		int best_score_at_depth = MIN_INT;
		best_cols.clear();

		vector<pair<int, int>> move_order;

		if (pv_move != -1 && !is_full(pv_move))
		{
			move_order.emplace_back(1000000, pv_move);
		}

		int tt_best_move = -1;
		uint64_t hash = hash_board(bb_x, bb_o);
		auto it = transposition_table.find(hash);
		if (it != transposition_table.end())
		{
			const TT_entry& e = it->second;
			tt_best_move = e.best_move;
		}

		if (tt_best_move != -1 && !is_full(tt_best_move) && tt_best_move != pv_move)
		{
			move_order.emplace_back(999999, tt_best_move);
		}

		for (int col = 0; col < 7; ++col)
		{
			if (!is_full(col))
			{
				place_piece(false, col);
				int score = bot_evaluate_board(0, check_endgame());
				remove_piece(false, col);
				move_order.emplace_back(score, col);
			}
		}

		sort(move_order.begin(), move_order.end(), greater<>());

		for (pair<int, int> pair : move_order)
		{
			int col = pair.second;

			if (!is_full(col))
			{
				place_piece(false, col);
				int score = bot_minimax(1, false, MIN_INT, MAX_INT, depth);
				remove_piece(false, col);

				if (score > best_score_at_depth)
				{
					best_score_at_depth = score;
					best_cols.clear();
					best_cols.push_back(col);
				}
				else if (score == best_score_at_depth)
				{
					best_cols.push_back(col);
				}
			}
		}

		if (!best_cols.empty())
		{
			uniform_int_distribution<int> dist(0, best_cols.size() - 1);
			final_best_col = best_cols[dist(rng)];
			pv_move = best_cols[0];
		}
		else
		{
			break;
		}

		auto current = chrono::high_resolution_clock::now();
		chrono::duration<double> duration = current - start;
		if (duration.count() > MAX_SEARCH_TIME)
			break;
	}

	if (final_best_col != -1)
	{
		place_piece(false, final_best_col);
	}
	else
	{
		cerr << "Bot could not find a valid move." << endl;
		exit(1);
	}

	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double> duration = end - start;
	cout << "Bot played column " << (final_best_col + 1) << " in " << duration.count() << " seconds (depth " << depth << ")" << endl;
	cout << "Transposition table size: " << transposition_table.size() << endl;
}