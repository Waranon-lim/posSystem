#pragma once
#include <sqlite3.h>

#include <string>

#include "../db/auth_repository.h"

// Service layer - handles business logic
void initDatabase(sqlite3*& db);
sqlite3_int64 generateHash(const std::string& password);

// Username check exposed to UI via service layer (UI -> Service -> Repository)
bool isUserExistsService(const std::string& username,
                         const AuthRepository& repo);

// Service functions called by UI layer
bool registerUserService(const std::string& username,
                         const std::string& password,
                         const AuthRepository& repo);
std::string loginUserService(const std::string& username,
                             const std::string& password,
                             const AuthRepository& repo);
