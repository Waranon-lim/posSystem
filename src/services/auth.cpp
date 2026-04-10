#include "auth.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>

long long int generateHash(std::string password) {
  long int hash = 0;
  long int prime = 31;
  for (int i = 0; i < password.length(); i++) {
    hash = (hash * prime) + password[i];
  }
  return hash;
}

void initDatabase(sqlite3*& db) {
  char* errorMessage;
  if (sqlite3_open("db/user.db", &db) == SQLITE_OK) {
    std::string sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "password_hash TEXT NOT NULL, "
        "role TEXT DEFAULT 'customer', "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

    int exit = sqlite3_exec(db, sql.c_str(), NULL, 0, &errorMessage);

    if (exit != SQLITE_OK) {
      std::cerr << "Error Creating Table: " << errorMessage << std::endl;
      sqlite3_free(errorMessage);
    }
  }
}

// Service layer: Handles business logic
bool registerUserService(const std::string& username,
                         const std::string& password, AuthRepository& repo) {
  // Validation
  if (username.empty() || password.empty()) {
    return false;
  }

  // Check if user already exists
  if (repo.isUserExists(username)) {
    return false;
  }

  // Hash password and save
  long long int hashed_pw = generateHash(password);
  return repo.saveUser(username, hashed_pw);
}

bool loginUserService(const std::string& username, const std::string& password,
                      AuthRepository& repo) {
  if (username.empty() || password.empty()) {
    return false;
  }

  long long int hashed_pw = generateHash(password);
  return repo.authenticateUser(username, hashed_pw);
}
