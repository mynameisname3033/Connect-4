#pragma once

struct board
{
	uint64_t bb_x;
	uint64_t bb_o;
	int num_moves;
	int heights[7];

	private:
		bool is_winning(uint64_t bb) const;
		
	public:
		board();
		void print() const;
		bool place_piece(bool is_x, int col);
		void remove_piece(bool is_x, int col);
		int check_endgame() const;
		bool is_full(int col) const;
};