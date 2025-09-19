#include <iostream>
#include <unordered_map>
#include <chrono>
#include <random>
#include <intrin.h>

#include "bot.h"
#include "tools.h"

using namespace std;

struct TT_entry
{
	int value;
	int best_move;
	int depth_remaining;
	uint8_t bound;
};

static unordered_map<uint64_t, TT_entry> transposition_table;

constexpr float MAX_SEARCH_TIME = 2;
constexpr int MAX_DEPTH = 42;
constexpr int MAX_INT = numeric_limits<int>::max();
constexpr int MIN_INT = -MAX_INT;

static inline int check_fork(uint64_t two_in_row, uint64_t empties, int shift)
{
	uint64_t left_open = (two_in_row << shift) & empties;
	uint64_t right_open = (two_in_row >> shift) & empties;
	uint64_t both_open = left_open & right_open;
	return 100 * __popcnt64(both_open);
}

static int get_threat_score(uint64_t bb1, uint64_t bb2)
{
	uint64_t empties = ~(bb1 | bb2);
	int score = 0;

	uint64_t h2 = bb1 & (bb1 >> 7);
	uint64_t h3 = h2 & (bb1 >> 14);
	score += 10 * __popcnt64((h2 << 7 & empties) | (h2 >> 7 & empties));
	score += 50 * __popcnt64((h3 << 7 & empties) | (h3 >> 7 & empties));

	uint64_t v2 = bb1 & (bb1 >> 1);
	uint64_t v3 = v2 & (bb1 >> 2);
	score += 10 * __popcnt64((v2 << 1 & empties) | (v2 >> 1 & empties));
	score += 50 * __popcnt64((v3 << 1 & empties) | (v3 >> 1 & empties));

	uint64_t d2 = bb1 & (bb1 >> 6);
	uint64_t d3 = d2 & (bb1 >> 12);
	score += 10 * __popcnt64((d2 << 6 & empties) | (d2 >> 6 & empties));
	score += 50 * __popcnt64((d3 << 6 & empties) | (d3 >> 6 & empties));

	uint64_t a2 = bb1 & (bb1 >> 8);
	uint64_t a3 = a2 & (bb1 >> 16);
	score += 10 * __popcnt64((a2 << 8 & empties) | (a2 >> 8 & empties));
	score += 50 * __popcnt64((a3 << 8 & empties) | (a3 >> 8 & empties));

	score += check_fork(h2, empties, 7);
	score += check_fork(v2, empties, 1);
	score += check_fork(d2, empties, 6);
	score += check_fork(a2, empties, 8);

	return score;
}

static int bot_evaluate_board(board& game_board, int depth, int endgame)
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

	uint64_t bb_x = game_board.bb_x;
	uint64_t bb_o = game_board.bb_o;

	int center_score = 0;
	for (int b = 0; b < 5; b++)
	{
		int w = 3 + b * 3;
		center_score += w * __popcnt64(bb_o & BUCKET_MASKS[b]);
		center_score -= w * __popcnt64(bb_x & BUCKET_MASKS[b]);
	}
	int moves_played = game_board.num_moves;
	float phase = 1.0f - (moves_played / 42.0f);
	score += (int)(center_score * phase * 500);

	score += get_threat_score(bb_o, bb_x);
	score -= get_threat_score(bb_x, bb_o);

	return score;
}

static int bot_minimax_recursive(board& game_board, int depth, bool is_maximizing, int alpha, int beta, int depth_limit)
{
	int endgame = game_board.check_endgame();
	if (depth >= depth_limit || endgame != 0)
	{
		return bot_evaluate_board(game_board, depth, endgame);
	}

	int remaining = depth_limit - depth;
	uint64_t hash = hash_board(game_board.bb_x, game_board.bb_o);
	int move_order[7] = { 3, 2, 4, 1, 5, 0, 6 };

	auto it = transposition_table.find(hash);
	if (it != transposition_table.end())
	{
		const TT_entry& e = it->second;

		if (e.depth_remaining >= remaining)
		{
			if (e.bound == 0 ||
				(e.bound == 1 && e.value >= beta) ||
				(e.bound == 2 && e.value <= alpha))
			{
				return e.value;
			}
		}

		if (e.best_move != -1)
		{
			for (int i = 0; i < 7; ++i)
			{
				if (move_order[i] == e.best_move)
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
			if (game_board.place_piece(false, col))
			{
				int score = bot_minimax_recursive(game_board, depth + 1, false, alpha, beta, depth_limit);
				game_board.remove_piece(false, col);

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
			if (game_board.place_piece(true, col))
			{
				int score = bot_minimax_recursive(game_board, depth + 1, true, alpha, beta, depth_limit);
				game_board.remove_piece(true, col);

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

void bot_turn(board& game_board)
{
	auto start = chrono::high_resolution_clock::now();
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

		int root_moves[7] = {};
		for (int i = 0; i < 7; ++i)
			root_moves[i] = move_order[i];

		/*if (pv_move != -1)
		{
			for (int i = 0; i < 7; ++i)
			{
				if (root_moves[i] == pv_move)
				{
					swap(root_moves[0], root_moves[i]);
					break;
				}
			}
		}*/

		for (int col : root_moves)
		{
			if (!game_board.is_full(col))
			{
				game_board.place_piece(false, col);
				int score = bot_minimax_recursive(game_board, 1, false, MIN_INT, MAX_INT, depth);
				game_board.remove_piece(false, col);

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
		game_board.place_piece(false, final_best_col);
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