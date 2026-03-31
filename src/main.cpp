#include "services/menu.h"
#include "services/auth.h"
#include <sqlite3.h>
#include <iostream>

using namespace std;

sqlite3 *db;
int main()
{
    initDatabase();
    while (true)
    {
        showMenu();
        int choice = getChoice();
        std::string username, password;
        switch (choice)
        {
        case 1:
            registerAccount();
            break;
        case 2:
          loginFlow(db);
            break;
        case 3:
            cout << "---your loging out---";
            sqlite3_close(db);
            return 0;
        }
    }
    return 0;
}
