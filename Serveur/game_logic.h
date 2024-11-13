//
// Created by felix on 06.11.2024.
//

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <sys/types.h>
#include <unistd.h> /* close */

#define NUM_PITS 6
#define INITIAL_SEEDS 4

// Struct for game state
typedef struct {
    int pits[2][NUM_PITS];       // Each player has 6 pits
    int player_score[2];          // Player scores
    int current_turn;             // 0 for player 1, 1 for player 2
} GameState;

// Function prototypes
void distribute_remaining_seeds(GameState *game);
int has_seeds(GameState *game, int player);
void initialize_game(GameState *game);
void board_in_buffer(const GameState *game, char *buffer, size_t size);
int make_move(GameState *game, int player, int pit_index);
int check_game_end(GameState *game);
int determine_winner(GameState *game);

#endif
