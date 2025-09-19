#pragma once

struct board
{
	uint64_t bb_x = 0;
	uint64_t bb_o = 0;

	int heights[7] = { 0, 7, 14, 21, 28, 35, 42 };
	int num_moves = 0;

	private:
		bool is_winning(uint64_t bb) const;

	public:
		void print() const;
		bool place_piece(bool is_x, int col);
		void remove_piece(bool is_x, int col);
		int check_endgame() const;
		bool is_full(int col) const;
};