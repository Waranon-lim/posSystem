#include "services/menu.h"
#include "services/auth.h"
#include <iostream>

using namespace std;

int main()
{
    initDatabase();
    showMenu();
    int choice = getChoice();
    switch (choice)
    {
    case 1:
        registerAccount();
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
