#pragma once
#include <sqlite3.h>

#include <string>

// Repository layer - handles all database operations
class AuthRepository {
 public:
  explicit AuthRepository(sqlite3* db);
  ~AuthRepository() {}  // No cleanup needed, db managed by caller

  // Check if username already exists
  bool isUserExists(const std::string& username) const;

  // In legacy schema, `password` stores user input and `password_hash` stores
  // hash.
  bool saveUser(const std::string& username, const std::string& password,
                sqlite3_int64 passwordHash) const;

  // Verify user credentials and return role when success, or "fail".
  std::string authenticateUser(const std::string& username,
                               sqlite3_int64 passwordHash) const;

 private:
  sqlite3* db_;
};
