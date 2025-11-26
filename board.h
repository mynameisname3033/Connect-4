#pragma once

extern uint64_t bb_x;
extern uint64_t bb_o;
extern int num_moves;

void init_full_board_mask();

void print_board();
void reset_board();

bool place_piece(bool is_x, int col);
void remove_piece(bool is_x, int col);

inline bool col_is_full(int col);
int check_endgame();

uint64_t hash_board();