#include "auth_ui.h"

#include <sqlite3.h>

#include <iostream>

#include "../services/auth.h"

// Global reference to repo - will be passed in from main
extern AuthRepository* g_authRepo;

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
  std::string username, password, confirmPassword;
  bool nameExists = false;

  do {
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "  TIP: type 'back' to return to main menu " << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    username = promptForUsername();
    if (username == "back" || username == "Back") return;

    nameExists = g_authRepo->isUserExists(username);
    if (nameExists) {
      std::cout << "This name already exists, please try again." << std::endl;
    }
  } while (nameExists);

  do {
    password = promptForPassword();
    std::cout << "Confirm your password : ";
    std::cin >> confirmPassword;

    if (password != confirmPassword) {
      std::cout << "Passwords do not match, please try again." << std::endl;
    }
  } while (password != confirmPassword);

  // Call service layer
  if (registerUserService(username, password, *g_authRepo)) {
    std::cout << "Success! Your account has been created: " << username
              << std::endl;
  } else {
    std::cout << "Registration failed. Please try again." << std::endl;
  }
}

void loginFlow() {
  bool loginSuccess = false;
  std::string username, password;

  while (!loginSuccess) {
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "  TIP: type 'back' to return to main menu " << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "----- login page -----" << std::endl;

    username = promptForUsername();
    if (username == "back" || username == "Back") return;

    password = promptForPassword();

    // Call service layer
    if (loginUserService(username, password, *g_authRepo)) {
      std::cout << "welcome " << username << std::endl;
      loginSuccess = true;
    } else {
      std::cout << "username or password incorrect" << std::endl;
    }
  }
}
