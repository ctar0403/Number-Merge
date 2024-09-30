#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace number_merge {

class Game {
public:
    static constexpr std::size_t kRows = 8;
    static constexpr std::size_t kColumns = 5;
    using Board = std::array<std::array<int, kColumns>, kRows>;

    struct MoveResult {
        bool placed = false;
        int score_gained = 0;
        int combo = 0;
        std::size_t row = 0;
        std::size_t column = 0;
        int resulting_value = 0;
    };

    Game();

    MoveResult place(std::size_t column, int value);
    void reset();

    const Board& board() const noexcept;
    int score() const noexcept;
    bool isGameOver() const noexcept;
    bool canPlace(std::size_t column) const noexcept;

private:
    struct Position {
        std::size_t row;
        std::size_t column;
    };

    std::vector<Position> connectedGroup(Position start) const;
    Position compactColumnToTop(std::size_t column, Position tracked);

    Board board_{};
    int score_ = 0;
};

} // namespace number_merge
