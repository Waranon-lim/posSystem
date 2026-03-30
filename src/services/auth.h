#pragma once
#include<string>
#include <sqlite3.h>

bool isNameExit(std::string username);
void registerAccount();
long long int generateHash(std::string password);
void initDatabase();

// -------------------------------login function -------------------------------

void loginFlow(sqlite3 *db);
bool loginUser(sqlite3* db, const std::string& username, const std::string& password);