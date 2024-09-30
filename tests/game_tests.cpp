#include "number_merge/game.hpp"

#include <cstdlib>
#include <iostream>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testTileFalls() {
    number_merge::Game game;
    expect(game.place(2, 2).placed, "a tile can be placed");
    expect(game.board()[number_merge::Game::kRows - 1][2] == 2,
           "a tile falls to the bottom");
}

void testHorizontalMerge() {
    number_merge::Game game;
    game.place(0, 2);
    game.place(1, 2);
    const auto move = game.place(2, 2);

    expect(move.combo == 1, "three horizontal tiles merge");
    expect(move.score_gained == 12, "a three-tile merge awards points");
    expect(game.board()[number_merge::Game::kRows - 1][2] == 4,
           "the most recent tile is upgraded");
    expect(game.board()[number_merge::Game::kRows - 1][0] == 0 &&
               game.board()[number_merge::Game::kRows - 1][1] == 0,
           "the other merged tiles disappear");
}

void testVerticalMergeAndGravity() {
    number_merge::Game game;
    game.place(0, 4);
    game.place(0, 4);
    const auto move = game.place(0, 4);

    expect(move.combo == 1, "three vertical tiles merge");
    expect(game.board()[number_merge::Game::kRows - 1][0] == 8,
           "the upgraded tile falls after a merge");
}

void testCascadeCombo() {
    number_merge::Game game;
    game.place(0, 4);
    game.place(1, 4);
    game.place(2, 2);
    game.place(2, 2);
    const auto move = game.place(2, 2);

    expect(move.combo == 2, "an upgraded tile can trigger a cascade");
    expect(move.score_gained == 60, "later cascade steps use a combo multiplier");
    expect(game.board()[number_merge::Game::kRows - 1][2] == 8,
           "the cascade leaves the twice-upgraded tile");
}

void testInvalidMoves() {
    number_merge::Game game;
    expect(!game.place(number_merge::Game::kColumns, 2).placed,
           "out-of-range columns are rejected");
    expect(!game.place(0, 0).placed, "non-positive tiles are rejected");

    for (std::size_t row = 0; row < number_merge::Game::kRows; ++row) {
        expect(game.place(0, static_cast<int>(row + 10)).placed,
               "a non-matching tile fills the column");
    }
    expect(!game.place(0, 99).placed, "a full column rejects a tile");
}

void testGameOverAndReset() {
    number_merge::Game game;
    for (std::size_t column = 0; column < number_merge::Game::kColumns; ++column) {
        for (std::size_t row = 0; row < number_merge::Game::kRows; ++row) {
            game.place(column, static_cast<int>(column * 100 + row + 10));
        }
    }
    expect(game.isGameOver(), "a full board ends the game");

    game.reset();
    expect(!game.isGameOver(), "reset starts a playable game");
    expect(game.score() == 0, "reset clears the score");
}

} // namespace

int main() {
    testTileFalls();
    testHorizontalMerge();
    testVerticalMergeAndGravity();
    testCascadeCombo();
    testInvalidMoves();
    testGameOverAndReset();
    std::cout << "All number merge tests passed.\n";
    return 0;
}
