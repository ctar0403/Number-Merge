#define UNICODE
#define _UNICODE
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

#include "number_merge/game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <random>
#include <string>

namespace {

constexpr wchar_t kWindowClass[] = L"WoodNumberMergeWindow";
constexpr int kDesignWidth = 520;
constexpr int kDesignHeight = 900;
constexpr int kBoardLeft = 38;
constexpr int kBoardTop = 188;
constexpr int kBoardRight = 482;
constexpr int kBoardBottom = 770;
constexpr int kGridLeft = 45;
constexpr int kGridTop = 197;
constexpr int kCellWidth = 86;
constexpr int kCellHeight = 64;
constexpr int kGridBottom = kGridTop + static_cast<int>(number_merge::Game::kRows) * kCellHeight;
constexpr UINT_PTR kAnimationTimer = 1;
constexpr double kPi = 3.14159265358979323846;
constexpr ULONGLONG kLaunchDuration = 300;
constexpr ULONGLONG kImpactDuration = 140;
constexpr ULONGLONG kBurstDuration = 720;

COLORREF blend(COLORREF color, COLORREF toward, double amount) {
    const auto mix = [amount](int from, int to) {
        return static_cast<int>(from + (to - from) * amount);
    };
    return RGB(mix(GetRValue(color), GetRValue(toward)),
               mix(GetGValue(color), GetGValue(toward)),
               mix(GetBValue(color), GetBValue(toward)));
}

void fillRoundedRect(HDC dc, int left, int top, int right, int bottom, int radius,
                     COLORREF fill, COLORREF outline = CLR_INVALID, int outline_width = 1) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = outline == CLR_INVALID
                   ? static_cast<HPEN>(GetStockObject(NULL_PEN))
                   : CreatePen(PS_SOLID, outline_width, outline);
    const HGDIOBJ old_brush = SelectObject(dc, brush);
    const HGDIOBJ old_pen = SelectObject(dc, pen);
    RoundRect(dc, left, top, right, bottom, radius, radius);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    if (outline != CLR_INVALID) {
        DeleteObject(pen);
    }
    DeleteObject(brush);
}

void fillRect(HDC dc, int left, int top, int right, int bottom, COLORREF color) {
    RECT rect{left, top, right, bottom};
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void fillEllipse(HDC dc, int center_x, int center_y, int radius, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    const HGDIOBJ old_brush = SelectObject(dc, brush);
    const HGDIOBJ old_pen = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, center_x - radius, center_y - radius,
            center_x + radius + 1, center_y + radius + 1);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(brush);
}

void drawText(HDC dc, const std::wstring& text, RECT rect, int size, int weight,
              COLORREF color, UINT flags = DT_CENTER | DT_VCENTER | DT_SINGLELINE,
              const wchar_t* face = L"Segoe UI") {
    HFONT font = CreateFontW(-size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
    const HGDIOBJ old_font = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, text.c_str(), -1, &rect, flags);
    SelectObject(dc, old_font);
    DeleteObject(font);
}

void drawShadowText(HDC dc, const std::wstring& text, RECT rect, int size,
                    COLORREF color, UINT flags = DT_CENTER | DT_VCENTER | DT_SINGLELINE) {
    RECT shadow = rect;
    OffsetRect(&shadow, 2, 3);
    drawText(dc, text, shadow, size, FW_HEAVY, RGB(60, 25, 10), flags, L"Arial");
    drawText(dc, text, rect, size, FW_HEAVY, color, flags, L"Arial");
}

COLORREF tileColor(int value) {
    static constexpr std::array<COLORREF, 12> colors{
        RGB(68, 182, 239),  RGB(238, 36, 49),   RGB(132, 40, 224),
        RGB(53, 201, 47),   RGB(255, 111, 22),  RGB(239, 62, 148),
        RGB(255, 184, 10),  RGB(13, 132, 235),  RGB(207, 27, 197),
        RGB(14, 177, 157),  RGB(243, 87, 24),   RGB(116, 68, 224)};
    int tier = 0;
    for (int number = std::max(value, 2); number > 2; number /= 2) {
        ++tier;
    }
    return colors[static_cast<std::size_t>(tier) % colors.size()];
}

void drawTile(HDC dc, int x, int y, int width, int height, int value, double scale = 1.0) {
    const int scaled_width = static_cast<int>(width * scale);
    const int scaled_height = static_cast<int>(height * scale);
    x -= (scaled_width - width) / 2;
    y -= (scaled_height - height) / 2;
    width = scaled_width;
    height = scaled_height;

    const COLORREF color = tileColor(value);
    fillRoundedRect(dc, x + 3, y + 5, x + width + 3, y + height + 5, 14,
                    RGB(55, 24, 11));
    fillRoundedRect(dc, x, y, x + width, y + height, 14,
                    blend(color, RGB(0, 0, 0), 0.28), RGB(63, 28, 16), 2);
    fillRoundedRect(dc, x + 2, y + 2, x + width - 2, y + height - 6, 12, color);
    fillRoundedRect(dc, x + 5, y + 4, x + width - 5, y + 11, 7,
                    blend(color, RGB(255, 255, 255), 0.42));

    const std::wstring label = std::to_wstring(value);
    int font_size = 25;
    if (label.size() >= 4) {
        font_size = 20;
    }
    if (label.size() >= 6) {
        font_size = 16;
    }
    RECT text_rect{x, y - 1, x + width, y + height - 1};
    drawShadowText(dc, label, text_rect, font_size, RGB(255, 255, 255));
}

void drawWood(HDC dc) {
    fillRect(dc, 0, 0, kDesignWidth, kDesignHeight, RGB(130, 67, 31));
    constexpr int plank_width = 74;
    for (int plank = -1; plank * plank_width < kDesignWidth; ++plank) {
        const int left = plank * plank_width;
        const COLORREF base = plank % 2 == 0 ? RGB(151, 79, 36) : RGB(137, 67, 30);
        fillRect(dc, left, 0, left + plank_width, kDesignHeight, base);
        fillRect(dc, left, 0, left + 3, kDesignHeight, RGB(91, 42, 20));
        fillRect(dc, left + 4, 0, left + 6, kDesignHeight, RGB(185, 99, 47));

        HPEN grain_pen = CreatePen(PS_SOLID, 1, blend(base, RGB(65, 28, 12), 0.22));
        const HGDIOBJ old_pen = SelectObject(dc, grain_pen);
        for (int grain = 0; grain < 7; ++grain) {
            POINT points[10]{};
            for (int point = 0; point < 10; ++point) {
                const int yy = point * 105 - 30;
                const double wave = std::sin((yy + grain * 37 + plank * 19) * 0.025);
                points[point] = {left + 11 + grain * 8 + static_cast<int>(wave * 3), yy};
            }
            Polyline(dc, points, 10);
        }
        SelectObject(dc, old_pen);
        DeleteObject(grain_pen);
    }
}

int randomTile(std::mt19937& generator) {
    static constexpr std::array<int, 10> tiles{2, 2, 2, 2, 4, 4, 4, 8, 8, 16};
    std::uniform_int_distribution<std::size_t> distribution(0, tiles.size() - 1);
    return tiles[distribution(generator)];
}

struct Layout {
    double scale = 1.0;
    int offset_x = 0;
    int offset_y = 0;
};

Layout layoutFor(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    Layout layout;
    layout.scale = std::min(width / static_cast<double>(kDesignWidth),
                            height / static_cast<double>(kDesignHeight));
    layout.offset_x = static_cast<int>((width - kDesignWidth * layout.scale) / 2.0);
    layout.offset_y = static_cast<int>((height - kDesignHeight * layout.scale) / 2.0);
    return layout;
}

POINT toDesign(HWND window, int x, int y) {
    const Layout layout = layoutFor(window);
    return {static_cast<LONG>((x - layout.offset_x) / layout.scale),
            static_cast<LONG>((y - layout.offset_y) / layout.scale)};
}

bool contains(const RECT& rect, POINT point) {
    return point.x >= rect.left && point.x < rect.right &&
           point.y >= rect.top && point.y < rect.bottom;
}

class AppState {
public:
    enum class Phase { Idle, Launch, Impact, Burst };

    AppState()
        : generator_(std::random_device{}()), current_(randomTile(generator_)),
          next_(randomTile(generator_)) {
        std::ifstream input("number_merge_best.txt");
        input >> best_;
        seedBoard();
    }

    void newGame(HWND window) {
        seedBoard();
        current_ = randomTile(generator_);
        next_ = randomTile(generator_);
        phase_ = Phase::Idle;
        hover_column_ = -1;
        message_.clear();
        KillTimer(window, kAnimationTimer);
        InvalidateRect(window, nullptr, FALSE);
    }

    void launch(HWND window, int column) {
        if (phase_ != Phase::Idle || game_.isGameOver() || column < 0 || column >= 5) {
            return;
        }
        if (!game_.canPlace(static_cast<std::size_t>(column))) {
            message_ = L"COLUMN FULL";
            MessageBeep(MB_ICONWARNING);
            InvalidateRect(window, nullptr, FALSE);
            return;
        }

        before_ = game_.board();
        launch_row_ = 0;
        while (launch_row_ < number_merge::Game::kRows &&
               before_[launch_row_][static_cast<std::size_t>(column)] != 0) {
            ++launch_row_;
        }
        launched_value_ = current_;
        last_move_ = game_.place(static_cast<std::size_t>(column), current_);
        current_ = next_;
        next_ = randomTile(generator_);
        phase_ = Phase::Launch;
        phase_started_ = GetTickCount64();
        message_.clear();

        if (game_.score() > best_) {
            best_ = game_.score();
            std::ofstream output("number_merge_best.txt", std::ios::trunc);
            output << best_;
        }

        SetTimer(window, kAnimationTimer, 16, nullptr);
        InvalidateRect(window, nullptr, FALSE);
    }

    void tick(HWND window) {
        const ULONGLONG elapsed = GetTickCount64() - phase_started_;
        if (phase_ == Phase::Launch && elapsed >= kLaunchDuration) {
            phase_ = Phase::Impact;
            phase_started_ = GetTickCount64();
        } else if (phase_ == Phase::Impact && elapsed >= kImpactDuration) {
            if (last_move_.combo > 0) {
                phase_ = Phase::Burst;
                phase_started_ = GetTickCount64();
                MessageBeep(MB_OK);
            } else {
                phase_ = Phase::Idle;
                KillTimer(window, kAnimationTimer);
            }
        } else if (phase_ == Phase::Burst && elapsed >= kBurstDuration) {
            phase_ = Phase::Idle;
            KillTimer(window, kAnimationTimer);
        }
        InvalidateRect(window, nullptr, FALSE);
    }

    void mouseMove(HWND window, POINT point) {
        int new_hover = -1;
        if (phase_ == Phase::Idle && point.x >= kGridLeft && point.x < kGridLeft + 5 * kCellWidth &&
            point.y >= kBoardTop && point.y < kBoardBottom) {
            new_hover = (point.x - kGridLeft) / kCellWidth;
        }
        const bool new_button_hover = contains(new_button_rect_, point);
        if (new_hover != hover_column_ || new_button_hover != new_button_hover_) {
            hover_column_ = new_hover;
            new_button_hover_ = new_button_hover;
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void click(HWND window, POINT point) {
        if (contains(new_button_rect_, point)) {
            newGame(window);
            return;
        }
        if (point.x >= kGridLeft && point.x < kGridLeft + 5 * kCellWidth &&
            point.y >= kBoardTop && point.y < kBoardBottom) {
            launch(window, (point.x - kGridLeft) / kCellWidth);
        }
    }

    bool isInteractive(POINT point) const {
        return contains(new_button_rect_, point) ||
               (point.x >= kGridLeft && point.x < kGridLeft + 5 * kCellWidth &&
                point.y >= kBoardTop && point.y < kBoardBottom);
    }

    void paint(HWND window, HDC target) const {
        RECT client{};
        GetClientRect(window, &client);
        const int width = client.right;
        const int height = client.bottom;
        HDC dc = CreateCompatibleDC(target);
        HBITMAP bitmap = CreateCompatibleBitmap(target, width, height);
        const HGDIOBJ old_bitmap = SelectObject(dc, bitmap);
        fillRect(dc, 0, 0, width, height, RGB(48, 24, 13));

        const Layout layout = layoutFor(window);
        SetMapMode(dc, MM_ANISOTROPIC);
        SetWindowExtEx(dc, kDesignWidth, kDesignHeight, nullptr);
        SetViewportExtEx(dc, static_cast<int>(kDesignWidth * layout.scale),
                         static_cast<int>(kDesignHeight * layout.scale), nullptr);
        SetViewportOrgEx(dc, layout.offset_x, layout.offset_y, nullptr);

        drawWood(dc);
        drawHeader(dc);
        drawBoard(dc);
        drawControls(dc);

        SetMapMode(dc, MM_TEXT);
        SetViewportOrgEx(dc, 0, 0, nullptr);
        BitBlt(target, 0, 0, width, height, dc, 0, 0, SRCCOPY);
        SelectObject(dc, old_bitmap);
        DeleteObject(bitmap);
        DeleteDC(dc);
    }

private:
    void seedBoard() {
        game_.reset();
        static constexpr std::array<int, 5> first_row{2, 4, 8, 16, 2};
        static constexpr std::array<int, 5> second_row{4, 8, 16, 2, 4};
        for (std::size_t column = 0; column < number_merge::Game::kColumns; ++column) {
            game_.place(column, first_row[column]);
            game_.place(column, second_row[column]);
        }
    }

    void drawHeader(HDC dc) const {
        RECT title{10, 18, 510, 57};
        drawShadowText(dc, L"WOOD NUMBER MERGE", title, 31, RGB(255, 239, 190));
        RECT subtitle{20, 57, 500, 78};
        drawText(dc, L"MATCH 2  -  LEVEL UP  -  MAKE COMBOS", subtitle, 13, FW_BOLD,
                 RGB(255, 213, 130));

        fillRoundedRect(dc, 43, 91, 247, 166, 18, RGB(76, 32, 13), RGB(54, 22, 9), 2);
        fillRoundedRect(dc, 273, 91, 477, 166, 18, RGB(76, 32, 13), RGB(54, 22, 9), 2);
        RECT score_label{55, 96, 235, 119};
        RECT score_value{55, 115, 235, 158};
        RECT best_label{285, 96, 465, 119};
        RECT best_value{285, 115, 465, 158};
        drawText(dc, L"SCORE", score_label, 14, FW_BOLD, RGB(255, 190, 64));
        drawShadowText(dc, std::to_wstring(game_.score()), score_value, 28, RGB(255, 241, 77));
        drawText(dc, L"BEST", best_label, 14, FW_BOLD, RGB(255, 190, 64));
        drawShadowText(dc, std::to_wstring(best_), best_value, 28, RGB(255, 255, 255));
    }

    void drawBoard(HDC dc) const {
        fillRoundedRect(dc, kBoardLeft + 5, kBoardTop + 7, kBoardRight + 5, kBoardBottom + 7,
                        18, RGB(62, 26, 12));
        fillRoundedRect(dc, kBoardLeft, kBoardTop, kBoardRight, kBoardBottom, 18,
                        RGB(72, 34, 17), RGB(44, 19, 9), 3);
        fillRoundedRect(dc, kGridLeft, kGridTop, kGridLeft + 5 * kCellWidth, kBoardBottom - 8,
                        11, RGB(82, 40, 20));

        for (int column = 0; column < 5; ++column) {
            const int left = kGridLeft + column * kCellWidth;
            const COLORREF lane = column % 2 == 0 ? RGB(87, 43, 21) : RGB(76, 36, 18);
            fillRect(dc, left + 2, kGridTop + 2, left + kCellWidth - 2, kBoardBottom - 10, lane);
            if (column == hover_column_ && phase_ == Phase::Idle && !game_.isGameOver()) {
                fillRoundedRect(dc, left + 3, kGridTop + 3, left + kCellWidth - 3,
                                kBoardBottom - 11, 8, RGB(111, 61, 27));
            }
            if (column > 0) {
                fillRect(dc, left - 1, kGridTop, left + 1, kBoardBottom - 8, RGB(42, 18, 8));
            }

            const int center = left + kCellWidth / 2;
            POINT arrow[3]{{center, kGridBottom + 14},
                           {center - 12, kGridBottom + 32},
                           {center + 12, kGridBottom + 32}};
            HBRUSH arrow_brush = CreateSolidBrush(column == hover_column_ ? RGB(255, 196, 44)
                                                                          : RGB(126, 71, 35));
            const HGDIOBJ old_brush = SelectObject(dc, arrow_brush);
            const HGDIOBJ old_pen = SelectObject(dc, GetStockObject(NULL_PEN));
            Polygon(dc, arrow, 3);
            SelectObject(dc, old_pen);
            SelectObject(dc, old_brush);
            DeleteObject(arrow_brush);
        }

        const bool show_before = phase_ == Phase::Launch ||
                                 (phase_ == Phase::Impact && last_move_.combo > 0);
        const auto& visible_board = show_before ? before_ : game_.board();
        for (std::size_t row = 0; row < number_merge::Game::kRows; ++row) {
            for (std::size_t column = 0; column < number_merge::Game::kColumns; ++column) {
                const int value = visible_board[row][column];
                if (value == 0) {
                    continue;
                }
                const bool animated_result =
                    phase_ == Phase::Burst ||
                    (phase_ == Phase::Impact && last_move_.combo == 0);
                if (animated_result && row == last_move_.row && column == last_move_.column) {
                    continue;
                }
                drawTile(dc, kGridLeft + static_cast<int>(column) * kCellWidth + 5,
                         kGridTop + static_cast<int>(row) * kCellHeight + 4,
                         kCellWidth - 10, kCellHeight - 9, value);
            }
        }

        if (phase_ == Phase::Launch) {
            const double elapsed = static_cast<double>(GetTickCount64() - phase_started_);
            const double progress = std::clamp(elapsed / static_cast<double>(kLaunchDuration),
                                               0.0, 1.0);
            const double eased = 1.0 - std::pow(1.0 - progress, 4.0);
            const int target_y = kGridTop + static_cast<int>(launch_row_) * kCellHeight + 4;
            const int start_y = kGridBottom + 28;
            const int y = static_cast<int>(start_y + (target_y - start_y) * eased);
            drawLaunchTrail(dc, y, progress);
            drawTile(dc, kGridLeft + static_cast<int>(last_move_.column) * kCellWidth + 5, y,
                     kCellWidth - 10, kCellHeight - 9, launched_value_);
        } else if (phase_ == Phase::Impact) {
            drawImpact(dc);
        } else if (phase_ == Phase::Burst) {
            drawBurst(dc);
        }

        if (game_.isGameOver() && phase_ == Phase::Idle) {
            fillRoundedRect(dc, 89, 362, 431, 558, 24, RGB(54, 22, 12), RGB(255, 181, 40), 4);
            RECT over{100, 382, 420, 426};
            RECT final_label{100, 431, 420, 461};
            RECT final_score{100, 460, 420, 508};
            RECT hint{100, 515, 420, 543};
            drawShadowText(dc, L"BOARD FULL!", over, 31, RGB(255, 91, 54));
            drawText(dc, L"FINAL SCORE", final_label, 15, FW_BOLD, RGB(255, 203, 119));
            drawShadowText(dc, std::to_wstring(game_.score()), final_score, 34, RGB(255, 244, 74));
            drawText(dc, L"PRESS NEW GAME TO PLAY AGAIN", hint, 13, FW_BOLD, RGB(255, 255, 255));
        }

        if (!message_.empty()) {
            RECT message_rect{100, 721, 420, 758};
            drawShadowText(dc, message_, message_rect, 18, RGB(255, 105, 69));
        }
    }

    void drawLaunchTrail(HDC dc, int tile_y, double progress) const {
        const int center_x = kGridLeft + static_cast<int>(last_move_.column) * kCellWidth +
                             kCellWidth / 2;
        const COLORREF tile_color = tileColor(launched_value_);
        for (int spark = 0; spark < 9; ++spark) {
            const double life = 1.0 - spark / 10.0;
            const int wobble = static_cast<int>(std::sin(progress * 18.0 + spark * 1.7) * 11.0);
            const int y = tile_y + 45 + spark * 15;
            if (y >= kBoardBottom - 9) {
                continue;
            }
            const int radius = std::max(2, 7 - spark / 2);
            const COLORREF color = spark % 2 == 0
                                       ? blend(tile_color, RGB(255, 245, 120), 0.65)
                                       : blend(RGB(255, 151, 22), RGB(92, 43, 20),
                                               (1.0 - life) * 0.45);
            fillEllipse(dc, center_x + wobble, y, radius, color);
        }
    }

    void drawImpact(HDC dc) const {
        const double progress = std::clamp(
            static_cast<double>(GetTickCount64() - phase_started_) /
                static_cast<double>(kImpactDuration),
            0.0, 1.0);
        const int x = kGridLeft + static_cast<int>(last_move_.column) * kCellWidth + 5;
        const std::size_t row = last_move_.combo > 0 ? launch_row_ : last_move_.row;
        const int y = kGridTop + static_cast<int>(row) * kCellHeight + 4;
        const int center_x = x + (kCellWidth - 10) / 2;
        const int center_y = y + (kCellHeight - 9) / 2;

        const int ring_radius = 34 + static_cast<int>(progress * 35.0);
        HPEN ring_pen = CreatePen(PS_SOLID, std::max(1, 4 - static_cast<int>(progress * 3.0)),
                                  blend(tileColor(launched_value_), RGB(255, 235, 104), 0.6));
        const HGDIOBJ old_pen = SelectObject(dc, ring_pen);
        const HGDIOBJ old_brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        Ellipse(dc, center_x - ring_radius, center_y - ring_radius,
                center_x + ring_radius, center_y + ring_radius);
        SelectObject(dc, old_brush);
        SelectObject(dc, old_pen);
        DeleteObject(ring_pen);

        const double bounce = 1.0 + 0.12 * std::sin(progress * kPi);
        const int value = last_move_.combo > 0 ? launched_value_ : last_move_.resulting_value;
        drawTile(dc, x, y, kCellWidth - 10, kCellHeight - 9, value, bounce);
    }

    void drawBurst(HDC dc) const {
        const double progress = std::clamp(
            static_cast<double>(GetTickCount64() - phase_started_) /
                static_cast<double>(kBurstDuration),
            0.0, 1.0);
        const int x = kGridLeft + static_cast<int>(last_move_.column) * kCellWidth + 5;
        const int final_y = kGridTop + static_cast<int>(last_move_.row) * kCellHeight + 4;
        const int start_y = kGridTop + static_cast<int>(launch_row_) * kCellHeight + 4;
        const double move_progress = std::min(1.0, progress / 0.28);
        const double move_eased = move_progress * move_progress * (3.0 - 2.0 * move_progress);
        const int y = static_cast<int>(start_y + (final_y - start_y) * move_eased);
        const int center_x = x + (kCellWidth - 10) / 2;
        const int center_y = final_y + (kCellHeight - 9) / 2;

        const double ray_life = std::max(0.0, 1.0 - progress / 0.48);
        if (ray_life > 0.0) {
            const COLORREF ray_color = blend(RGB(255, 242, 89), RGB(99, 49, 23),
                                             1.0 - ray_life);
            HPEN ray_pen = CreatePen(PS_SOLID, std::max(1, static_cast<int>(5 * ray_life)),
                                     ray_color);
            const HGDIOBJ old_pen = SelectObject(dc, ray_pen);
            for (int ray = 0; ray < 16; ++ray) {
                const double angle = ray * kPi / 8.0;
                const double inner = 37.0 + progress * 35.0;
                const double outer = 70.0 + progress * 95.0;
                MoveToEx(dc, center_x + static_cast<int>(std::cos(angle) * inner),
                         center_y + static_cast<int>(std::sin(angle) * inner), nullptr);
                LineTo(dc, center_x + static_cast<int>(std::cos(angle) * outer),
                       center_y + static_cast<int>(std::sin(angle) * outer));
            }
            SelectObject(dc, old_pen);
            DeleteObject(ray_pen);
        }

        // Deterministic particles keep the effect stable between repaint calls while
        // still providing varied spark directions, speeds, sizes, and colors.
        const COLORREF result_color = tileColor(last_move_.resulting_value);
        for (int particle = 0; particle < 34; ++particle) {
            const double angle = particle * 2.399963229728653 +
                                 (last_move_.resulting_value % 11) * 0.09;
            const double speed = 58.0 + (particle % 9) * 12.0;
            const double distance = speed * progress;
            const int px = center_x + static_cast<int>(std::cos(angle) * distance);
            const int py = center_y + static_cast<int>(std::sin(angle) * distance +
                                                       72.0 * progress * progress);
            const int tail_x = center_x + static_cast<int>(std::cos(angle) * distance * 0.72);
            const int tail_y = center_y + static_cast<int>(std::sin(angle) * distance * 0.72 +
                                                           50.0 * progress * progress);
            COLORREF color = particle % 4 == 0
                                 ? RGB(255, 255, 220)
                                 : (particle % 3 == 0 ? RGB(255, 178, 26) : result_color);
            color = blend(color, RGB(86, 41, 20), progress * 0.72);

            HPEN trail_pen = CreatePen(PS_SOLID, particle % 3 == 0 ? 3 : 2, color);
            const HGDIOBJ old_trail = SelectObject(dc, trail_pen);
            MoveToEx(dc, tail_x, tail_y, nullptr);
            LineTo(dc, px, py);
            SelectObject(dc, old_trail);
            DeleteObject(trail_pen);

            const int radius = std::max(1, static_cast<int>((4 + particle % 5) *
                                                             (1.0 - progress * 0.78)));
            if (particle % 5 == 0) {
                POINT diamond[4]{{px, py - radius - 2}, {px + radius, py},
                                 {px, py + radius + 2}, {px - radius, py}};
                HBRUSH brush = CreateSolidBrush(color);
                const HGDIOBJ old_brush = SelectObject(dc, brush);
                const HGDIOBJ old_particle_pen = SelectObject(dc, GetStockObject(NULL_PEN));
                Polygon(dc, diamond, 4);
                SelectObject(dc, old_particle_pen);
                SelectObject(dc, old_brush);
                DeleteObject(brush);
            } else {
                fillEllipse(dc, px, py, radius, color);
            }
        }

        for (int ring = 0; ring < 3; ++ring) {
            const double ring_progress = std::clamp(progress * 1.8 - ring * 0.16, 0.0, 1.0);
            if (ring_progress <= 0.0 || ring_progress >= 1.0) {
                continue;
            }
            const int radius = 32 + static_cast<int>(ring_progress * 105.0);
            HPEN ring_pen = CreatePen(PS_SOLID, std::max(1, 4 - ring),
                                      blend(result_color, RGB(91, 44, 21), ring_progress));
            const HGDIOBJ old_ring_pen = SelectObject(dc, ring_pen);
            const HGDIOBJ old_ring_brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
            Ellipse(dc, center_x - radius, center_y - radius,
                    center_x + radius, center_y + radius);
            SelectObject(dc, old_ring_brush);
            SelectObject(dc, old_ring_pen);
            DeleteObject(ring_pen);
        }

        const double pulse = 1.0 + 0.24 * std::sin(std::min(1.0, progress / 0.68) * kPi) *
                                         (1.0 - progress * 0.35);
        drawTile(dc, x, y, kCellWidth - 10, kCellHeight - 9,
                 last_move_.resulting_value, pulse);

        if (progress < 0.9) {
            const int badge_y = std::min(kBoardBottom - 70, final_y + 62) -
                                static_cast<int>(progress * 42.0);
            const COLORREF badge_color = blend(RGB(20, 91, 230), RGB(75, 37, 19),
                                               std::max(0.0, progress - 0.65) * 2.0);
            fillRoundedRect(dc, center_x - 86, badge_y, center_x + 86, badge_y + 40, 18,
                            badge_color, RGB(255, 255, 255), 2);
            std::wstring badge = L"+" + std::to_wstring(last_move_.score_gained);
            if (last_move_.combo > 1) {
                badge += L"  COMBO x" + std::to_wstring(last_move_.combo);
            }
            RECT badge_rect{center_x - 84, badge_y, center_x + 84, badge_y + 38};
            drawShadowText(dc, badge, badge_rect, last_move_.combo > 1 ? 15 : 20,
                           RGB(255, 255, 255));
        }
    }

    void drawControls(HDC dc) const {
        RECT prompt{25, 775, 495, 802};
        drawText(dc, L"CLICK A COLUMN TO LAUNCH", prompt, 15, FW_HEAVY,
                 RGB(255, 229, 164));

        fillRoundedRect(dc, 34, 811, 151, 881, 16, RGB(79, 34, 15), RGB(49, 20, 8), 2);
        RECT next_label{39, 815, 91, 840};
        drawText(dc, L"NEXT", next_label, 12, FW_BOLD, RGB(255, 184, 68));
        drawTile(dc, 96, 819, 47, 49, next_);

        RECT current_label{164, 807, 330, 828};
        drawText(dc, L"CURRENT", current_label, 12, FW_BOLD, RGB(255, 184, 68));
        drawTile(dc, 208, 824, 78, 58, current_);

        const COLORREF button = new_button_hover_ ? RGB(255, 154, 24) : RGB(234, 104, 20);
        fillRoundedRect(dc, new_button_rect_.left + 3, new_button_rect_.top + 5,
                        new_button_rect_.right + 3, new_button_rect_.bottom + 5,
                        16, RGB(69, 27, 10));
        fillRoundedRect(dc, new_button_rect_.left, new_button_rect_.top,
                        new_button_rect_.right, new_button_rect_.bottom,
                        16, button, RGB(95, 35, 8), 2);
        RECT button_text = new_button_rect_;
        drawShadowText(dc, L"NEW GAME", button_text, 15, RGB(255, 255, 255));
    }

    number_merge::Game game_;
    number_merge::Game::Board before_{};
    std::mt19937 generator_;
    int current_ = 2;
    int next_ = 2;
    int best_ = 0;
    int launched_value_ = 0;
    std::size_t launch_row_ = 0;
    number_merge::Game::MoveResult last_move_{};
    Phase phase_ = Phase::Idle;
    ULONGLONG phase_started_ = 0;
    int hover_column_ = -1;
    bool new_button_hover_ = false;
    std::wstring message_;
    const RECT new_button_rect_{352, 819, 486, 875};
};

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    auto* app = reinterpret_cast<AppState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
    case WM_NCCREATE: {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(l_param);
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        if (app != nullptr) {
            app->paint(window, dc);
        }
        EndPaint(window, &paint);
        return 0;
    }
    case WM_SIZE:
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    case WM_TIMER:
        if (app != nullptr && w_param == kAnimationTimer) {
            app->tick(window);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (app != nullptr) {
            app->mouseMove(window, toDesign(window, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)));
        }
        return 0;
    case WM_LBUTTONUP:
        if (app != nullptr) {
            app->click(window, toDesign(window, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)));
        }
        return 0;
    case WM_KEYDOWN:
        if (app != nullptr) {
            if (w_param >= '1' && w_param <= '5') {
                app->launch(window, static_cast<int>(w_param - '1'));
            } else if (w_param == 'R' || w_param == 'N') {
                app->newGame(window);
            } else if (w_param == VK_ESCAPE) {
                DestroyWindow(window);
            }
        }
        return 0;
    case WM_SETCURSOR:
        if (app != nullptr && LOWORD(l_param) == HTCLIENT) {
            POINT cursor{};
            GetCursorPos(&cursor);
            ScreenToClient(window, &cursor);
            if (app->isInteractive(toDesign(window, cursor.x, cursor.y))) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        break;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(l_param);
        info->ptMinTrackSize = {390, 700};
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(window, message, w_param, l_param);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show_command) {
    SetProcessDPIAware();
    AppState app;

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = windowProcedure;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    window_class.hbrBackground = nullptr;
    window_class.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&window_class)) {
        return 1;
    }

    RECT work_area{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0);
    const double initial_scale = std::min(
        1.0, std::min((work_area.right - work_area.left - 60) / static_cast<double>(kDesignWidth),
                      (work_area.bottom - work_area.top - 80) / static_cast<double>(kDesignHeight)));
    const int client_width = static_cast<int>(kDesignWidth * initial_scale);
    const int client_height = static_cast<int>(kDesignHeight * initial_scale);
    RECT window_rect{0, 0, client_width, client_height};
    AdjustWindowRectEx(&window_rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    const int window_width = window_rect.right - window_rect.left;
    const int window_height = window_rect.bottom - window_rect.top;
    const int window_x = work_area.left + (work_area.right - work_area.left - window_width) / 2;
    const int window_y = work_area.top + (work_area.bottom - work_area.top - window_height) / 2;
    HWND window = CreateWindowExW(
        0, kWindowClass, L"Wood Number Merge", WS_OVERLAPPEDWINDOW,
        window_x, window_y, window_width, window_height,
        nullptr, nullptr, instance, &app);
    if (window == nullptr) {
        return 1;
    }

    ShowWindow(window, show_command);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
