#include <iostream>
using namespace std;

char board[10] = { '0','1','2','3','4','5','6','7','8','9' };
// Note: index 0 is unused (holds '0' as a placeholder), we use 1-9 to match player input.

void printBoard() {
    cout << "\n";
    cout << " " << board[1] << " | " << board[2] << " | " << board[3] << "\n";
    cout << "---+---+---\n";
    cout << " " << board[4] << " | " << board[5] << " | " << board[6] << "\n";
    cout << "---+---+---\n";
    cout << " " << board[7] << " | " << board[8] << " | " << board[9] << "\n\n";
}

// Checks if the given mark ('X' or 'O') has won
bool checkWin(char mark) {
    int wins[8][3] = {
        {1,2,3}, {4,5,6}, {7,8,9},   // rows
        {1,4,7}, {2,5,8}, {3,6,9},   // columns
        {1,5,9}, {3,5,7}             // diagonals
    };

    for (int i = 0; i < 8; i++) {
        int a = wins[i][0], b = wins[i][1], c = wins[i][2];
        if (board[a] == mark && board[b] == mark && board[c] == mark) {
            return true;
        }
    }
    return false;
}

bool isBoardFull() {
    for (int i = 1; i <= 9; i++) {
        if (board[i] != 'X' && board[i] != 'O') {
            return false; // still a number, so an empty cell exists
        }
    }
    return true;
}

// Simple computer move: try to win, else block, else pick first empty cell
int computerMove() {
    // 1. Check if computer can win
    for (int i = 1; i <= 9; i++) {
        if (board[i] != 'X' && board[i] != 'O') {
            char backup = board[i];
            board[i] = 'O';
            if (checkWin('O')) {
                board[i] = backup;
                return i;
            }
            board[i] = backup;
        }
    }

    // 2. Check if human is about to win, block them
    for (int i = 1; i <= 9; i++) {
        if (board[i] != 'X' && board[i] != 'O') {
            char backup = board[i];
            board[i] = 'X';
            if (checkWin('X')) {
                board[i] = backup;
                return i;
            }
            board[i] = backup;
        }
    }

    // 3. Otherwise, pick the first empty cell
    for (int i = 1; i <= 9; i++) {
        if (board[i] != 'X' && board[i] != 'O') {
            return i;
        }
    }

    return -1; // no move available (shouldn't happen)
}

int main() {
    cout << "=========================\n";
    cout << "   TIC TAC TOE GAME\n";
    cout << "=========================\n";
    cout << "You are X. Computer is O.\n";
    cout << "Enter a number 1-9 to place your mark:\n";

    printBoard();

    int choice;
    bool gameOver = false;

    while (!gameOver) {
        // ----- Human turn -----
        cout << "Your move (1-9): ";
        cin >> choice;

        if (cin.eof()) {
            cout << "\nNo more input detected. Exiting.\n";
            break;
        }
        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Please enter a valid number.\n";
            continue;
        }

        if (choice < 1 || choice > 9 || board[choice] == 'X' || board[choice] == 'O') {
            cout << "Invalid move, try again.\n";
            continue;
        }

        board[choice] = 'X';
        printBoard();

        if (checkWin('X')) {
            cout << "Congratulations! You win!\n";
            break;
        }
        if (isBoardFull()) {
            cout << "It's a draw!\n";
            break;
        }

        // ----- Computer turn -----
        cout << "Computer is making a move...\n";
        int compChoice = computerMove();
        board[compChoice] = 'O';
        printBoard();

        if (checkWin('O')) {
            cout << "Computer wins! Better luck next time.\n";
            break;
        }
        if (isBoardFull()) {
            cout << "It's a draw!\n";
            break;
        }
    }

    cout << "Thanks for playing!\n";
    return 0;
}
