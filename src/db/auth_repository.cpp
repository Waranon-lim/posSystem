#include "auth_repository.h"

#include <iostream>
#include <sstream>

AuthRepository::AuthRepository(sqlite3* db) : db_(db) {}

bool AuthRepository::isUserExists(const std::string& username) {
  sqlite3_stmt* stmt;
  int count = 0;

  std::stringstream ss;
  ss << "SELECT COUNT(*) FROM users WHERE username = '" << username << "';";
  std::string query = ss.str();

  if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
    }
  }

  sqlite3_finalize(stmt);
  return (count > 0);
}

bool AuthRepository::saveUser(const std::string& username,
                              long long int passwordHash) {
  std::stringstream ss;
  ss << "INSERT INTO users (username, password) VALUES ('" << username << "', "
     << passwordHash << ");";

  std::string sql = ss.str();
  char* errorMessage = nullptr;

  int exit = sqlite3_exec(db_, sql.c_str(), NULL, 0, &errorMessage);

  if (exit != SQLITE_OK) {
    std::cerr << "SQL Error: " << errorMessage << std::endl;
    sqlite3_free(errorMessage);
    return false;
  }
  return true;
}

bool AuthRepository::authenticateUser(const std::string& username,
                                      long long int passwordHash) {
  sqlite3_stmt* stmt;

  std::string sql = "SELECT id FROM users WHERE username = ? AND password = ?;";

  if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 2, passwordHash);
  bool success = false;

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    success = true;
  }

  sqlite3_finalize(stmt);
  return success;
}
