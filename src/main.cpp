#include <sqlite3.h>

#include <iostream>

#include "db/auth_repository.h"
#include "services/auth.h"
#include "services/menu.h"
#include "ui/auth_ui.h"

using namespace std;

sqlite3* db = nullptr;
AuthRepository* g_authRepo = nullptr;

int main() {
  initDatabase(db);
  g_authRepo = new AuthRepository(db);

  while (true) {
    showMenu();
    int choice = getChoice();
    switch (choice) {
      case 1:
        registerAccount();
        break;
      case 2:
        loginFlow();
        break;
      case 3:
        cout << "---your loging out---";
        if (g_authRepo != nullptr) delete g_authRepo;
        sqlite3_close(db);
        return 0;
    }
  }
  return 0;
}
