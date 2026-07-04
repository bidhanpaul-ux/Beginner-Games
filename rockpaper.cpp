/*
    Rock, Paper, Scissors — C++ Console Game
    ------------------------------------------
    Play against the computer for as many rounds as you like.

    New concepts compared to tic-tac-toe and the calculator:
      - enum                (giving names to a fixed set of options)
      - rand() / srand()    (generating random numbers for the computer)
      - comparing two enum values to decide a winner

    Compile:  g++ -o rps rps.cpp
    Run:      ./rps
*/

#include <iostream>
#include <ctime>    // needed for time(), used to seed the random generator
#include <cstdlib>  // needed for rand() and srand()
using namespace std;

// An enum gives readable names to a fixed set of values instead of
// using plain numbers like 0, 1, 2 everywhere (which is easy to mix up).
enum Choice {
    ROCK = 1,
    PAPER = 2,
    SCISSORS = 3
};

// Converts a Choice into text for printing
string choiceToString(Choice c) {
    switch (c) {
        case ROCK:     return "Rock";
        case PAPER:    return "Paper";
        case SCISSORS: return "Scissors";
        default:       return "Unknown";
    }
}

// Computer randomly picks Rock, Paper, or Scissors
Choice getComputerChoice() {
    int randomValue = (rand() % 3) + 1; // rand() % 3 gives 0, 1, or 2 -> +1 makes it 1, 2, or 3
    return static_cast<Choice>(randomValue);
    // static_cast<Choice> converts the plain int into our enum type
}

// Returns:  1 if player wins, -1 if computer wins, 0 if it's a tie
int decideWinner(Choice player, Choice computer) {
    if (player == computer) {
        return 0; // same choice = tie
    }

    // Check every case where the PLAYER wins
    if ((player == ROCK && computer == SCISSORS) ||
        (player == PAPER && computer == ROCK) ||
        (player == SCISSORS && computer == PAPER)) {
        return 1;
    }

    // Otherwise the computer must have won
    return -1;
}

void printMenu() {
    cout << "\n1. Rock\n";
    cout << "2. Paper\n";
    cout << "3. Scissors\n";
    cout << "4. Quit\n";
    cout << "Enter your choice (1-4): ";
}

int main() {
    // Seed the random number generator using the current time,
    // so you get different computer choices each time you run the program.
    // (Without this, rand() would produce the same sequence every run.)
    srand(static_cast<unsigned int>(time(0)));

    int wins = 0, losses = 0, ties = 0;
    bool playing = true;

    cout << "===== ROCK, PAPER, SCISSORS =====\n";
    cout << "Beat the computer's random choice!\n";

    while (playing) {
        printMenu();

        int input;
        cin >> input;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Please enter a number from the menu.\n";
            continue;
        }

        if (input == 4) {
            cout << "Thanks for playing!\n";
            break;
        }

        if (input < 1 || input > 3) {
            cout << "Invalid choice, try again.\n";
            continue;
        }

        Choice playerChoice = static_cast<Choice>(input);
        Choice computerChoice = getComputerChoice();

        cout << "You chose:      " << choiceToString(playerChoice) << "\n";
        cout << "Computer chose: " << choiceToString(computerChoice) << "\n";

        int result = decideWinner(playerChoice, computerChoice);

        if (result == 0) {
            cout << "It's a tie!\n";
            ties++;
        } else if (result == 1) {
            cout << "You win this round!\n";
            wins++;
        } else {
            cout << "Computer wins this round!\n";
            losses++;
        }

        cout << "Score -> Wins: " << wins << "  Losses: " << losses << "  Ties: " << ties << "\n";
    }

    cout << "\nFinal Score -> Wins: " << wins << "  Losses: " << losses << "  Ties: " << ties << "\n";
    return 0;
}
