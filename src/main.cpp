#include <sqlite3.h>

#include <iostream>
#include <memory>

#include "db/auth_repository.h"
#include "services/auth.h"
#include "services/menu.h"
#include "ui/auth_ui.h"

using namespace std;

sqlite3* db = nullptr;
std::unique_ptr<AuthRepository> g_authRepo;  // Smart pointer: auto cleanup

int main() {
  initDatabase(db);
  if (!db) {
    std::cerr << "Failed to initialize database" << std::endl;
    return 1;
  }

  g_authRepo = std::make_unique<AuthRepository>(db);

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
        cout << "---your logging out---";
        g_authRepo.reset();  // Explicitly cleanup
        sqlite3_close(db);
        return 0;
    }
  }
  return 0;
}
