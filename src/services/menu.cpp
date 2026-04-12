#include "menu.h"

#include <iostream>

namespace {
void printMainMenu() {
  std::cout << "\n==========================================" << std::endl;
  std::cout << "1. Register" << std::endl;
  std::cout << "2. Login" << std::endl;
  std::cout << "3. Exit" << std::endl;
  std::cout << "==========================================" << std::endl;
  std::cout << "Choose your number: ";
}
}  // namespace

void showMenu() { printMainMenu(); }

int getChoice() {
  int num = 0;
  while (true) {
    if (std::cin >> num && num >= 1 && num <= 3) {
      return num;
    }

    std::cout << "Invalid choice. Please enter 1, 2, or 3." << std::endl;
    std::cin.clear();
    std::cin.ignore(1000, '\n');
    printMainMenu();
  }
}