#pragma once
#include <sqlite3.h>

#include <string>

// Repository layer - handles all database operations
class AuthRepository {
 public:
  explicit AuthRepository(sqlite3* db);

  // Check if username already exists
  bool isUserExists(const std::string& username);

  // Insert new user into database
  bool saveUser(const std::string& username, long long int passwordHash);

  // Verify user credentials
  bool authenticateUser(const std::string& username,
                        long long int passwordHash);

 private:
  sqlite3* db_;
};
