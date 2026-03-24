#include "services/menu.h"
#include <iostream>

using namespace std;

int main()
{
    showMenu();

    int choice = getChoice();
    switch (choice)
        {
        case 1:
            cout << "call register function";
            break;
        case 2:
            cout << "call login function";
            break;
        case 3:
            cout << "---your loging out---";
            return 0;
        }
    return 0;
}
