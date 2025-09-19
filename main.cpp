#include <iostream>
#include "bot.h"
#include "tools.h"

using namespace std;

static board game_board;

static void player_turn()
{
	cout << "Enter the column: ";
	int col;
	cin >> col;

	if (col < 1 || col > 7 || !game_board.place_piece(true, col - 1))
	{
		cout << "Invalid column. Please input a valid unfilled column betweeen 1 and 7." << endl << endl;
		player_turn();
	}
}

static void print_endgame(bool print_board)
{
	int endgame = game_board.check_endgame();
	if (endgame == 1)
	{
		if (print_board)
			game_board.print();
		cout << "You win!" << endl;
		exit(0);
	}
	else if (endgame == -1)
	{
		if (print_board)
			game_board.print();
		cout << "Bot wins!" << endl;
		exit(0);
	}
	else if (endgame == 2)
	{
		if (print_board)
			game_board.print();
		cout << "It's a tie!" << endl;
		exit(0);
	}
}

int main()
{
	init_cell_data();
	init_buckets();

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
		}
		else
		{
			bot_turn(game_board);
			game_board.print();
			print_endgame(false);
		}

		if (player_first)
		{
			bot_turn(game_board);
			game_board.print();
			print_endgame(false);
		}
		else
		{
			player_turn();
			print_endgame(true);
		}
	}

	return 0;
}