#pragma once
#include <sqlite3.h>

#include <string>

#include "db/auth_repository.h"

// Service layer - handles business logic
void initDatabase(sqlite3*& db);
long long int generateHash(std::string password);

// Service functions called by UI layer
bool registerUserService(const std::string& username,
                         const std::string& password, AuthRepository& repo);
bool loginUserService(const std::string& username, const std::string& password,
                      AuthRepository& repo);
