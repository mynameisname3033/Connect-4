#pragma once

extern uint64_t bb_x;
extern uint64_t bb_o;
extern uint8_t num_moves;
extern uint8_t heights[7];

void print_board();
bool place_piece(bool is_x, uint8_t col);
void remove_piece(bool is_x, uint8_t col);
int8_t check_endgame();
bool is_winning(uint64_t bb);
bool is_full(uint8_t col);