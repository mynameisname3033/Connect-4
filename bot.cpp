#include <iostream>
#include <unordered_map>
#include <chrono>
#include <random>

#include "board.h"
#include "bot.h"
#include "TT_entry.h"

using namespace std;

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

constexpr float MAX_SEARCH_TIME = 1;
constexpr int MAX_DEPTH = 42;
constexpr int MAX_INT = numeric_limits<int>::max();
constexpr int MIN_INT = -MAX_INT;

static unordered_map<uint64_t, TT_entry> transposition_table;
static bool tt_reserved = false;

static int bot_evaluate_board(uint8_t depth, uint8_t endgame)
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
	for (int i = 0; i < 49; i++)
	{
		uint64_t bit = 1ULL << i;

		if (bb_o & bit)
			center_score += CELL_WEIGHTS[i];
		else if (bb_x & bit)
			center_score -= CELL_WEIGHTS[i];
	}
	float phase = 1.0f - (num_moves / 42.0f);
	score += static_cast<int>(center_score * phase);

	return score;
}

static int bot_minimax(uint8_t depth, bool is_maximizing, int alpha, int beta, uint8_t depth_limit)
{
	int8_t endgame = check_endgame();
	if (depth >= depth_limit || endgame != 0)
	{
		return bot_evaluate_board(depth, endgame);
	}

	uint8_t remaining = depth_limit - depth;
	uint64_t hash = hash_board(bb_x, bb_o);

	uint8_t move_order[7] = { 3, 2, 4, 1, 5, 0, 6 };

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
		int8_t best_col = -1;
		for (uint8_t col : move_order)
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
		int8_t best_col = -1;
		for (uint8_t col : move_order)
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

	if (!tt_reserved)
	{
		transposition_table.reserve(1 << 24);
		tt_reserved = true;
	}

	if (num_moves == 0)
	{
		place_piece(false, 3);
		auto end = chrono::high_resolution_clock::now();
		chrono::duration<double> duration = end - start;
		cout << "Bot played column " << 4 << " in " << duration.count() << " seconds" << endl;
		cout << "Transposition table size: " << transposition_table.size() << endl;
		return;
	}

	static mt19937 rng(random_device{}());

	uint8_t move_order[7] = { 3, 2, 4, 1, 5, 0, 6 };

	int8_t pv_move = -1;
	vector<int> best_cols;
	int8_t final_best_col = -1;
	int depth;

	for (depth = 1; depth <= MAX_DEPTH; ++depth)
	{
		int best_score_at_depth = MIN_INT;
		best_cols.clear();

		vector<pair<int, uint8_t>> move_order;
		move_order.reserve(7);

		if (pv_move != -1 && !is_full(pv_move))
		{
			move_order.emplace_back(1000000, pv_move);
		}

		int8_t tt_best_move = -1;
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

		for (uint8_t col = 0; col < 7; ++col)
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

		for (pair<int, uint8_t> pair : move_order)
		{
			uint8_t col = pair.second;

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
	cout << "Heuristic evaluation: " << bot_evaluate_board(0, check_endgame()) << endl;
}