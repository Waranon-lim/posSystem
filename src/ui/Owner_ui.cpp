#include <iostream>

#include "Dashboard_ui.h"

namespace {
void printDashboardHeader(const std::string& ownerName) {
  std::cout << "\n=== " << ownerName << " Dashboard ===" << std::endl;
  std::cout << "1. View daily report" << std::endl;
  std::cout << "2. Manage inventory" << std::endl;
  std::cout << "3. Logout" << std::endl;
}
}  // namespace

void displayMainMenu(const std::string& ownerName) {
  int choice = 0;

  while (true) {
    printDashboardHeader(ownerName);
    std::cout << "Select option: ";

    if (!(std::cin >> choice)) {
      std::cin.clear();
      std::cin.ignore(1000, '\n');
      std::cout << "Please enter a number." << std::endl;
      continue;
    }

    switch (choice) {
      case 1:
        std::cout << "Here is your report." << std::endl;
        break;
      case 2:
        std::cout << "Add your stock." << std::endl;
        break;
      case 3:
        std::cout << "You are logging out." << std::endl;
        return;
      default:
        std::cout << "Invalid choice, try again." << std::endl;
        break;
    }
  }
}