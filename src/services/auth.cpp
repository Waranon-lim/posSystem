#include "auth.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>

// ⚠️ WARNING: Simple hash for learning purposes only.
// For production, use bcrypt or argon2!
sqlite3_int64 generateHash(const std::string& password) {
  sqlite3_int64 hash = 0;
  const sqlite3_int64 prime = 31;
  for (size_t i = 0; i < password.length(); i++) {
    hash = (hash * prime) + password[i];
  }
  return hash;
}

bool isUserExistsService(const std::string& username,
                         const AuthRepository& repo) {
  if (username.empty()) {
    return false;
  }
  return repo.isUserExists(username);
}

void initDatabase(sqlite3*& db) {
  char* errorMessage = nullptr;  // Initialize to null to prevent double-free
  if (sqlite3_open("db/user.db", &db) == SQLITE_OK) {
    std::string sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "password_hash INTEGER NOT NULL, "
        "role TEXT DEFAULT 'customer', "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

    int result = sqlite3_exec(db, sql.c_str(), NULL, 0, &errorMessage);

    if (result != SQLITE_OK) {
      std::cerr << "Error Creating Table: " << errorMessage << std::endl;
      sqlite3_free(errorMessage);
    }
  }
}

// Service layer: Handles business logic
bool registerUserService(const std::string& username,
                         const std::string& password,
                         const AuthRepository& repo) {
  // Validation
  if (username.empty() || password.empty()) {
    return false;
  }

  // Check if user already exists
  if (repo.isUserExists(username)) {
    return false;
  }

  // Hash password and save
  sqlite3_int64 passwordHash = generateHash(password);
  return repo.saveUser(username, passwordHash);
}

bool loginUserService(const std::string& username, const std::string& password,
                      const AuthRepository& repo) {
  if (username.empty() || password.empty()) {
    return false;
  }

  sqlite3_int64 passwordHash = generateHash(password);
  return repo.authenticateUser(username, passwordHash);
}
