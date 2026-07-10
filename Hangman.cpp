/*
    Hangman — C++ Console Game
    ----------------------------
    Guess the hidden word one letter at a time before the hangman drawing
    is completed (6 wrong guesses).

    New concepts compared to your earlier games:
      - std::string manipulation    (building the "_ _ _" display, checking
                                      if a letter is already guessed)
      - Working with a word list    (an array of strings, picked at random)
      - Multi-stage ASCII art       (drawing progressively worse pictures
                                      based on how many mistakes you've made)

    Compile:  g++ -o hangman hangman.cpp
    Run:      ./hangman
*/

#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
using namespace std;

// A small word bank to pick from. Feel free to add your own words later.
string wordList[] = {
    "PROGRAMMING", "COMPUTER", "KEYBOARD", "ALGORITHM", "FUNCTION",
    "VARIABLE", "COLLEGE", "SILIGURI", "ENGINEER", "PORTFOLIO",
    "RECURSION", "COMPILER", "DEBUGGING", "SOFTWARE", "NETWORK"
};
const int WORD_COUNT = 15;

// ASCII art for each stage of the hangman, from 0 wrong guesses up to 6.
// Index into this array using how many wrong guesses have been made.
string hangmanStages[] = {
    R"(
  +---+
  |   |
      |
      |
      |
      |
=========)",
    R"(
  +---+
  |   |
  O   |
      |
      |
      |
=========)",
    R"(
  +---+
  |   |
  O   |
  |   |
      |
      |
=========)",
    R"(
  +---+
  |   |
  O   |
 /|   |
      |
      |
=========)",
    R"(
  +---+
  |   |
  O   |
 /|\  |
      |
      |
=========)",
    R"(
  +---+
  |   |
  O   |
 /|\  |
 /    |
      |
=========)",
    R"(
  +---+
  |   |
  O   |
 /|\  |
 / \  |
      |
=========)"
};

// Builds the "P R _ G R A _" style display: revealed letters show, others are blanks.
string buildDisplay(const string& word, const string& guessedLetters) {
    string display = "";
    for (char letter : word) {
        bool revealed = (guessedLetters.find(letter) != string::npos);
        display += revealed ? letter : '_';
        display += ' ';
    }
    return display;
}

bool wordFullyGuessed(const string& word, const string& guessedLetters) {
    for (char letter : word) {
        if (guessedLetters.find(letter) == string::npos) return false;
    }
    return true;
}

void playRound() {
    string word = wordList[rand() % WORD_COUNT];
    string guessedLetters = "";   // every letter the player has tried, right or wrong
    int wrongGuesses = 0;
    const int MAX_WRONG = 6;

    cout << "\nGuess the word! It has " << word.size() << " letters.\n";

    while (wrongGuesses < MAX_WRONG) {
        cout << hangmanStages[wrongGuesses] << "\n\n";
        cout << "Word: " << buildDisplay(word, guessedLetters) << "\n";
        cout << "Guessed so far: " << (guessedLetters.empty() ? "(none)" : guessedLetters) << "\n";
        cout << "Wrong guesses: " << wrongGuesses << " / " << MAX_WRONG << "\n";

        if (wordFullyGuessed(word, guessedLetters)) {
            cout << "\nYou guessed it! The word was " << word << ". You win!\n";
            return;
        }

        cout << "Enter a letter: ";
        char input;
        cin >> input;
        input = toupper(input);

        if (!isalpha(input)) {
            cout << "Please enter a letter from A-Z.\n";
            continue;
        }

        if (guessedLetters.find(input) != string::npos) {
            cout << "You already guessed '" << input << "'. Try a different letter.\n";
            continue;
        }

        guessedLetters += input;

        if (word.find(input) == string::npos) {
            wrongGuesses++;
            cout << "'" << input << "' is not in the word.\n";
        } else {
            cout << "Good guess! '" << input << "' is in the word.\n";
        }
    }

    // If we exit the loop without returning, the player ran out of guesses.
    cout << hangmanStages[MAX_WRONG] << "\n\n";
    cout << "Out of guesses! The word was: " << word << "\n";
    cout << "Better luck next time!\n";
}

int main() {
    srand(static_cast<unsigned int>(time(0)));

    cout << "===== HANGMAN =====\n";
    cout << "Guess the hidden word one letter at a time.\n";
    cout << "Six wrong guesses and the hangman drawing is complete!\n";

    bool playAgain = true;
    while (playAgain) {
        playRound();

        cout << "\nPlay again? (y/n): ";
        char choice;
        cin >> choice;
        playAgain = (tolower(choice) == 'y');
    }

    cout << "\nThanks for playing!\n";
    return 0;
}
