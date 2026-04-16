#include "auth_repository.h"

#include <iostream>
#include <stdexcept>

AuthRepository::AuthRepository(sqlite3* db) : db_(db) {
  if (!db_) {
    throw std::invalid_argument("Database pointer cannot be null");
  }
}

bool AuthRepository::isUserExists(const std::string& username) const {
  sqlite3_stmt* stmt;
  int count = 0;

  // Use parameterized query to prevent SQL injection
  const char* query = "SELECT COUNT(*) FROM users WHERE username = ?;";

  if (sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "SQL prepare error: " << sqlite3_errmsg(db_) << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return (count > 0);
}

bool AuthRepository::saveUser(const std::string& username,
                              sqlite3_int64 passwordHash) const {
  // Use parameterized query to prevent SQL injection
  const char* sql =
      "INSERT INTO users (username, password_hash) VALUES (?, ?);";
  sqlite3_stmt* stmt;

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "SQL prepare error: " << sqlite3_errmsg(db_) << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, passwordHash);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "SQL insert error: " << sqlite3_errmsg(db_) << std::endl;
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

bool AuthRepository::authenticateUser(const std::string& username,
                                      sqlite3_int64 passwordHash) const {
  sqlite3_stmt* stmt;

  const char* sql =
      "SELECT id FROM users WHERE username = ? AND password_hash = ?;";

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "SQL prepare error: " << sqlite3_errmsg(db_) << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, passwordHash);
  bool success = false;

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    success = true;
  }

  sqlite3_finalize(stmt);
  return success;
}
