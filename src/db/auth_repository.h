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

  // password_hash is stored as INTEGER in SQLite and passed as sqlite3_int64.
  bool saveUser(const std::string& username, sqlite3_int64 passwordHash) const;

  // Verify user credentials
  bool authenticateUser(const std::string& username,
                        sqlite3_int64 passwordHash) const;

 private:
  sqlite3* db_;
};
