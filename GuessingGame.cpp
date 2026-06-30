#include <iostream>
using namespace std;

int main() {
    int secret_number = 7; 
    int user_guess;

    cout << "Welcome to Bidhan's Game! Guess a number between 1 and 10: ";
    cin >> user_guess;

    if (user_guess == secret_number) {
        cout << "Congratulations! You won the game!" << endl;
    } else {
        cout << "Wrong guess! The secret number was " << secret_number << ". Try again!" << endl;
    }

    return 0;
}
