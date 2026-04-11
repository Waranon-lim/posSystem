#pragma once
#include <memory>
#include <string>

#include "../db/auth_repository.h"  // Include repo header for extern declaration

// UI layer - handles user input and output
std::string promptForUsername();
std::string promptForPassword();
void registerAccount();
void loginFlow();

// Global reference to repo - will be passed in from main
extern std::unique_ptr<AuthRepository> g_authRepo;
