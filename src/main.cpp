#include "number_merge/game.hpp"

#include <array>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

namespace {

int randomTile(std::mt19937& generator) {
    // Small numbers are deliberately common so the player can plan merges.
    static constexpr std::array<int, 8> tiles{2, 2, 2, 2, 4, 4, 8, 8};
    std::uniform_int_distribution<std::size_t> distribution(0, tiles.size() - 1);
    return tiles[distribution(generator)];
}

void draw(const number_merge::Game& game, int current, int next) {
    std::cout << "\n=================================\n"
              << "          WOOD NUMBER MERGE      \n"
              << "=================================\n"
              << " Score: " << game.score() << "     Tile: " << current
              << "     Next: " << next << "\n\n";

    std::cout << "       1     2     3     4     5\n";
    std::cout << "    +-----+-----+-----+-----+-----+\n";
    for (const auto& row : game.board()) {
        std::cout << "    |";
        for (const int value : row) {
            if (value == 0) {
                std::cout << "     |";
            } else {
                std::cout << std::setw(4) << value << " |";
            }
        }
        std::cout << '\n';
    }
    std::cout << "    +-----+-----+-----+-----+-----+\n"
              << " Drop with 1-5. Commands: h help, r restart, q quit\n";
}

void showHelp() {
    std::cout << "\nHOW TO PLAY\n"
              << "Choose a column from 1 to 5. The shown tile falls to the\n"
              << "lowest empty space. Connect 3 or more equal tiles side-to-side\n"
              << "or vertically to merge them into one tile worth twice as much.\n"
              << "The merged tile can immediately create another match for a\n"
              << "higher-scoring combo. The game ends when every column is full.\n";
}

} // namespace

int main() {
    number_merge::Game game;
    std::random_device device;
    std::mt19937 generator(device());
    int current = randomTile(generator);
    int next = randomTile(generator);
    std::string input;

    showHelp();
    while (true) {
        draw(game, current, next);
        if (game.isGameOver()) {
            std::cout << "Game over! Final score: " << game.score()
                      << ". Enter r to restart or q to quit: ";
        } else {
            std::cout << "> ";
        }

        if (!std::getline(std::cin, input)) {
            break;
        }

        if (input == "q" || input == "Q") {
            break;
        }
        if (input == "h" || input == "H") {
            showHelp();
            continue;
        }
        if (input == "r" || input == "R") {
            game.reset();
            current = randomTile(generator);
            next = randomTile(generator);
            continue;
        }
        if (game.isGameOver()) {
            std::cout << "Restart or quit before making another move.\n";
            continue;
        }

        std::istringstream parser(input);
        int selected_column = 0;
        char extra = '\0';
        if (!(parser >> selected_column) || (parser >> extra) ||
            selected_column < 1 || selected_column > 5) {
            std::cout << "Please enter a column number from 1 to 5.\n";
            continue;
        }

        const auto move = game.place(static_cast<std::size_t>(selected_column - 1), current);
        if (!move.placed) {
            std::cout << "That column is full. Choose another one.\n";
            continue;
        }

        if (move.combo > 0) {
            std::cout << "Merge! +" << move.score_gained;
            if (move.combo > 1) {
                std::cout << " (combo x" << move.combo << ')';
            }
            std::cout << "\n";
        }
        current = next;
        next = randomTile(generator);
    }

    std::cout << "Thanks for playing!\n";
    return 0;
}

