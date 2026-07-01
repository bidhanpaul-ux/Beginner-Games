#include <iostream>
using namespace std;

int main() {
    int secret_number = 7;
    int user_guess;
    int attempts = 3; // The player gets 3 lives

    cout << "Welcome to Bidhan's Upgraded Game!" << endl;

    // This is a WHILE loop. It repeats the code inside until attempts hit 0.
    while (attempts > 0) {
        cout << "\nYou have " << attempts << " attempts left. Enter your guess: ";
        cin >> user_guess;

        if (user_guess == secret_number) {
            cout << "Congratulations! You won the game!" << endl;
            return 0; // This stops the game immediately because the user won
        }
        else {
            cout << "Wrong guess!";
            attempts = attempts - 1; // Subtract 1 life
        }
    }

    // If the loop finishes and the user never guessed right:
    cout << "\nGame Over! You ran out of turns. The number was " << secret_number << "." << endl;
    return 0;
}
