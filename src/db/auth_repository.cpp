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
                              const std::string& password,
                              sqlite3_int64 passwordHash) const {
  // Backward compatibility: older DBs may still have legacy `password` column
  // with NOT NULL constraint. Insert both when legacy column exists.
  bool hasLegacyPassword = false;
  sqlite3_stmt* schemaStmt = nullptr;
  if (sqlite3_prepare_v2(db_, "PRAGMA table_info(users);", -1, &schemaStmt,
                         nullptr) == SQLITE_OK) {
    while (sqlite3_step(schemaStmt) == SQLITE_ROW) {
      const unsigned char* columnName = sqlite3_column_text(schemaStmt, 1);
      if (!columnName) continue;
      if (std::string(reinterpret_cast<const char*>(columnName)) ==
          "password") {
        hasLegacyPassword = true;
        break;
      }
    }
  }
  sqlite3_finalize(schemaStmt);

  // Use parameterized query to prevent SQL injection
  const char* sql =
      hasLegacyPassword
          ? "INSERT INTO users (username, password_hash, password) VALUES (?, "
            "?, ?);"
          : "INSERT INTO users (username, password_hash) VALUES (?, ?);";
  sqlite3_stmt* stmt;

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "SQL prepare error: " << sqlite3_errmsg(db_) << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, passwordHash);
  if (hasLegacyPassword) {
    sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_TRANSIENT);
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "SQL insert error: " << sqlite3_errmsg(db_) << std::endl;
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

std::string AuthRepository::authenticateUser(const std::string& username,
                                             sqlite3_int64 passwordHash) const {
  sqlite3_stmt* stmt;

  const char* sql =
      "SELECT role FROM users WHERE username = ? AND password_hash = ?;";

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "SQL prepare error: " << sqlite3_errmsg(db_) << std::endl;
    return "fail";
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, passwordHash);
  std::string role = "fail";

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char* roleText = sqlite3_column_text(stmt, 0);
    role = roleText ? reinterpret_cast<const char*>(roleText) : "customer";
  }

  sqlite3_finalize(stmt);
  return role;
}
