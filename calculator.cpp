/*
    Simple Calculator in C++
    -------------------------
    A basic console calculator that keeps asking for calculations
    until the user chooses to exit.

    New concepts compared to the tic-tac-toe project:
      - switch statement       (cleaner than many if-else chains)
      - functions that RETURN a value (not just void functions)
      - basic error handling   (division by zero, invalid menu choice)
      - a "menu-driven" program structure (very common pattern)

    Compile:  g++ -o calculator calculator.cpp
    Run:      ./calculator
*/

#include <iostream>
using namespace std;

// ---- Functions that perform the actual math ----
// Each takes two doubles (decimal numbers) and returns a double result.

double add(double a, double b) {
    return a + b;
}

double subtract(double a, double b) {
    return a - b;
}

double multiply(double a, double b) {
    return a * b;
}

// Division needs extra care: dividing by zero is not allowed.
// We use a bool "output parameter" (passed by reference with &)
// to tell the caller whether the division actually succeeded.
double divide(double a, double b, bool &success) {
    if (b == 0) {
        success = false;
        return 0; // dummy value, won't be used since success is false
    }
    success = true;
    return a / b;
}

// Prints the menu of operations
void printMenu() {
    cout << "\n===== SIMPLE CALCULATOR =====\n";
    cout << "1. Add        (+)\n";
    cout << "2. Subtract   (-)\n";
    cout << "3. Multiply   (*)\n";
    cout << "4. Divide     (/)\n";
    cout << "5. Exit\n";
    cout << "==============================\n";
    cout << "Choose an option (1-5): ";
}

int main() {
    int option;
    bool running = true;

    cout << "Welcome to the Simple C++ Calculator!\n";

    while (running) {
        printMenu();
        cin >> option;

        // Handle non-numeric input (e.g. user types a letter)
        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input. Please enter a number from the menu.\n";
            continue;
        }

        // Exit option — check this before asking for numbers
        if (option == 5) {
            cout << "Goodbye!\n";
            break;
        }

        // Reject anything outside the valid menu range
        if (option < 1 || option > 4) {
            cout << "Invalid option. Please choose a number between 1 and 5.\n";
            continue;
        }

        // Ask for the two numbers to operate on
        double num1, num2;
        cout << "Enter first number: ";
        cin >> num1;
        cout << "Enter second number: ";
        cin >> num2;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid number entered. Returning to menu.\n";
            continue;
        }

        double result = 0;
        bool divideSuccess = true; // only relevant for option 4

        // switch works like a cleaner set of if-else checks,
        // when comparing ONE variable against several fixed values.
        switch (option) {
            case 1:
                result = add(num1, num2);
                cout << num1 << " + " << num2 << " = " << result << "\n";
                break; // break stops it from "falling through" to case 2

            case 2:
                result = subtract(num1, num2);
                cout << num1 << " - " << num2 << " = " << result << "\n";
                break;

            case 3:
                result = multiply(num1, num2);
                cout << num1 << " * " << num2 << " = " << result << "\n";
                break;

            case 4:
                result = divide(num1, num2, divideSuccess);
                if (divideSuccess) {
                    cout << num1 << " / " << num2 << " = " << result << "\n";
                } else {
                    cout << "Error: Cannot divide by zero!\n";
                }
                break;

            default:
                // Should never be reached because we already validated
                // "option" above, but it's good practice to have it.
                cout << "Something went wrong.\n";
                break;
        }
    }

    return 0;
}
