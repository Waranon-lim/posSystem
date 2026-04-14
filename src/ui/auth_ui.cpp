#include "auth_ui.h"

#include <cctype>
#include <iostream>

#include "../services/auth.h"
#include "ui/Dashboard_ui.h"

// Global reference to repo - defined in main.cpp
extern std::unique_ptr<AuthRepository> g_authRepo;

namespace {
void clearScreen() { std::cout << "\033[2J\033[1;1H"; }

void printAuthHint() {
  std::cout << "------------------------------------------" << std::endl;
  std::cout << "Type 'back' to return to the main menu." << std::endl;
  std::cout << "------------------------------------------" << std::endl;
}

bool isBackCommand(const std::string& input) {
  return input == "back" || input == "Back";
}
}  // namespace

std::string promptForUsername() {
  std::string username;
  std::cout << "Enter your username : ";
  std::cin >> username;
  return username;
}

std::string promptForPassword() {
  std::string password;
  std::cout << "Enter your password : ";
  std::cin >> password;
  return password;
}

void registerAccount() {
  if (!g_authRepo) {
    std::cerr << "Repository is not initialized." << std::endl;
    return;
  }

  std::string username, password, confirmPassword;
  while (true) {
    clearScreen();
    printAuthHint();
    username = promptForUsername();
    if (isBackCommand(username)) {
      return;
    }

    if (isUserExistsService(username, *g_authRepo)) {
      std::cout << "This name already exists, please try again." << std::endl;
      clearScreen();
      continue;
    }

    password = promptForPassword();
    std::cout << "Confirm your password : ";
    std::cin >> confirmPassword;

    if (password != confirmPassword) {
      std::cout << "Passwords do not match, please try again." << std::endl;
      clearScreen();
      continue;
    }

    if (registerUserService(username, password, *g_authRepo)) {
      std::cout << "Success! Your account has been created: " << username
                << std::endl;
      return;
    }

    std::cout << "Registration failed. Please try again." << std::endl;
    clearScreen();
  }
}

void loginFlow() {
  if (!g_authRepo) {
    std::cerr << "Repository is not initialized." << std::endl;
    return;
  }

  bool loginSuccess = false;
  std::string username, password;

  while (!loginSuccess) {
    clearScreen();
    printAuthHint();
    std::cout << "Login" << std::endl;

    username = promptForUsername();
    if (isBackCommand(username)) {
      return;
    }

    password = promptForPassword();

    std::string role = loginUserService(username, password, *g_authRepo);
    if (role != "fail") {
      std::cout << "welcome " << username << std::endl;
      loginSuccess = true;

      // Convert role to lowercase for comparison
      std::string lowerRole = role;
      for (char& c : lowerRole) {
        c = std::tolower(c);
      }

      if (lowerRole == "owner") {
        displayMainMenu(username);
      } else {
        displayCustomerService(username);
      }
    } else {
      std::cout << "username or password incorrect" << std::endl;
      clearScreen();
    }
  }
}
