//
// Created by felix on 06.11.2024.
//

#include "game_logic.h"

#include <stdio.h>

// Initialize the game state
void initialize_game(GameState *game) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < NUM_PITS; j++) {
            game->pits[i][j] = INITIAL_SEEDS; // Start with 4 seeds per pit
        }
    }
    game->player_score[0] = 0;
    game->player_score[1] = 0;
    game->current_turn = 0; // Player 1 starts
}

void board_in_buffer(const GameState *game, char *buffer, size_t size) {
    snprintf(buffer, size, "Joueur 2\n");
    
    for (int j = NUM_PITS - 1; j >= 0; j--) {
        snprintf(buffer + strlen(buffer), size - strlen(buffer), " %d ", game->pits[1][j]);
    }
    snprintf(buffer + strlen(buffer), size - strlen(buffer), "\n");

    for (int i = 0; i < NUM_PITS; i++) {
        snprintf(buffer + strlen(buffer), size - strlen(buffer), " %d ", game->pits[0][i]);
    }

    snprintf(buffer + strlen(buffer), size - strlen(buffer), "\nJoueur 1\n");
}

// Helper function to check if a player has any seeds left in their pits
int has_seeds(GameState *game, int player) {
    for (int i = 0; i < NUM_PITS; i++) {
        if (game->pits[player][i] > 0) {
            return 1; // Player has seeds
        }
    }
    return 0; // No seeds
}

// Process a move by a player, ensuring it doesn't starve the opponent
int make_move(GameState *game, int player, int pit_index) {
    if (pit_index < 0 || pit_index >= NUM_PITS || game->pits[player][pit_index] == 0) {
        return -1; // Invalid move
    }

    int seeds = game->pits[player][pit_index];
    game->pits[player][pit_index] = 0; // Empty the selected pit

    int row = player;
    int pos = pit_index;

    // Distribute seeds in a counter-clockwise manner
    while (seeds > 0) {
        pos++;
        if (pos == NUM_PITS) {
            pos = 0;
            row = 1 - row; // Switch rows
        }

        if (!(row == player && pos == pit_index)) { // Skip the starting pit
            game->pits[row][pos]++;
            seeds--;
        }
    }

    // Check if this move would starve the opponent
    if (!has_seeds(game, 1 - player)) {
        // If opponent has no seeds, undo the move and return an error
        game->pits[player][pit_index] += seeds;
        return -2; // Invalid move, would starve opponent
    }

    // Update turn
    game->current_turn = 1 - game->current_turn;
    return 0; // Move successful
}
// Distribute any remaining seeds to the players when the game ends
void distribute_remaining_seeds(GameState *game) {
    for (int i = 0; i < NUM_PITS; i++) {
        game->player_score[0] += game->pits[0][i];
        game->pits[0][i] = 0;

        game->player_score[1] += game->pits[1][i];
        game->pits[1][i] = 0;
    }
}

// Check if the game has ended and distribute remaining seeds if necessary
int check_game_end(GameState *game) {
    int player1_seeds = has_seeds(game, 0);
    int player2_seeds = has_seeds(game, 1);

    // If one of the players has no seeds and no valid move, end the game
    if (!player1_seeds || !player2_seeds) {
        distribute_remaining_seeds(game);
        return 1; // Game ended
    }

    return 0; // Game continues
}
// Determine the winner based on player scores
int determine_winner(GameState *game) {
    if (game->player_score[0] > game->player_score[1]) {
        return 0; // Player 1 wins
    } else if (game->player_score[1] > game->player_score[0]) {
        return 1; // Player 2 wins
    } else {
        return -1; // Tie
    }
}

// int main() {
//     GameState game;
//     initialize_game(&game);
    
//     printf("Choisissez le sens de rotation (1 pour anti-horaire, 2 pour horaire) : ");
//     int sensRotation;
//     scanf("%d", &sensRotation);
    
//     printf("-------------------------------------------------------------\n");

//     while (!check_game_end(&game) && has_seeds(&game, 0) && has_seeds(&game, 1)) {
//         print_board(&game);
        
//         int joueur = game.current_turn + 1;
//         printf("Joueur %d, choisissez un trou (0-5 pour joueur 1, 6-11 pour joueur 2): ", joueur);
//         int trou;
//         scanf("%d", &trou);

//         int result = make_move(&game, game.current_turn, trou);
//         if (result == -1) {
//             printf("Coup invalide. Essayez de nouveau.\n");
//         } else if (result == -2) {
//             printf("Coup invalide, cela affamerait l'adversaire. Essayez de nouveau.\n");
//         }

//         printf("-------------------------------------------------------------\n");
//     }

//     print_board(&game);

//     int winner = determine_winner(&game);
//     if (winner == 0) {
//         printf("Joueur 1 gagne!\n");
//     } else if (winner == 1) {
//         printf("Joueur 2 gagne!\n");
//     } else {
//         printf("Match nul!\n");
//     }

//     return 0;
// }