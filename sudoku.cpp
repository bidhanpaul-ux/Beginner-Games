#include <iostream>
#include <vector>

// 0 represents empty spaces
int board[9][9] = {
    {5, 3, 0, 0, 7, 0, 0, 0, 0},
    {6, 0, 0, 1, 9, 5, 0, 0, 0},
    {0, 9, 8, 0, 0, 0, 0, 6, 0},
    {8, 0, 0, 0, 6, 0, 0, 0, 3},
    {4, 0, 0, 8, 0, 3, 0, 0, 1},
    {7, 0, 0, 0, 2, 0, 0, 0, 6},
    {0, 6, 0, 0, 0, 0, 2, 8, 0},
    {0, 0, 0, 4, 1, 9, 0, 0, 5},
    {0, 0, 0, 0, 8, 0, 0, 7, 9}
};

// Tracks which cells were originally given so the player can't overwrite them
bool original[9][9];

void printBoard() {
    // Clear terminal screen for a smooth rendering look
    #ifdef _WIN32
        std::system("cls");
    #else
        std::system("clear");
    #endif

    std::cout << "      S U D O K U\n";
    std::cout << "  -------------------------\n";
    for (int r = 0; r < 9; ++r) {
        if (r > 0 && r % 3 == 0) {
            std::cout << "  |-------+-------+-------|\n";
        }
        std::cout << "  | ";
        for (int c = 0; c < 9; ++c) {
            if (c > 0 && c % 3 == 0) {
                std::cout << "| ";
            }
            if (board[r][c] == 0) {
                std::cout << ". ";
            } else {
                std::cout << board[r][c] << " ";
            }
        }
        std::cout << "|\n";
    }
    std::cout << "  -------------------------\n";
}

bool isSafe(int row, int col, int num) {
    // Check row and column
    for (int x = 0; x < 9; ++x) {
        if (board[row][x] == num || board[x][col] == num) {
            return false;
        }
    }

    // Check 3x3 box
    int startRow = row - row % 3;
    int startCol = col - col % 3;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i + startRow][j + startCol] == num) {
                return false;
            }
        }
    }
    return true;
}

bool checkWin() {
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (board[r][c] == 0) return false;
        }
    }
    return true;
}

int main() {
    // Initialize original board mask
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            original[r][c] = (board[r][r] != 0);
        }
    }

    while (true) {
        printBoard();

        if (checkWin()) {
            std::cout << "\nCONGRATULATIONS! You solved the puzzle!\n";
            break;
        }

        int row, col, val;
        std::cout << "\nEnter placement (Row Col Value) e.g., 1 3 4 (or 0 0 0 to quit): ";
        if (!(std::cin >> row >> col >> val)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        // Quit condition
        if (row == 0 && col == 0 && val == 0) break;

        // Convert 1-based indexing to 0-based indexing for arrays
        row--; col--;

        // Validating range bounds
        if (row < 0 || row > 8 || col < 0 || col > 8 || val < 1 || val > 9) {
            std::cout << "\n[!] Invalid input ranges (1-9 only). Press Enter...";
            std::cin.ignore(); std::cin.get();
            continue;
        }

        // Prevent modifying starting numbers
        if (original[row][col]) {
            std::cout << "\n[!] Cannot alter the default puzzle hints! Press Enter...";
            std::cin.ignore(); std::cin.get();
            continue;
        }

        // Validate Sudoku placement rules
        if (isSafe(row, col, val)) {
            board[row][col] = val;
        } else {
            std::cout << "\n[!] Move violates Sudoku rules! Press Enter...";
            std::cin.ignore(); std::cin.get();
        }
    }

    return 0;
}
