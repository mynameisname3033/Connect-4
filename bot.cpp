#include <iostream>
#include <unordered_map>
#include <chrono>
#include <random>
#include <array>

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

static constexpr int LINE_VALUES[5] =
{
	0,
	5,
	15,
	150,
	100000000
};

static constexpr float MAX_SEARCH_TIME = .1;
static constexpr int MAX_DEPTH = 42;
static constexpr int MAX_INT = numeric_limits<int>::max();
static constexpr int MIN_INT = -MAX_INT;

static unordered_map<uint64_t, TT_entry> transposition_table;
static bool tt_reserved = false;

static float center_evaluation_phases[43];
static uint64_t lines[69];

void bot::reset()
{
	transposition_table.clear();
	tt_reserved = false;
}

void bot::init_center_evaluation_phases()
{
	for (int i = 0; i <= 42; ++i)
	{
		center_evaluation_phases[i] = 1.0f - (i / 42.0f);
	}
}

void bot::init_4_in_a_row_lines()
{
	const int dirs[4][2] =
	{
		{0, 1},
		{1, 0},
		{1, 1},
		{-1, 1}
	};

	int line = 0;
	for (int r = 0; r < 6; ++r)
	{
		for (int c = 0; c < 7; ++c)
		{
			for (auto& d : dirs)
			{
				int dr = d[0], dc = d[1];

				int r3 = r + dr * 3;
				int c3 = c + dc * 3;

				if (r3 < 0 || r3 >= 6 || c3 < 0 || c3 >= 7)
					continue;

				uint64_t mask = 0;
				for (int k = 0; k < 4; ++k)
				{
					int rr = r + dr * k;
					int cc = c + dc * k;
					int index = cc * 7 + rr;
					mask |= (1ULL << index);
				}
				lines[line++] = mask;
			}
		}
	}
}

static int board_heuristic()
{
	int score = 0;

	{
		uint64_t x = bb_x;
		while (x)
		{
			int idx = _tzcnt_u64(x);
			score -= CELL_WEIGHTS[idx];
			x &= x - 1;
		}

		uint64_t o = bb_o;
		while (o)
		{
			int idx = _tzcnt_u64(o);
			score += CELL_WEIGHTS[idx];
			o &= o - 1;
		}

		score = static_cast<int>(score * center_evaluation_phases[num_moves]);
	}

	{
		for (uint64_t& mask : lines)
		{
			uint64_t xb = bb_x & mask;
			uint64_t ob = bb_o & mask;

			if (xb && ob) continue;

			int nx = __popcnt64(xb);
			int no = __popcnt64(ob);

			if (nx) score -= LINE_VALUES[nx];
			else if (no) score += LINE_VALUES[no];
		}
	}

	return score;
}

static int minimax(int depth, bool is_maximizing, int alpha, int beta, int depth_limit)
{
	int endgame = check_endgame();
	if (depth >= depth_limit || endgame != 0)
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

		return board_heuristic();
	}

	int depth_remaining = depth_limit - depth;
	uint64_t hash = hash_board();
	int move_order[7] = { 3, 2, 4, 1, 5, 0, 6 };

	auto it = transposition_table.find(hash);
	if (it != transposition_table.end())
	{
		const TT_entry& entry = it->second;

		if (entry.depth_remaining >= depth_remaining)
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
			for (int col = 0; col < 7; ++col)
			{
				if (move_order[col] == entry.best_move)
				{
					swap(move_order[0], move_order[col]);
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
				int score = minimax(depth + 1, false, alpha, beta, depth_limit);
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
		entry.depth_remaining = depth_remaining;
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
				int score = minimax(depth + 1, true, alpha, beta, depth_limit);
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
		entry.depth_remaining = depth_remaining;
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

void bot::turn()
{
	auto start = chrono::high_resolution_clock::now();

	if (!tt_reserved)
	{
		transposition_table.reserve(20000000);
		tt_reserved = true;
	}

	static mt19937 rng(random_device{}());

	vector<int> best_cols;
	int final_best_col = -1;
	int depth;
	int best_score_at_depth;

	for (depth = 0; depth < min(MAX_DEPTH, 42 - num_moves); ++depth)
	{
		best_cols.clear();

		vector<pair<int, int>> move_order;
		move_order.reserve(7);

		if (final_best_col != -1 && !col_is_full(final_best_col))
		{
			move_order.emplace_back(1000000, final_best_col);
		}

		int tt_best_move = -1;
		uint64_t hash = hash_board();
		auto it = transposition_table.find(hash);
		if (it != transposition_table.end())
		{
			const TT_entry& e = it->second;
			tt_best_move = e.best_move;
		}

		if (tt_best_move != -1 && !col_is_full(tt_best_move) && (move_order.empty() || move_order[0].second != tt_best_move))
		{
			move_order.push_back({ 999999, tt_best_move });
		}

		for (int col = 0; col < 7; ++col)
		{
			if (col == tt_best_move || !move_order.empty() && move_order[0].second == col)
				continue;

			if (place_piece(false, col))
			{
				int score = board_heuristic();
				remove_piece(false, col);
				move_order.push_back({ score, col });
			}
		}

		sort(move_order.begin(), move_order.end(), greater<>());

		best_score_at_depth = MIN_INT;
		for (pair<int, int>& pair : move_order)
		{
			int col = pair.second;

			if (place_piece(false, col))
			{
				int score = minimax(0, false, MIN_INT, MAX_INT, depth);
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
		cerr << "Bot1 could not find a valid move." << endl;
		exit(1);
	}

	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double> duration = end - start;
	cout << "Bot1 played column " << (final_best_col + 1) << " in " << duration.count() << " seconds (depth " << depth << ")" << endl;
	cout << "Transposition table size: " << transposition_table.size() << endl;
	cout << "Heuristic evaluation: " << board_heuristic() << endl;
	cout << "Minimax evaluation: " << best_score_at_depth;

	int moves_until_loss = 100000000 - best_score_at_depth;
	int moves_until_win = best_score_at_depth + 100000000;
	if (moves_until_loss <= 50)
		cout << " (forced loss)";
	else if (moves_until_win <= 50)
		cout << " (forced win)";

	cout << endl;
}