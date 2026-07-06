/*
 * ============================================================================
 *  SNAKE  —  Real-time C++ Console Game
 * ============================================================================
 *  A classic Snake game that runs live in your terminal — the snake keeps
 *  moving on its own, and you steer it with WASD without pressing Enter.
 *
 *  New concepts compared to your earlier turn-based games:
 *      - std::deque              (a list you can add/remove from both ends —
 *                                  perfect for a snake's body)
 *      - Real-time game loop      (the game advances on a timer, not just
 *                                  when you press Enter)
 *      - Non-blocking keyboard input (reading a key WITHOUT pausing the
 *                                  program to wait for it)
 *      - Cross-platform code      (#ifdef _WIN32 ... #else ... #endif lets
 *                                  one file compile correctly on both
 *                                  Windows and Linux/macOS)
 *
 *  Controls: W/A/S/D to move, Q to quit.
 *
 *  Compile (Linux/macOS):  g++ -std=c++17 -O2 -Wall -o snake snake.cpp
 *  Compile (Windows):      g++ -std=c++17 -O2 -Wall -o snake.exe snake.cpp
 *  Run:                    ./snake   (must be run in a real terminal window,
 *                                      not through an IDE's "output" panel,
 *                                      or piped input — it needs live keys.)
 * ============================================================================
 */

#include <iostream>
#include <deque>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <string>

// ----------------------------------------------------------------------------
//  Cross-platform terminal handling.
//  Reading a single keypress WITHOUT blocking (i.e. without freezing the
//  program until the user presses something) works differently on Windows
//  vs Linux/macOS, so we write two versions and let the preprocessor pick
//  the right one at compile time based on which OS you're compiling for.
// ----------------------------------------------------------------------------
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>

    // Returns the pressed key, or -1 if nothing was pressed.
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

    void restoreTerminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios);
    }

    // Puts the terminal into "raw mode": normally the terminal waits for
    // you to press Enter before your program sees any input (this is
    // called "canonical mode"), and it echoes what you type back to the
    // screen. Raw mode turns both of those off, so we can read a single
    // keypress the instant it happens.
    void enableRawMode() {
        tcgetattr(STDIN_FILENO, &g_originalTermios);
        atexit(restoreTerminal); // make sure the terminal is restored when the program exits

        termios raw = g_originalTermios;
        raw.c_lflag &= ~(ECHO | ICANON); // turn off echo and "wait for Enter"
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        // Also make reading non-blocking, so checking for a key never pauses the game loop.
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

// ----------------------------------------------------------------------------
//  Game types
// ----------------------------------------------------------------------------
enum class Direction { UP, DOWN, LEFT, RIGHT };

struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

// ============================================================================
//  SNAKEGAME CLASS
//  Holds all the game STATE and RULES, completely separate from how it's
//  drawn on screen or how keys are read. This separation is good practice:
//  it means the core logic can be tested on its own (see test_snake.cpp).
// ============================================================================
class SnakeGame {
public:
    SnakeGame(int width, int height) : width(width), height(height) {
        // Start with a snake of length 3 in the middle, moving right.
        int startX = width / 2;
        int startY = height / 2;
        body.push_back({startX, startY});
        body.push_back({startX - 1, startY});
        body.push_back({startX - 2, startY});
        direction = Direction::RIGHT;
        spawnFood();
    }

    // Changes direction, but refuses to let the snake reverse directly
    // into itself (e.g. going Right then instantly Left).
    void setDirection(Direction newDir) {
        bool isOpposite =
            (direction == Direction::UP    && newDir == Direction::DOWN)  ||
            (direction == Direction::DOWN  && newDir == Direction::UP)    ||
            (direction == Direction::LEFT  && newDir == Direction::RIGHT) ||
            (direction == Direction::RIGHT && newDir == Direction::LEFT);

        if (!isOpposite) direction = newDir;
    }

    // Advances the game by one tick. Returns false if the snake just died.
    bool step() {
        if (gameOver) return false;

        Point head = body.front();
        Point newHead = head;
        switch (direction) {
            case Direction::UP:    newHead.y -= 1; break;
            case Direction::DOWN:  newHead.y += 1; break;
            case Direction::LEFT:  newHead.x -= 1; break;
            case Direction::RIGHT: newHead.x += 1; break;
        }

        // Wall collision
        if (newHead.x < 0 || newHead.x >= width || newHead.y < 0 || newHead.y >= height) {
            gameOver = true;
            return false;
        }

        bool willGrow = (newHead == food);

        // Self-collision. If the snake ISN'T growing this turn, its tail
        // segment is about to move away, so colliding with the tail's
        // CURRENT position is actually fine — only the segments that will
        // still be occupied next turn count as a collision.
        int segmentsToCheck = willGrow ? (int)body.size() : (int)body.size() - 1;
        for (int i = 0; i < segmentsToCheck; ++i) {
            if (body[i] == newHead) {
                gameOver = true;
                return false;
            }
        }

        body.push_front(newHead);

        if (willGrow) {
            score++;
            spawnFood();
            // Note: we do NOT pop_back() here — leaving the tail in place
            // is exactly what makes the snake grow by one segment.
        } else {
            body.pop_back();
        }

        return true;
    }

    bool isGameOver() const { return gameOver; }
    int getScore() const { return score; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const std::deque<Point>& getBody() const { return body; }
    Point getFood() const { return food; }

private:
    int width, height;
    std::deque<Point> body;
    Direction direction;
    Point food{0, 0};
    int score = 0;
    bool gameOver = false;

    void spawnFood() {
        // Keep picking random cells until we find one the snake isn't on.
        // (Fine for a snake this small relative to the board; would need a
        // smarter approach if the snake ever nearly filled the board.)
        while (true) {
            Point candidate{ rand() % width, rand() % height };
            bool onSnake = false;
            for (auto& segment : body) {
                if (segment == candidate) { onSnake = true; break; }
            }
            if (!onSnake) {
                food = candidate;
                return;
            }
        }
    }
};

// ----------------------------------------------------------------------------
//  Rendering — draws the current game state as text.
// ----------------------------------------------------------------------------
void render(const SnakeGame& game) {
    // ANSI escape code: move cursor to top-left and clear the screen.
    // This lets us redraw the board in place instead of scrolling forever.
    std::cout << "\033[H\033[2J";

    int width = game.getWidth();
    int height = game.getHeight();
    const auto& body = game.getBody();
    Point food = game.getFood();

    std::cout << "Score: " << game.getScore() << "   (W/A/S/D to move, Q to quit)\n";

    // Top border
    std::cout << "+" << std::string(width, '-') << "+\n";

    for (int y = 0; y < height; ++y) {
        std::cout << "|";
        for (int x = 0; x < width; ++x) {
            Point here{x, y};
            if (here == body.front()) {
                std::cout << "O"; // head
            } else if (here == food) {
                std::cout << "*"; // food
            } else {
                bool isBody = false;
                for (auto it = body.begin() + 1; it != body.end(); ++it) {
                    if (*it == here) { isBody = true; break; }
                }
                std::cout << (isBody ? "#" : " ");
            }
        }
        std::cout << "|\n";
    }

    // Bottom border
    std::cout << "+" << std::string(width, '-') << "+\n";
}

// ============================================================================
//  ENTRY POINT
// ============================================================================
int main() {
    srand(static_cast<unsigned int>(time(0)));

#ifndef _WIN32
    enableRawMode();
#endif

    const int WIDTH = 30;
    const int HEIGHT = 15;
    const int TICK_MS = 150; // lower = faster game speed

    SnakeGame game(WIDTH, HEIGHT);

    while (!game.isGameOver()) {
        render(game);

        int key = readKeyNonBlocking();
        switch (key) {
            case 'w': case 'W': game.setDirection(Direction::UP);    break;
            case 's': case 'S': game.setDirection(Direction::DOWN);  break;
            case 'a': case 'A': game.setDirection(Direction::LEFT);  break;
            case 'd': case 'D': game.setDirection(Direction::RIGHT); break;
            case 'q': case 'Q':
                std::cout << "\nQuitting. Final score: " << game.getScore() << "\n";
                return 0;
            default: break; // no key pressed, or an unrecognised one — ignore
        }

        game.step();
        sleepMs(TICK_MS);
    }

    render(game);
    std::cout << "\nGame over! Final score: " << game.getScore() << "\n";
    return 0;
}
