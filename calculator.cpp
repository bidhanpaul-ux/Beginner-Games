#include <iostream>
using namespace std;

double add(double a, double b) {
    return a + b;
}

double subtract(double a, double b) {
    return a - b;
}

double multiply(double a, double b) {
    return a * b;
}

double divide(double a, double b, bool &success) {
    if (b == 0) {
        success = false;
        return 0; 
    }
    success = true;
    return a / b;
}

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

    
        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input. Please enter a number from the menu.\n";
            continue;
        }

        
        if (option == 5) {
            cout << "Goodbye!\n";
            break;
        }

       
        if (option < 1 || option > 4) {
            cout << "Invalid option. Please choose a number between 1 and 5.\n";
            continue;
        }

      
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
        bool divideSuccess = true;

       
        switch (option) {
            case 1:
                result = add(num1, num2);
                cout << num1 << " + " << num2 << " = " << result << "\n";
                break; 

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
    
                cout << "Something went wrong.\n";
                break;
        }
    }

    return 0;
}
