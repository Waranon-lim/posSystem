#include "auth_ui.h"

#include <iostream>

#include "../services/auth.h"
#include "ui/Dashboard_ui.h"

// Global reference to repo - defined in main.cpp
extern std::unique_ptr<AuthRepository> g_authRepo;

namespace {
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
    printAuthHint();
    username = promptForUsername();
    if (isBackCommand(username)) {
      return;
    }

    if (isUserExistsService(username, *g_authRepo)) {
      std::cout << "This name already exists, please try again." << std::endl;
      continue;
    }

    password = promptForPassword();
    std::cout << "Confirm your password : ";
    std::cin >> confirmPassword;

    if (password != confirmPassword) {
      std::cout << "Passwords do not match, please try again." << std::endl;
      continue;
    }

    if (registerUserService(username, password, *g_authRepo)) {
      std::cout << "Success! Your account has been created: " << username
                << std::endl;
      return;
    }

    std::cout << "Registration failed. Please try again." << std::endl;
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
      displayMainMenu(username);
    } else {
      std::cout << "username or password incorrect" << std::endl;
    }
  }
}
