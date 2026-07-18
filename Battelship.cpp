/*
 * ============================================================================
 *  BATTLESHIP  —  C++ Console Edition, vs a Hunt-and-Target AI
 * ============================================================================
 *  Place your fleet, then take turns firing at each other's grid. Neither
 *  side can see the other's ships — you're both firing blind, and the
 *  computer actually has to search for your ships rather than just react
 *  to something it can see (unlike tic-tac-toe or Connect Four, where the
 *  whole board is visible to both players).
 *
 *  SIMPLIFICATIONS (worth mentioning in your README):
 *      - 4 ships (sizes 5, 4, 3, 2) instead of the official 5.
 *      - An 8x8 grid instead of the official 10x10.
 *      - No manual ship rotation preview — you just type an orientation.
 *
 *  New concepts compared to your earlier games:
 *      - Hidden information       (each side has a board the OTHER side
 *                                   can't see — two separate grids, not one
 *                                   shared board like tic-tac-toe)
 *      - A "Hunt and Target" AI    (a genuinely famous, real Battleship
 *                                   strategy: fire randomly until you hit
 *                                   something, then methodically search the
 *                                   cells right next to that hit instead of
 *                                   going back to firing randomly)
 *      - Random ship placement     (trying random positions until one
 *                                   happens to fit without overlapping)
 *
 *  Compile:  g++ -std=c++17 -O2 -Wall -o battleship battleship.cpp
 *  Run:      ./battleship
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

const int SIZE = 8;
const vector<int> SHIP_SIZES = {5, 4, 3, 2};

enum class CellState { EMPTY, SHIP, HIT, MISS };
enum class AttackResult { MISS, HIT, SUNK, ALREADY_TRIED, INVALID };

namespace Color {
    const string RESET = "\033[0m";
    const string BOLD  = "\033[1m";
    const string BLUE  = "\033[1;34m";
    const string GREY  = "\033[1;30m";
    const string RED   = "\033[1;31m";
    const string GREEN = "\033[1;32m";
    const string YELLOW= "\033[1;33m";
}

struct Ship {
    vector<pair<int,int>> cells;
    set<pair<int,int>> hits;

    bool occupies(int r, int c) const {
        for (auto& cell : cells) if (cell.first == r && cell.second == c) return true;
        return false;
    }
    bool isSunk() const { return hits.size() == cells.size(); }
};

// ============================================================================
//  BOARD CLASS
//  One player's grid: their ships, and the record of shots fired AT them.
// ============================================================================
class Board {
public:
    Board() {
        grid.assign(SIZE, vector<CellState>(SIZE, CellState::EMPTY));
    }

    bool inBounds(int r, int c) const { return r >= 0 && r < SIZE && c >= 0 && c < SIZE; }

    // Checks whether a ship of the given length CAN be placed here, without
    // actually placing it — used both for validating player input and for
    // the computer's random-placement attempts.
    bool canPlaceShip(int row, int col, int length, bool horizontal) const {
        for (int i = 0; i < length; i++) {
            int r = horizontal ? row : row + i;
            int c = horizontal ? col + i : col;
            if (!inBounds(r, c)) return false;
            if (grid[r][c] == CellState::SHIP) return false; // no overlapping ships
        }
        return true;
    }

    bool placeShip(int row, int col, int length, bool horizontal) {
        if (!canPlaceShip(row, col, length, horizontal)) return false;

        Ship ship;
        for (int i = 0; i < length; i++) {
            int r = horizontal ? row : row + i;
            int c = horizontal ? col + i : col;
            grid[r][c] = CellState::SHIP;
            ship.cells.push_back({r, c});
        }
        ships.push_back(ship);
        return true;
    }

    // Randomly places every ship in SHIP_SIZES, retrying with new random
    // positions whenever a placement doesn't fit. This is a common, simple
    // strategy for random layout generation: it's not the most efficient
    // possible way, but for a grid this small it finds a valid layout
    // almost immediately, and it's easy to understand.
    void placeShipsRandomly() {
        for (int length : SHIP_SIZES) {
            bool placed = false;
            while (!placed) {
                int row = rand() % SIZE;
                int col = rand() % SIZE;
                bool horizontal = (rand() % 2 == 0);
                placed = placeShip(row, col, length, horizontal);
            }
        }
    }

    // Fires at (row, col). Returns what happened, and records the shot so
    // the same cell can't be attacked twice.
    AttackResult receiveAttack(int row, int col) {
        if (!inBounds(row, col)) return AttackResult::INVALID;

        if (grid[row][col] == CellState::HIT || grid[row][col] == CellState::MISS) {
            return AttackResult::ALREADY_TRIED;
        }

        if (grid[row][col] == CellState::SHIP) {
            grid[row][col] = CellState::HIT;
            for (auto& ship : ships) {
                if (ship.occupies(row, col)) {
                    ship.hits.insert({row, col});
                    if (ship.isSunk()) return AttackResult::SUNK;
                    return AttackResult::HIT;
                }
            }
        }

        grid[row][col] = CellState::MISS;
        return AttackResult::MISS;
    }

    bool allShipsSunk() const {
        for (auto& ship : ships) if (!ship.isSunk()) return false;
        return true;
    }

    // revealShips: true shows your OWN board (ships visible), false is
    // how your opponent's board should look to you (ships hidden — you
    // only see the hits/misses you've actually scored).
    void render(bool revealShips) const {
        cout << "   ";
        for (int c = 0; c < SIZE; c++) cout << (char)('A' + c) << " ";
        cout << "\n";

        for (int r = 0; r < SIZE; r++) {
            cout << (r + 1 < 10 ? " " : "") << (r + 1) << " ";
            for (int c = 0; c < SIZE; c++) {
                switch (grid[r][c]) {
                    case CellState::HIT:   cout << Color::RED << "X" << Color::RESET << " "; break;
                    case CellState::MISS:  cout << Color::GREY << "o" << Color::RESET << " "; break;
                    case CellState::SHIP:
                        cout << (revealShips ? Color::BLUE + "#" + Color::RESET : ".") << " ";
                        break;
                    default: cout << ". "; break;
                }
            }
            cout << "\n";
        }
    }

    int shipsRemaining() const {
        int count = 0;
        for (auto& ship : ships) if (!ship.isSunk()) count++;
        return count;
    }

private:
    vector<vector<CellState>> grid;
    vector<Ship> ships;
};

// ============================================================================
//  HUNTANDTARGET AI CLASS
//  Implements the classic Battleship strategy real players use:
//    - HUNT mode: fire at random untried cells.
//    - TARGET mode: once a shot hits, switch to methodically checking the
//      cells directly above/below/left/right of that hit, since a ship
//      must extend in some direction from there.
// ============================================================================
class HuntAndTargetAI {
public:
    // Decides the next cell to fire at.
    pair<int,int> chooseTarget() {
        // Prefer the target queue (cells adjacent to a previous hit) —
        // finishing off a ship we've already found is always better than
        // firing randomly elsewhere.
        while (!targetQueue.empty()) {
            pair<int,int> candidate = targetQueue.front();
            targetQueue.pop();
            if (!tried.count(candidate)) return candidate;
        }

        // Nothing queued — hunt randomly among cells not yet tried.
        while (true) {
            pair<int,int> candidate = {rand() % SIZE, rand() % SIZE};
            if (!tried.count(candidate)) return candidate;
        }
    }

    // Tells the AI what happened after firing at `cell`, so it can decide
    // how to respond on its NEXT turn.
    void notifyResult(pair<int,int> cell, AttackResult result) {
        tried.insert(cell);

        if (result == AttackResult::HIT) {
            // A hit but not a sink — the rest of the ship must be in one
            // of the 4 directions from here. Queue up all 4 neighbours;
            // invalid or already-tried ones get skipped when we dequeue.
            int r = cell.first, c = cell.second;
            for (auto& d : vector<pair<int,int>>{{1,0},{-1,0},{0,1},{0,-1}}) {
                pair<int,int> neighbour = {r + d.first, c + d.second};
                if (neighbour.first >= 0 && neighbour.first < SIZE &&
                    neighbour.second >= 0 && neighbour.second < SIZE &&
                    !tried.count(neighbour)) {
                    targetQueue.push(neighbour);
                }
            }
        } else if (result == AttackResult::SUNK) {
            // Simplified: once a ship is fully sunk, clear the queue and
            // return to hunting randomly. (A more advanced AI would keep
            // any queued cells that couldn't have belonged to the sunk
            // ship, in case they belong to a DIFFERENT nearby ship — this
            // is a deliberate simplification for a first version.)
            targetQueue = {};
        }
    }

private:
    set<pair<int,int>> tried;
    queue<pair<int,int>> targetQueue;
};

// ----------------------------------------------------------------------------
//  Input helpers
// ----------------------------------------------------------------------------
bool parseCoordinate(const string& input, int& row, int& col) {
    if (input.size() < 2) return false;
    char colChar = toupper(input[0]);
    if (colChar < 'A' || colChar >= 'A' + SIZE) return false;

    string rowPart = input.substr(1);
    for (char ch : rowPart) if (!isdigit(ch)) return false;

    int rowNum = stoi(rowPart);
    if (rowNum < 1 || rowNum > SIZE) return false;

    col = colChar - 'A';
    row = rowNum - 1;
    return true;
}

void placeShipsInteractively(Board& board) {
    cout << "\nPlace your fleet! Enter coordinates like B3, and orientation H or V.\n";
    for (int length : SHIP_SIZES) {
        while (true) {
            cout << "Place a ship of length " << length << " — start coordinate: ";
            string coordInput;
            cin >> coordInput;
            int row, col;
            if (!parseCoordinate(coordInput, row, col)) {
                cout << "Invalid coordinate. Use a letter A-" << (char)('A' + SIZE - 1)
                     << " followed by a number 1-" << SIZE << ".\n";
                continue;
            }

            cout << "Orientation (H = horizontal, V = vertical): ";
            char orientation;
            cin >> orientation;
            bool horizontal = (toupper(orientation) == 'H');

            if (board.placeShip(row, col, length, horizontal)) {
                board.render(true);
                break;
            } else {
                cout << "That doesn't fit there (out of bounds or overlapping). Try again.\n";
            }
        }
    }
}

// ============================================================================
//  ENTRY POINT
// ============================================================================
int main() {
    srand(static_cast<unsigned int>(time(0)));

    cout << Color::BOLD << Color::BLUE <<
R"(
  ____        _   _   _           _     _
 |  _ \      | | | | | |         | |   (_)
 | |_) | __ _| |_| |_| | ___  ___| |__  _ _ __
 |  _ < / _` | __| __| |/ _ \/ __| '_ \| | '_ \
 | |_) | (_| | |_| |_| |  __/\__ \ | | | | |_) |
 |____/ \__,_|\__|\__|_|\___||___/_| |_|_| .__/
                                          | |
                                          |_|
)"
    << Color::RESET << Color::GREY << "  vs. a Hunt-and-Target AI\n\n" << Color::RESET;

    Board playerBoard;
    Board computerBoard;
    HuntAndTargetAI ai;

    cout << "Would you like to place your ships manually, or randomly? (m/r): ";
    char placementChoice;
    cin >> placementChoice;

    if (tolower(placementChoice) == 'm') {
        placeShipsInteractively(playerBoard);
    } else {
        playerBoard.placeShipsRandomly();
        cout << "Your ships have been placed randomly:\n";
        playerBoard.render(true);
    }

    computerBoard.placeShipsRandomly();

    cout << "\nAll ships placed. Let the battle begin!\n";
    cout << "Enter coordinates like B3 to fire at the computer's board.\n";

    while (true) {
        cout << "\n" << Color::BOLD << "Your board:" << Color::RESET << "\n";
        playerBoard.render(true);
        cout << "\n" << Color::BOLD << "Computer's board (your view):" << Color::RESET << "\n";
        computerBoard.render(false);

        // ---- Player's turn ----
        cout << "\nYour shot: ";
        string input;
        if (!(cin >> input)) { cout << "\nInput ended. Exiting.\n"; return 0; }

        int row, col;
        if (!parseCoordinate(input, row, col)) {
            cout << "Invalid coordinate.\n";
            continue;
        }

        AttackResult result = computerBoard.receiveAttack(row, col);
        switch (result) {
            case AttackResult::HIT:   cout << Color::YELLOW << "Hit!\n" << Color::RESET; break;
            case AttackResult::SUNK:  cout << Color::RED << Color::BOLD << "You sunk a ship!\n" << Color::RESET; break;
            case AttackResult::MISS:  cout << "Miss.\n"; break;
            case AttackResult::ALREADY_TRIED: cout << "You already fired there — try a different cell.\n"; continue;
            case AttackResult::INVALID: cout << "That's off the board.\n"; continue;
        }

        if (computerBoard.allShipsSunk()) {
            cout << "\n" << Color::GREEN << Color::BOLD << "You win! All enemy ships sunk!\n" << Color::RESET;
            break;
        }

        // ---- Computer's turn ----
        pair<int,int> target = ai.chooseTarget();
        AttackResult compResult = playerBoard.receiveAttack(target.first, target.second);
        ai.notifyResult(target, compResult);

        string coordStr = string(1, (char)('A' + target.second)) + to_string(target.first + 1);
        switch (compResult) {
            case AttackResult::HIT:  cout << "\nComputer fires at " << coordStr << " — Hit!\n"; break;
            case AttackResult::SUNK: cout << "\nComputer fires at " << coordStr << " — Sunk your ship!\n"; break;
            case AttackResult::MISS: cout << "\nComputer fires at " << coordStr << " — Miss.\n"; break;
            default: break; // AI never fires invalid/repeat cells by construction
        }

        if (playerBoard.allShipsSunk()) {
            cout << "\n" << Color::RED << Color::BOLD << "The computer sank your entire fleet. You lose!\n" << Color::RESET;
            break;
        }
    }

    return 0;
}
