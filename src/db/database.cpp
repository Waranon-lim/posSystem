 #include "database.h"

Database::Database(const std::string& path) {
    // SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE = open or create the file
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Cannot open database: " +
                                 std::string(sqlite3_errmsg(db_)));
    }
    // Enable foreign keys (SQLite disables them by default!)
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::initialize_schema() {
    // R"(...)" is a raw string literal — it lets you write multi-line SQL without escaping quotes or newlines. Very handy.
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS products (
            id       INTEGER PRIMARY KEY AUTOINCREMENT,
            name     TEXT    NOT NULL,
            price    REAL    NOT NULL,
            stock    INTEGER NOT NULL DEFAULT 0,
            category TEXT    NOT NULL DEFAULT ''
        );

        CREATE TABLE IF NOT EXISTS sales (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp      TEXT    NOT NULL,
            total          REAL    NOT NULL,
            payment_method TEXT    NOT NULL
        );

        CREATE TABLE IF NOT EXISTS sale_items (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            sale_id    INTEGER NOT NULL REFERENCES sales(id),
            product_id INTEGER NOT NULL REFERENCES products(id),
            quantity   INTEGER NOT NULL,
            unit_price REAL    NOT NULL
        );
    )";

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::string err(errmsg);
        sqlite3_free(errmsg);
        throw std::runtime_error("Schema init failed: " + err);
    }
}