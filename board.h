#pragma once

extern uint64_t bb_x;
extern uint64_t bb_o;
extern int num_moves;
extern int heights[7];

void print();
bool place_piece(bool is_x, int col);
void remove_piece(bool is_x, int col);
int check_endgame();
bool is_winning(uint64_t bb);
bool is_full(int col);