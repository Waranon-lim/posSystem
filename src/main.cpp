#include "services/menu.h"
#include "services/auth.h"
#include <sqlite3.h>
#include <iostream>

using namespace std;

sqlite3 *db;
int main()
{
    while (true)
    {
        initDatabase();
        showMenu();
        int choice = getChoice();
        std::string username, password;
        switch (choice)
        {
        case 1:
            registerAccount();
            break;
        case 2:
            std::cout << "----- login page -----" << std::endl;
            std::cout << "enter your username : ";
            std::cin >> username;
            std::cout << "enter your password : ";
            std::cin >> password;
            if (loginUser(db, username, password))
            {
                std::cout << "welcome " << username << std::endl;
            }
            else
            {
                std::cout << "username or password incorrect" << std::endl;
            }
            break;
        case 3:
            cout << "---your loging out---";
            sqlite3_close(db);
            return 0;
        }
    }
    return 0;
}
