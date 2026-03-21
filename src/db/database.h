#pragma once
#include <sqlite3.h>
#include <string>
#include <stdexcept>

class Database {
public:
    // Opens (or creates) the .db file at the given path
    explicit Database(const std::string& path);
    ~Database();

    // No copying — only one owner of the connection
    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // Returns the raw handle — repositories will use this
    sqlite3* handle() const { return db_; }

    // Creates all tables (safe to call even if they already exist)
    void initialize_schema();

private:
    sqlite3* db_ = nullptr;
};