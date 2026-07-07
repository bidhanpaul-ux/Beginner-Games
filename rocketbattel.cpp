#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// ----------------------------------------------------------------------------
//  Cross-platform non-blocking keyboard input (same approach as Snake).
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

    void restoreTerminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios);
    }

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

// ----------------------------------------------------------------------------
//  Basic types
// ----------------------------------------------------------------------------
struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

// ============================================================================
//  ROCKETBATTLE CLASS
//  All game state and rules, kept separate from rendering/input (same
//  design idea as the Snake game) so the logic can be tested on its own.
// ============================================================================
class RocketBattle {
public:
    RocketBattle(int width, int height)
        : width(width), height(height), playerX(width / 2) {
    }

    void moveLeft()  { if (playerX > 0) playerX--; }
    void moveRight() { if (playerX < width - 1) playerX++; }

    // Fires a bullet from the player's current position, straight up.
    void fire() {
        bullets.push_back(Point{playerX, height - 2});
    }

    // Advances the whole game world by one tick. Returns false once the
    // game has ended (out of lives).
    bool step() {
        if (gameOver) return false;
        tickCount++;

        moveBullets();
        maybeMoveEnemies();
        maybeSpawnEnemy();
        resolveCollisions();
        resolveEnemiesReachingBottom();

        if (lives <= 0) gameOver = true;
        return !gameOver;
    }

    bool isGameOver() const { return gameOver; }
    int getScore() const { return score; }
    int getLives() const { return lives; }
    int getPlayerX() const { return playerX; }
    int getPlayerY() const { return height - 1; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const std::vector<Point>& getBullets() const { return bullets; }
    const std::vector<Point>& getEnemies() const { return enemies; }

    // Test-only helper: lets unit tests set up specific scenarios directly
    // instead of relying on random spawns.
    void debugAddEnemy(int x, int y) { enemies.push_back({x, y}); }

private:
    int width, height;
    int playerX;
    std::vector<Point> bullets;
    std::vector<Point> enemies;
    int score = 0;
    int lives = 3;
    bool gameOver = false;
    int tickCount = 0;

    static const int ENEMY_MOVE_INTERVAL = 4; // enemies move every 4 ticks (slower than bullets)
    static const int SPAWN_INTERVAL = 12;      // a new enemy appears every 12 ticks

    void moveBullets() {
        for (auto& b : bullets) b.y -= 1;
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(), [](const Point& b) { return b.y < 0; }),
            bullets.end()
        );
    }

    void maybeMoveEnemies() {
        if (tickCount % ENEMY_MOVE_INTERVAL != 0) return;
        for (auto& e : enemies) e.y += 1;
    }

    void maybeSpawnEnemy() {
        if (tickCount % SPAWN_INTERVAL != 0) return;
        int x = rand() % width;
        enemies.push_back({x, 0});
    }

    // A bullet destroys any enemy occupying the same cell; both are removed
    // and the score goes up. Using indices (not iterators) keeps this easy
    // to follow — check every bullet against every enemy.
    void resolveCollisions() {
        std::vector<bool> bulletHit(bullets.size(), false);
        std::vector<bool> enemyHit(enemies.size(), false);

        for (size_t bi = 0; bi < bullets.size(); ++bi) {
            for (size_t ei = 0; ei < enemies.size(); ++ei) {
                if (enemyHit[ei]) continue;
                if (bullets[bi] == enemies[ei]) {
                    bulletHit[bi] = true;
                    enemyHit[ei] = true;
                    score++;
                    break;
                }
            }
        }

        std::vector<Point> survivingBullets, survivingEnemies;
        for (size_t i = 0; i < bullets.size(); ++i) if (!bulletHit[i]) survivingBullets.push_back(bullets[i]);
        for (size_t i = 0; i < enemies.size(); ++i) if (!enemyHit[i]) survivingEnemies.push_back(enemies[i]);
        bullets = survivingBullets;
        enemies = survivingEnemies;
    }

    // Any enemy that reaches the player's row without being shot costs a life.
    void resolveEnemiesReachingBottom() {
        int playerRow = height - 1;
        std::vector<Point> remaining;
        for (auto& e : enemies) {
            if (e.y >= playerRow) {
                lives--;
            } else {
                remaining.push_back(e);
            }
        }
        enemies = remaining;
    }
};

// ----------------------------------------------------------------------------
//  Rendering
// ----------------------------------------------------------------------------
void render(const RocketBattle& game) {
    std::cout << "\033[H\033[2J"; // move cursor home + clear screen

    int width = game.getWidth();
    int height = game.getHeight();

    std::cout << "Score: " << game.getScore() << "   Lives: " << game.getLives()
               << "   (A/D move, SPACE fire, Q quit)\n";
    std::cout << "+" << std::string(width, '-') << "+\n";

    for (int y = 0; y < height; ++y) {
        std::cout << "|";
        for (int x = 0; x < width; ++x) {
            char c = ' ';

            if (x == game.getPlayerX() && y == game.getPlayerY()) {
                c = 'A'; // player's rocket
            } else {
                for (auto& b : game.getBullets()) {
                    if (b.x == x && b.y == y) { c = '|'; break; }
                }
                if (c == ' ') {
                    for (auto& e : game.getEnemies()) {
                        if (e.x == x && e.y == y) { c = 'V'; break; }
                    }
                }
            }

            std::cout << c;
        }
        std::cout << "|\n";
    }

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
    const int HEIGHT = 18;
    const int TICK_MS = 100;

    RocketBattle game(WIDTH, HEIGHT);

    while (!game.isGameOver()) {
        render(game);

        int key = readKeyNonBlocking();
        switch (key) {
            case 'a': case 'A': game.moveLeft();  break;
            case 'd': case 'D': game.moveRight(); break;
            case ' ':           game.fire();      break;
            case 'q': case 'Q':
                std::cout << "\nQuitting. Final score: " << game.getScore() << "\n";
                return 0;
            default: break;
        }

        game.step();
        sleepMs(TICK_MS);
    }

    render(game);
    std::cout << "\nGame over! Final score: " << game.getScore() << "\n";
    return 0;
}
