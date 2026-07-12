/*
 * ============================================================================
 *  TETRIS  —  Real-time C++ Console Edition
 * ============================================================================
 *  Classic falling-block puzzle: guide each tetromino into place, clear full
 *  rows, and try to survive as the pace speeds up.
 *
 *  Controls: A/D move, S soft-drop, W rotate, SPACE hard-drop, Q quit.
 *
 *  New concepts compared to your earlier games:
 *      - Representing a fixed library of shapes (the 7 tetrominoes) as
 *        coordinate data, and rotating between 4 states for each
 *      - A 2D grid that permanently accumulates state (locked blocks stay
 *        forever, unlike Snake's body or Rocket Battle's enemies)
 *      - Row-clearing logic: scanning for full rows, removing them, and
 *        shifting everything above down — a real algorithm worth
 *        understanding, not just copying
 *      - A "next piece" preview and a difficulty curve that speeds up
 *        over time (score-based level progression)
 *
 *  Compile:  g++ -std=c++17 -O2 -Wall -o tetris tetris.cpp
 *  Run:      ./tetris   (run in a real terminal — needs live keyboard input)
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

// ----------------------------------------------------------------------------
//  Cross-platform non-blocking keyboard input (same approach as Snake
//  and Rocket Battle — reused here since it's proven to work).
// ----------------------------------------------------------------------------
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>

    int readKeyNonBlocking() {
        if (_kbhit()) return _getch();
        return -1;
    }
    void sleepMs(int ms) { Sleep(ms); }

#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>

    termios g_originalTermios;
    void restoreTerminal() { tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios); }

    void enableRawMode() {
        tcgetattr(STDIN_FILENO, &g_originalTermios);
        atexit(restoreTerminal);
        termios raw = g_originalTermios;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    int readKeyNonBlocking() {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n == 1) return (unsigned char)c;
        return -1;
    }
    void sleepMs(int ms) { usleep(ms * 1000); }
#endif

namespace Color {
    const string RESET = "\033[0m";
    const string BOLD  = "\033[1m";
    const string CYAN  = "\033[1;36m";   // I piece
    const string YELLOW= "\033[1;33m";   // O piece
    const string MAGENTA="\033[1;35m";   // T piece
    const string GREEN = "\033[1;32m";   // S piece
    const string RED   = "\033[1;31m";   // Z piece
    const string BLUE  = "\033[1;34m";   // J piece
    const string ORANGE= "\033[38;5;208m"; // L piece
    const string GREY  = "\033[0;90m";
}

const int ROWS = 20;
const int COLS = 10;

// ----------------------------------------------------------------------------
//  Tetromino shape data.
//  Each piece has 4 rotation states, each state listing the 4 filled
//  cells (row, col) within a 4x4 bounding box. This is a simplified
//  version of the standard "Super Rotation System" used by real Tetris.
// ----------------------------------------------------------------------------
struct Cell2 { int r, c; };

const array<array<array<Cell2, 4>, 4>, 7> PIECE_SHAPES = {{
    // I
    {{
        {{ {1,0},{1,1},{1,2},{1,3} }},
        {{ {0,2},{1,2},{2,2},{3,2} }},
        {{ {2,0},{2,1},{2,2},{2,3} }},
        {{ {0,1},{1,1},{2,1},{3,1} }},
    }},
    // O
    {{
        {{ {0,1},{0,2},{1,1},{1,2} }},
        {{ {0,1},{0,2},{1,1},{1,2} }},
        {{ {0,1},{0,2},{1,1},{1,2} }},
        {{ {0,1},{0,2},{1,1},{1,2} }},
    }},
    // T
    {{
        {{ {0,1},{1,0},{1,1},{1,2} }},
        {{ {0,1},{1,1},{1,2},{2,1} }},
        {{ {1,0},{1,1},{1,2},{2,1} }},
        {{ {0,1},{1,0},{1,1},{2,1} }},
    }},
    // S
    {{
        {{ {0,1},{0,2},{1,0},{1,1} }},
        {{ {0,1},{1,1},{1,2},{2,2} }},
        {{ {0,1},{0,2},{1,0},{1,1} }},
        {{ {0,1},{1,1},{1,2},{2,2} }},
    }},
    // Z
    {{
        {{ {0,0},{0,1},{1,1},{1,2} }},
        {{ {0,2},{1,1},{1,2},{2,1} }},
        {{ {0,0},{0,1},{1,1},{1,2} }},
        {{ {0,2},{1,1},{1,2},{2,1} }},
    }},
    // J
    {{
        {{ {0,0},{1,0},{1,1},{1,2} }},
        {{ {0,1},{0,2},{1,1},{2,1} }},
        {{ {1,0},{1,1},{1,2},{2,2} }},
        {{ {0,1},{1,1},{2,0},{2,1} }},
    }},
    // L
    {{
        {{ {0,2},{1,0},{1,1},{1,2} }},
        {{ {0,1},{1,1},{2,1},{2,2} }},
        {{ {1,0},{1,1},{1,2},{2,0} }},
        {{ {0,0},{0,1},{1,1},{2,1} }},
    }},
}};

const array<string, 7> PIECE_COLORS = {
    Color::CYAN, Color::YELLOW, Color::MAGENTA, Color::GREEN,
    Color::RED, Color::BLUE, Color::ORANGE
};

// ============================================================================
//  TETRISGAME CLASS
//  Holds the board, the falling piece, and all rules — kept separate from
//  rendering/input so the logic can be unit-tested directly.
// ============================================================================
class TetrisGame {
public:
    TetrisGame() {
        board.assign(ROWS, vector<int>(COLS, -1)); // -1 = empty cell
        nextType = rand() % 7;
        spawnPiece();
    }

    bool moveLeft()  { return tryMove(0, -1); }
    bool moveRight() { return tryMove(0, 1); }

    // Moves the piece down one row. Returns false if it couldn't move
    // (meaning it just locked into place).
    bool softDrop() {
        if (tryMove(1, 0)) return true;
        lockPiece();
        return false;
    }

    void hardDrop() {
        while (tryMove(1, 0)) { /* keep falling */ }
        lockPiece();
    }

    void rotate() {
        int newRotation = (rotationIndex + 1) % 4;
        // Try the rotation as-is, then a couple of small horizontal
        // "wall kicks" so rotating near a wall doesn't just always fail.
        for (int kick : {0, -1, 1, -2, 2}) {
            if (canPlace(pieceType, newRotation, pieceRow, pieceCol + kick)) {
                rotationIndex = newRotation;
                pieceCol += kick;
                return;
            }
        }
        // No valid rotation found — stays as-is (this is normal Tetris behaviour).
    }

    // Advances gravity by one step. Called on a timer by the game loop,
    // separate from the player's own move/rotate key presses.
    void gravityTick() {
        if (gameOver) return;
        softDrop();
    }

    bool isGameOver() const { return gameOver; }
    int getScore() const { return score; }
    int getLevel() const { return level; }
    int getLinesCleared() const { return linesCleared; }
    int getNextPieceType() const { return nextType; }
    const vector<vector<int>>& getBoard() const { return board; }

    // Returns the current piece's 4 absolute (row, col) cells — used by
    // the renderer to draw the falling piece on top of the locked board.
    array<Cell2, 4> getCurrentPieceCells() const {
        array<Cell2, 4> cells;
        for (int i = 0; i < 4; ++i) {
            Cell2 offset = PIECE_SHAPES[pieceType][rotationIndex][i];
            cells[i] = { pieceRow + offset.r, pieceCol + offset.c };
        }
        return cells;
    }
    int getCurrentPieceType() const { return pieceType; }

    // Test-only helpers, not used by the actual game loop in main().
    void debugSetBoard(const vector<vector<int>>& b) { board = b; }
    void debugSetPiece(int type, int rotation, int row, int col) {
        pieceType = type; rotationIndex = rotation; pieceRow = row; pieceCol = col;
    }

private:
    vector<vector<int>> board; // -1 = empty, otherwise a piece-colour index 0-6
    int pieceType = 0, rotationIndex = 0, pieceRow = 0, pieceCol = 0;
    int nextType = 0;
    int score = 0, level = 1, linesCleared = 0;
    bool gameOver = false;

    bool canPlace(int type, int rotation, int row, int col) const {
        for (auto& offset : PIECE_SHAPES[type][rotation]) {
            int r = row + offset.r, c = col + offset.c;
            if (c < 0 || c >= COLS || r >= ROWS) return false;
            if (r >= 0 && board[r][c] != -1) return false; // r<0 means still above the visible board — allowed
        }
        return true;
    }

    bool tryMove(int dRow, int dCol) {
        if (canPlace(pieceType, rotationIndex, pieceRow + dRow, pieceCol + dCol)) {
            pieceRow += dRow;
            pieceCol += dCol;
            return true;
        }
        return false;
    }

    void spawnPiece() {
        pieceType = nextType;
        nextType = rand() % 7;
        rotationIndex = 0;
        pieceRow = -1; // start just above the visible board, like real Tetris
        pieceCol = COLS / 2 - 2;

        if (!canPlace(pieceType, rotationIndex, pieceRow, pieceCol)) {
            gameOver = true;
        }
    }

    void lockPiece() {
        for (auto& offset : PIECE_SHAPES[pieceType][rotationIndex]) {
            int r = pieceRow + offset.r, c = pieceCol + offset.c;
            if (r >= 0 && r < ROWS && c >= 0 && c < COLS) board[r][c] = pieceType;
        }
        clearFullLines();
        spawnPiece();
    }

    void clearFullLines() {
        int cleared = 0;
        for (int r = ROWS - 1; r >= 0; --r) {
            bool full = true;
            for (int c = 0; c < COLS; ++c) {
                if (board[r][c] == -1) { full = false; break; }
            }
            if (full) {
                board.erase(board.begin() + r);
                board.insert(board.begin(), vector<int>(COLS, -1));
                cleared++;
                r++; // re-check this row index, since everything shifted down into it
            }
        }

        if (cleared > 0) {
            // Simplified version of the classic Tetris scoring table.
            // Clamp defensively: a single real piece can clear at most 4
            // lines, but this guards against any unexpected edge case.
            int scoreIndex = min(cleared, 4);
            static const int LINE_SCORES[5] = {0, 100, 300, 500, 800};
            score += LINE_SCORES[scoreIndex] * level;
            linesCleared += cleared;
            level = 1 + linesCleared / 10; // level up every 10 lines
        }
    }
};

// ----------------------------------------------------------------------------
//  Rendering
// ----------------------------------------------------------------------------
void render(const TetrisGame& game) {
    cout << "\033[H\033[2J";

    // Build a display grid starting from the locked board.
    vector<vector<int>> display = game.getBoard();
    for (auto& cell : game.getCurrentPieceCells()) {
        if (cell.r >= 0 && cell.r < ROWS && cell.c >= 0 && cell.c < COLS) {
            display[cell.r][cell.c] = game.getCurrentPieceType();
        }
    }

    cout << "Score: " << game.getScore() << "   Level: " << game.getLevel()
         << "   Lines: " << game.getLinesCleared() << "\n";
    cout << "(A/D move, S soft-drop, W rotate, SPACE hard-drop, Q quit)\n\n";

    cout << "+" << string(COLS * 2, '-') << "+\n";
    for (int r = 0; r < ROWS; ++r) {
        cout << "|";
        for (int c = 0; c < COLS; ++c) {
            if (display[r][c] == -1) cout << "  ";
            else cout << PIECE_COLORS[display[r][c]] << "[]" << Color::RESET;
        }
        cout << "|\n";
    }
    cout << "+" << string(COLS * 2, '-') << "+\n";

    cout << "Next: ";
    int nt = game.getNextPieceType();
    for (auto& offset : PIECE_SHAPES[nt][0]) {
        cout << "(" << offset.r << "," << offset.c << ") ";
    }
    cout << "  [" << PIECE_COLORS[nt] << "####" << Color::RESET << "]\n";
}

// ============================================================================
//  ENTRY POINT
// ============================================================================
int main() {
    srand(static_cast<unsigned int>(time(0)));

#ifndef _WIN32
    enableRawMode();
#endif

    cout << Color::BOLD << Color::CYAN <<
R"(
  _____ ______ _______ _____ _____  _____
 |_   _|  ____|__   __|  __ \_   _|/ ____|
   | | | |__     | |  | |__) || | | (___
   | | |  __|    | |  |  _  / | |  \___ \
  _| |_| |____   | |  | | \ \_| |_ ____) |
 |_____|______|  |_|  |_|  \_\_____|_____/
)"
    << Color::RESET << Color::GREY << "  Press Enter to start...\n" << Color::RESET;
    cin.get();

    TetrisGame game;

    const int BASE_TICK_MS = 500;
    int ticksSinceGravity = 0;

    while (!game.isGameOver()) {
        render(game);

        int key = readKeyNonBlocking();
        switch (key) {
            case 'a': case 'A': game.moveLeft();  break;
            case 'd': case 'D': game.moveRight(); break;
            case 's': case 'S': game.softDrop();  break;
            case 'w': case 'W': game.rotate();    break;
            case ' ':           game.hardDrop();  break;
            case 'q': case 'Q':
                cout << "\nQuitting. Final score: " << game.getScore() << "\n";
                return 0;
            default: break;
        }

        // Gravity speeds up as the level increases, independent of how
        // often the player presses keys.
        int gravityIntervalMs = max(80, BASE_TICK_MS - (game.getLevel() - 1) * 40);
        ticksSinceGravity += 30; // matches the sleepMs() call below
        if (ticksSinceGravity >= gravityIntervalMs) {
            game.gravityTick();
            ticksSinceGravity = 0;
        }

        sleepMs(30);
    }

    render(game);
    cout << "\nGame over! Final score: " << game.getScore()
         << "   Lines cleared: " << game.getLinesCleared() << "\n";
    return 0;
}
