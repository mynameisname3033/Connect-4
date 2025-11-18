#include <iostream>
#include "bot.h"
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
	int8_t endgame = check_endgame();
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

int main()
{
	init_full_board_mask();

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

			bot_turn();
			print_board();
			print_endgame(false);
		}
		else
		{
			bot_turn();
			print_board();
			print_endgame(false);

			player_turn();
			print_endgame(true);
		}
	}
	
	return 0;
}