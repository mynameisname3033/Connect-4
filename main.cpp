#include <iostream>
#include <chrono>
#include "bot.h"
#include "bot2.h"
#include "board.h"

using namespace std;

static void player_turn()
{
	cout << "Enter the column: ";
	int col;
	cin >> col;

	if (col < 1 || col > 7 || !place_piece(true, col - 1))
	{
		cout << "Invalid column. Please input a valid unfilled column betweeen 1 and 7." << endl << endl;
		player_turn();
	}
}


static void print_endgame(bool should_print_board)
{
	int endgame = check_endgame();
	if (endgame == 1)
	{
		if (should_print_board)
			print_board();
		cout << "You win!" << endl;
		exit(0);
	}
	else if (endgame == -1)
	{
		if (should_print_board)
			print_board();
		cout << "Bot wins!" << endl;
		exit(0);
	}
	else if (endgame == 2)
	{
		if (should_print_board)
			print_board();
		cout << "It's a tie!" << endl;
		exit(0);
	}
}

static void randomize_board(int min_moves = 5, int max_moves = 15, bool x_first = true)
{
	int moves_to_make = min_moves + (rand() % (max_moves - min_moves + 1));

	for (int i = 0; i < moves_to_make; ++i)
	{
		int legal_cols[7];
		int count = 0;
		for (int col = 0; col < 7; ++col)
		{
			if (!col_is_full(col))
				legal_cols[count++] = col;
		}

		if (count == 0)
			break;

		int col = legal_cols[rand() % count];
		place_piece(x_first, col);

		if (check_endgame() != 0)
		{
			remove_piece(x_first, col);
			break;
		}

		x_first = !x_first;
	}
}

static void bot_test()
{
	int bot1_wins = 0;
	int bot2_wins = 0;
	int ties = 0;

	srand((unsigned)time(nullptr));

	for (int round = 1; round <= 50; ++round)
	{
		reset_board();
		bot::reset();
		bot2::reset();

		bool bot1_turn = round % 2 == 1;

		randomize_board(2, 5, !bot1_turn);
		print_board();

		while (true)
		{
			if (bot1_turn)
				bot::turn();
			else
				bot2::turn();

			int endgame = check_endgame();
			if (endgame == -1)
			{
				bot1_wins++;
				break;
			}
			else if (endgame == 1)
			{
				bot2_wins++;
				break;
			}
			else if (endgame == 2)
			{
				ties++;
				break;
			}

			bot1_turn = !bot1_turn;
			print_board();
		}
	}

	cout << "Bot1 wins: " << bot1_wins << endl;
	cout << "Bot2 wins: " << bot2_wins << endl;
	cout << "Ties: " << ties << endl;
}

int main()
{
	init_full_board_mask();
	bot::init_4_in_a_row_lines();
	bot::init_center_evaluation_phases();
	bot2::init_4_in_a_row_lines();
	bot2::init_center_evaluation_phases();

	string r;
	cout << "Do you want to go first? (y/n) ";
	cin >> r;
	bool player_first = r == "y";
	cout << endl;

	while (true)
	{
		if (player_first)
		{
			player_turn();
			print_endgame(true);

			bot::turn();
			print_board();
			print_endgame(false);
		}
		else
		{
			bot::turn();
			print_board();
			print_endgame(false);

			player_turn();
			print_endgame(true);
		}
	}

	//bot_test();
	
	return 0;
}