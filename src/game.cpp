#include "number_merge/game.hpp"

#include <algorithm>
#include <queue>

namespace number_merge {

Game::Game() {
    reset();
}

void Game::reset() {
    for (auto& row : board_) {
        row.fill(0);
    }
    score_ = 0;
}

Game::MoveResult Game::place(std::size_t column, int value) {
    MoveResult result;
    if (column >= kColumns || value <= 0 || !canPlace(column)) {
        return result;
    }

    Position active{kRows - 1, column};
    while (board_[active.row][column] != 0) {
        --active.row;
    }
    board_[active.row][column] = value;
    result.placed = true;

    // Only the newly placed (and then upgraded) tile can trigger a merge.
    // This makes each move predictable while still allowing cascade combos.
    for (int chain = 1;; ++chain) {
        const auto group = connectedGroup(active);
        if (group.size() < 3) {
            break;
        }

        const int upgraded = board_[active.row][active.column] * 2;
        for (const Position position : group) {
            board_[position.row][position.column] = 0;
        }
        board_[active.row][active.column] = upgraded;

        const int points = upgraded * static_cast<int>(group.size()) * chain;
        result.score_gained += points;
        result.combo = chain;

        for (std::size_t current_column = 0; current_column < kColumns; ++current_column) {
            active = applyGravityToColumn(current_column, active);
        }
    }

    score_ += result.score_gained;
    return result;
}

const Game::Board& Game::board() const noexcept {
    return board_;
}

int Game::score() const noexcept {
    return score_;
}

bool Game::isGameOver() const noexcept {
    return std::all_of(board_.front().begin(), board_.front().end(),
                       [](int value) { return value != 0; });
}

bool Game::canPlace(std::size_t column) const noexcept {
    return column < kColumns && board_[0][column] == 0;
}

std::vector<Game::Position> Game::connectedGroup(Position start) const {
    const int target = board_[start.row][start.column];
    if (target == 0) {
        return {};
    }

    std::array<std::array<bool, kColumns>, kRows> visited{};
    std::queue<Position> pending;
    std::vector<Position> group;
    pending.push(start);
    visited[start.row][start.column] = true;

    constexpr int row_offsets[] = {-1, 1, 0, 0};
    constexpr int column_offsets[] = {0, 0, -1, 1};

    while (!pending.empty()) {
        const Position current = pending.front();
        pending.pop();
        group.push_back(current);

        for (int direction = 0; direction < 4; ++direction) {
            const int next_row = static_cast<int>(current.row) + row_offsets[direction];
            const int next_column = static_cast<int>(current.column) + column_offsets[direction];
            if (next_row < 0 || next_row >= static_cast<int>(kRows) ||
                next_column < 0 || next_column >= static_cast<int>(kColumns)) {
                continue;
            }

            const auto row = static_cast<std::size_t>(next_row);
            const auto column = static_cast<std::size_t>(next_column);
            if (!visited[row][column] && board_[row][column] == target) {
                visited[row][column] = true;
                pending.push({row, column});
            }
        }
    }

    return group;
}

Game::Position Game::applyGravityToColumn(std::size_t column, Position tracked) {
    std::size_t write_row = kRows;
    for (std::size_t read_row = kRows; read_row-- > 0;) {
        if (board_[read_row][column] == 0) {
            continue;
        }

        --write_row;
        if (write_row != read_row) {
            board_[write_row][column] = board_[read_row][column];
            board_[read_row][column] = 0;
        }
        if (tracked.row == read_row && tracked.column == column) {
            tracked.row = write_row;
        }
    }
    return tracked;
}

} // namespace number_merge

