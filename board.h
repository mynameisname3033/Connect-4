#pragma once

extern uint64_t bb_x;
extern uint64_t bb_o;
extern uint8_t num_moves;
extern uint8_t heights[7];

void init_full_board_mask();
void print_board();

bool place_piece(bool is_x, int col);
void remove_piece(bool is_x, int col);
void reset_board();

inline bool col_is_full(int col);
int check_endgame();

uint64_t hash_board();