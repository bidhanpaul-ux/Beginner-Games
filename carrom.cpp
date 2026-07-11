/*
 * ============================================================================
 *  CARROM  —  C++ Console Edition with real 2D physics
 * ============================================================================
 *  A simplified 2-player Carrom game. Aim your striker with an angle and
 *  power, and watch it collide with coins, bounce off walls, and pot coins
 *  into the corner pockets — all simulated with actual physics, then drawn
 *  as an animated ASCII board.
 *
 *  SIMPLIFICATIONS (worth knowing, and worth mentioning in your README):
 *      - The board is a square, not the official diamond-bordered board.
 *      - A smaller ring of coins is used (8 coins + queen) instead of the
 *        official 18 coins + queen.
 *      - All coins (including the striker) have equal mass for a clean
 *        elastic-collision formula. Real strikers are heavier.
 *      - The "cover the queen" rule is simplified to an instant bonus.
 *      - The board is diamond-shaped in real Carrom; here it's a square
 *        with 4 corner pockets, which is enough to demonstrate the same
 *        physics ideas.
 *
 *  New concepts compared to your earlier games:
 *      - Continuous 2D physics (floating-point positions & velocities,
 *        not a fixed grid)
 *      - Circle-circle collision detection & elastic collision response
 *      - Friction (gradual deceleration) and wall bounces (reflection)
 *      - Simulating many physics "ticks" per turn and animating the result
 *
 *  Compile:  g++ -std=c++17 -O2 -Wall -o carrom carrom.cpp
 *  Run:      ./carrom
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <chrono>
#include <thread>

using namespace std;

namespace Color {
    const string RESET  = "\033[0m";
    const string BOLD   = "\033[1m";
    const string WHITE_C= "\033[1;97m";
    const string BLACK_C= "\033[1;30m";
    const string RED    = "\033[1;31m";
    const string YELLOW = "\033[1;33m";
    const string CYAN   = "\033[1;36m";
    const string GREY   = "\033[0;90m";
}

// ----------------------------------------------------------------------------
//  Board / physics constants
// ----------------------------------------------------------------------------
const double FIELD_SIZE   = 300.0;  // playing field is FIELD_SIZE x FIELD_SIZE units
const double COIN_RADIUS  = 7.0;
const double STRIKER_RADIUS = 8.0;
const double POCKET_RADIUS = 13.0;
const double POCKET_INSET  = 16.0;  // pockets sit slightly inside the corners
const double FRICTION      = 0.985; // velocity multiplier applied each physics tick
const double STOP_SPEED    = 0.05;  // below this speed, a coin is considered stopped
const double WALL_RESTITUTION = 0.75; // energy retained (as a velocity fraction) on a wall bounce
const int    MAX_TICKS_PER_SHOT = 600;

enum class CoinType { STRIKER, WHITE, BLACK, QUEEN };

struct Coin {
    double x, y;
    double vx = 0, vy = 0;
    double radius;
    CoinType type;
    bool active = true; // false once potted
};

string coinGlyph(CoinType t) {
    switch (t) {
        case CoinType::STRIKER: return Color::CYAN + "S" + Color::RESET;
        case CoinType::WHITE:   return Color::WHITE_C + "o" + Color::RESET;
        case CoinType::BLACK:   return Color::BLACK_C + "o" + Color::RESET;
        case CoinType::QUEEN:   return Color::RED + "Q" + Color::RESET;
    }
    return "?";
}

// ============================================================================
//  CARROMGAME CLASS
//  Holds all coins and pure physics/game logic, separate from rendering
//  and input — same design principle used in the Snake and Rocket Battle
//  games, which is what lets this be unit-tested directly.
// ============================================================================
class CarromGame {
public:
    CarromGame() { setupBoard(); }

    void setupBoard() {
        coins.clear();
        double cx = FIELD_SIZE / 2.0;
        double cy = FIELD_SIZE / 2.0;

        // Queen at the exact centre.
        coins.push_back(Coin{cx, cy, 0, 0, COIN_RADIUS, CoinType::QUEEN});

        // A ring of 8 coins (alternating White/Black) around the queen —
        // a simplified stand-in for the official 18-coin hexagonal layout.
        double ringRadius = COIN_RADIUS * 2.4;
        for (int i = 0; i < 8; ++i) {
            double angle = i * (2.0 * M_PI / 8.0);
            double x = cx + ringRadius * cos(angle);
            double y = cy + ringRadius * sin(angle);
            CoinType t = (i % 2 == 0) ? CoinType::WHITE : CoinType::BLACK;
            coins.push_back(Coin{x, y, 0, 0, COIN_RADIUS, t});
        }

        spawnStriker(cx);
        whiteScore = blackScore = 0;
        whitesTurn = true;
        gameOver = false;
    }

    // Places (or replaces) the striker at a given x position along the
    // player's own baseline (near the bottom edge).
    void spawnStriker(double x) {
        // Remove any existing striker first (there's only ever one).
        for (auto it = coins.begin(); it != coins.end(); ) {
            if (it->type == CoinType::STRIKER) it = coins.erase(it);
            else ++it;
        }
        double clampedX = max(STRIKER_RADIUS, min(FIELD_SIZE - STRIKER_RADIUS, x));
        coins.push_back(Coin{clampedX, FIELD_SIZE - 20.0, 0, 0, STRIKER_RADIUS, CoinType::STRIKER});
    }

    // Launches the striker at the given angle (degrees, 0 = right, 90 = straight
    // up the board, 180 = left) and power (1-100), then simulates physics
    // until everything stops moving or we hit the safety tick limit.
    // If `frames` is non-null, a snapshot of coin positions is appended
    // periodically so the caller can animate the shot.
    void takeShot(double angleDegrees, double power, vector<vector<Coin>>* frames = nullptr) {
        Coin* striker = findStriker();
        if (!striker) return;

        double angleRad = angleDegrees * M_PI / 180.0;
        double speed = power * 1.3; // scales the 1-100 power input into simulation units
        striker->vx = cos(angleRad) * speed;
        striker->vy = -sin(angleRad) * speed; // negative because y=0 is the top of the field

        bool anyPottedThisShot = false;

        for (int tick = 0; tick < MAX_TICKS_PER_SHOT; ++tick) {
            applyFriction();
            moveCoins();
            resolveWallCollisions();
            resolveCoinCollisions();
            anyPottedThisShot |= resolvePocketing();

            if (frames && tick % 3 == 0) frames->push_back(coins);

            if (allCoinsStopped()) break;
        }

        (void)anyPottedThisShot; // scoring already applied inside resolvePocketing()

        // Prepare the next turn: respawn the striker (fouls already handled).
        if (findStriker() == nullptr) {
            spawnStriker(FIELD_SIZE / 2.0);
        }

        checkGameOver();
        if (!gameOver) whitesTurn = !whitesTurn;
    }

    Coin* findStriker() {
        for (auto& c : coins) if (c.type == CoinType::STRIKER && c.active) return &c;
        return nullptr;
    }

    bool isGameOver() const { return gameOver; }
    bool isWhitesTurn() const { return whitesTurn; }
    int getWhiteScore() const { return whiteScore; }
    int getBlackScore() const { return blackScore; }
    const vector<Coin>& getCoins() const { return coins; }

    int remainingCoinsOfType(CoinType t) const {
        int count = 0;
        for (auto& c : coins) if (c.active && c.type == t) count++;
        return count;
    }

    // ---- Test-only helpers: let unit tests set up exact scenarios and
    // ---- step the physics one tick at a time, without going through a
    // ---- full animated shot. Not used by the actual game in main().
    void debugSetCoins(const vector<Coin>& newCoins) { coins = newCoins; }
    void debugStepOnce() {
        applyFriction();
        moveCoins();
        resolveWallCollisions();
        resolveCoinCollisions();
        resolvePocketing();
    }
    void debugSetTurn(bool isWhite) { whitesTurn = isWhite; }

private:
    vector<Coin> coins;
    int whiteScore = 0, blackScore = 0;
    bool whitesTurn = true;
    bool gameOver = false;

    void applyFriction() {
        for (auto& c : coins) {
            if (!c.active) continue;
            c.vx *= FRICTION;
            c.vy *= FRICTION;
            if (hypot(c.vx, c.vy) < STOP_SPEED) { c.vx = 0; c.vy = 0; }
        }
    }

    void moveCoins() {
        for (auto& c : coins) {
            if (!c.active) continue;
            c.x += c.vx * 0.15; // 0.15 is the simulation's per-tick time step
            c.y += c.vy * 0.15;
        }
    }

    // Reflects a coin's velocity off the field's edges, keeping it inside
    // the boundary and losing a bit of speed (restitution) on each bounce.
    void resolveWallCollisions() {
        for (auto& c : coins) {
            if (!c.active) continue;
            if (c.x - c.radius < 0)              { c.x = c.radius; c.vx = -c.vx * WALL_RESTITUTION; }
            if (c.x + c.radius > FIELD_SIZE)      { c.x = FIELD_SIZE - c.radius; c.vx = -c.vx * WALL_RESTITUTION; }
            if (c.y - c.radius < 0)              { c.y = c.radius; c.vy = -c.vy * WALL_RESTITUTION; }
            if (c.y + c.radius > FIELD_SIZE)      { c.y = FIELD_SIZE - c.radius; c.vy = -c.vy * WALL_RESTITUTION; }
        }
    }

    // Detects and resolves collisions between every pair of active coins.
    // Uses the standard equal-mass elastic collision formula: velocity
    // components ALONG the line connecting the two centres get swapped;
    // the components perpendicular to that line are untouched. This is
    // the same physics idea behind billiard-ball collisions.
    void resolveCoinCollisions() {
        for (size_t i = 0; i < coins.size(); ++i) {
            if (!coins[i].active) continue;
            for (size_t j = i + 1; j < coins.size(); ++j) {
                if (!coins[j].active) continue;

                double dx = coins[j].x - coins[i].x;
                double dy = coins[j].y - coins[i].y;
                double dist = hypot(dx, dy);
                double minDist = coins[i].radius + coins[j].radius;

                if (dist < minDist && dist > 1e-9) {
                    // --- Separate overlapping coins first ---
                    double overlap = (minDist - dist) / 2.0;
                    double nx = dx / dist, ny = dy / dist; // unit normal from i to j
                    coins[i].x -= nx * overlap;
                    coins[i].y -= ny * overlap;
                    coins[j].x += nx * overlap;
                    coins[j].y += ny * overlap;

                    // --- Elastic collision response (equal masses) ---
                    double relVx = coins[i].vx - coins[j].vx;
                    double relVy = coins[i].vy - coins[j].vy;
                    double speedAlongNormal = relVx * nx + relVy * ny;

                    // Only resolve if they're actually moving toward each other,
                    // otherwise we'd incorrectly "collide" coins that are
                    // separating (e.g. right after a previous bounce).
                    if (speedAlongNormal > 0) {
                        coins[i].vx -= speedAlongNormal * nx;
                        coins[i].vy -= speedAlongNormal * ny;
                        coins[j].vx += speedAlongNormal * nx;
                        coins[j].vy += speedAlongNormal * ny;
                    }
                }
            }
        }
    }

    vector<pair<double,double>> pocketCenters() const {
        double m = POCKET_INSET;
        return {
            {m, m}, {FIELD_SIZE - m, m},
            {m, FIELD_SIZE - m}, {FIELD_SIZE - m, FIELD_SIZE - m}
        };
    }

    // Checks every active coin against every pocket. Returns true if
    // anything was potted this call. Scoring and fouls are applied here.
    bool resolvePocketing() {
        bool pottedAny = false;
        auto pockets = pocketCenters();

        for (auto& c : coins) {
            if (!c.active) continue;
            for (auto& p : pockets) {
                double dist = hypot(c.x - p.first, c.y - p.second);
                if (dist < POCKET_RADIUS) {
                    c.active = false;
                    pottedAny = true;
                    applyPottingRules(c.type);
                    break;
                }
            }
        }
        return pottedAny;
    }

    void applyPottingRules(CoinType potted) {
        switch (potted) {
            case CoinType::WHITE:
                if (whitesTurn) whiteScore++; else blackScore++;
                // (Simplified: potting the opponent's colour still scores
                // for whoever is shooting, rather than being a pure foul —
                // a deliberate simplification noted at the top of the file.)
                break;
            case CoinType::BLACK:
                if (whitesTurn) whiteScore++; else blackScore++;
                break;
            case CoinType::QUEEN:
                // Simplified rule: instant +3 bonus (real Carrom requires
                // "covering" the queen with your very next scoring shot).
                if (whitesTurn) whiteScore += 3; else blackScore += 3;
                break;
            case CoinType::STRIKER:
                // Foul: lose a point (simplified penalty), striker respawns
                // automatically at the start of the next turn.
                if (whitesTurn) whiteScore = max(0, whiteScore - 1);
                else             blackScore = max(0, blackScore - 1);
                break;
        }
    }

    bool allCoinsStopped() const {
        for (auto& c : coins) {
            if (!c.active) continue;
            if (hypot(c.vx, c.vy) > 0.0) return false;
        }
        return true;
    }

    void checkGameOver() {
        bool anyWhite = remainingCoinsOfType(CoinType::WHITE) > 0;
        bool anyBlack = remainingCoinsOfType(CoinType::BLACK) > 0;
        if (!anyWhite && !anyBlack) gameOver = true;
    }
};

// ----------------------------------------------------------------------------
//  Rendering — maps continuous field coordinates onto an ASCII grid.
// ----------------------------------------------------------------------------
const int GRID_SIZE = 32; // characters wide/tall

void renderFrame(const vector<Coin>& coins, int whiteScore, int blackScore, bool whitesTurn) {
    cout << "\033[H\033[2J";

    vector<vector<string>> grid(GRID_SIZE, vector<string>(GRID_SIZE, " "));

    double scale = GRID_SIZE / FIELD_SIZE;

    // Mark the 4 pockets first so coins draw on top of them if overlapping.
    double m = POCKET_INSET;
    vector<pair<double,double>> pockets = {
        {m, m}, {FIELD_SIZE - m, m}, {m, FIELD_SIZE - m}, {FIELD_SIZE - m, FIELD_SIZE - m}
    };
    for (auto& p : pockets) {
        int gx = (int)(p.first * scale), gy = (int)(p.second * scale);
        if (gx >= 0 && gx < GRID_SIZE && gy >= 0 && gy < GRID_SIZE)
            grid[gy][gx] = Color::GREY + "O" + Color::RESET;
    }

    for (auto& c : coins) {
        if (!c.active) continue;
        int gx = (int)(c.x * scale), gy = (int)(c.y * scale);
        if (gx >= 0 && gx < GRID_SIZE && gy >= 0 && gy < GRID_SIZE)
            grid[gy][gx] = coinGlyph(c.type);
    }

    cout << (whitesTurn ? Color::WHITE_C + "White's turn" + Color::RESET
                        : Color::BLACK_C + Color::BOLD + "Black's turn" + Color::RESET)
         << "   White: " << whiteScore << "   Black: " << blackScore << "\n";

    cout << "+" << string(GRID_SIZE, '-') << "+\n";
    for (int y = 0; y < GRID_SIZE; ++y) {
        cout << "|";
        for (int x = 0; x < GRID_SIZE; ++x) cout << grid[y][x];
        cout << "|\n";
    }
    cout << "+" << string(GRID_SIZE, '-') << "+\n";
}

// ============================================================================
//  ENTRY POINT
// ============================================================================
int main() {
    cout << Color::BOLD << Color::CYAN <<
R"(
   _____ _____ ____  ____   ____  __  __
  / ____/ ____/ __ \|  _ \ / __ \|  \/  |
 | |   | |   | |  | | |_) | |  | | \  / |
 | |   | |   | |  | |  _ <| |  | | |\/| |
 | |___| |___| |__| | |_) | |__| | |  | |
  \_____\_____\____/|____/ \____/|_|  |_|
)"
    << Color::RESET
    << Color::GREY << "  Simplified 2-player Carrom, with real 2D physics\n\n" << Color::RESET;

    cout << "White player pots white coins, Black player pots black coins.\n";
    cout << "The Queen is worth a bonus. Potting the striker itself is a foul.\n";
    cout << "You'll enter an angle (0-180 degrees, 90 = straight up the board),\n";
    cout << "a power (1-100), and where along your baseline to place the striker.\n\n";
    cout << "Press Enter to start...";
    cin.get();

    CarromGame game;

    while (!game.isGameOver()) {
        renderFrame(game.getCoins(), game.getWhiteScore(), game.getBlackScore(), game.isWhitesTurn());

        cout << "\n" << (game.isWhitesTurn() ? "White" : "Black") << "'s shot.\n";

        double baselineX, angle, power;
        cout << "Striker position along baseline (0-" << (int)FIELD_SIZE << "): ";
        cin >> baselineX;
        if (cin.eof()) { cout << "\nInput ended. Exiting.\n"; return 0; }
        cout << "Angle in degrees (0-180, 90=straight up): ";
        cin >> angle;
        if (cin.eof()) { cout << "\nInput ended. Exiting.\n"; return 0; }
        cout << "Power (1-100): ";
        cin >> power;
        if (cin.eof()) { cout << "\nInput ended. Exiting.\n"; return 0; }

        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input, try that shot again.\n";
            continue;
        }

        game.spawnStriker(baselineX);

        vector<vector<Coin>> frames;
        game.takeShot(angle, power, &frames);

        // Animate the shot frame by frame.
        for (auto& frame : frames) {
            renderFrame(frame, game.getWhiteScore(), game.getBlackScore(), !game.isWhitesTurn());
            this_thread::sleep_for(chrono::milliseconds(40));
        }
    }

    renderFrame(game.getCoins(), game.getWhiteScore(), game.getBlackScore(), game.isWhitesTurn());
    cout << "\n" << Color::BOLD << "Game over!\n" << Color::RESET;
    if (game.getWhiteScore() > game.getBlackScore()) cout << Color::WHITE_C << "White wins!\n" << Color::RESET;
    else if (game.getBlackScore() > game.getWhiteScore()) cout << Color::BLACK_C << Color::BOLD << "Black wins!\n" << Color::RESET;
    else cout << "It's a tie!\n";

    return 0;
}
