#include "product_repository.h"
#include <stdexcept>

ProductRepository::ProductRepository(sqlite3* db) : db_(db) {}

// ── Helper: turn a prepared statement row into a Product struct ──────────
static Product row_to_product(sqlite3_stmt* stmt) {
    Product p;
    p.id       = sqlite3_column_int (stmt, 0);
    p.name     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    p.price    = sqlite3_column_double(stmt, 2);
    p.stock    = sqlite3_column_int   (stmt, 3);
    p.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    return p;
}

// ── INSERT ───────────────────────────────────────────────────────────────
Product ProductRepository::save(const Product& p) {
    const char* sql =
        "INSERT INTO products (name, price, stock, category) "
        "VALUES (?, ?, ?, ?);";

    // Step 1: Prepare — compile the SQL
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    // Step 2: Bind — safely plug in values (the ?'s above)
    // Bind positions are 1-indexed!
    sqlite3_bind_text  (stmt, 1, p.name.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, p.price);
    sqlite3_bind_int   (stmt, 3, p.stock);
    sqlite3_bind_text  (stmt, 4, p.category.c_str(), -1, SQLITE_TRANSIENT);

    // Step 3: Execute, then free the statement
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);  // always finalize — frees memory

    // Return the product with the auto-assigned ID
    Product saved = p;
    saved.id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    return saved;
}

// ── SELECT one ───────────────────────────────────────────────────────────
std::optional<Product> ProductRepository::find_by_id(int id) {
    const char* sql = "SELECT id, name, price, stock, category "
                      "FROM products WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    sqlite3_bind_int(stmt, 1, id);

    std::optional<Product> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)   // SQLITE_ROW means a row was found
        result = row_to_product(stmt);
    // if no row: result stays empty (std::nullopt)

    sqlite3_finalize(stmt);
    return result;
}

// ── SELECT all ───────────────────────────────────────────────────────────
std::vector<Product> ProductRepository::find_all() {
    const char* sql = "SELECT id, name, price, stock, category FROM products;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    std::vector<Product> products;
    while (sqlite3_step(stmt) == SQLITE_ROW)  // loop until no more rows
        products.push_back(row_to_product(stmt));

    sqlite3_finalize(stmt);
    return products;
}

// ── UPDATE ───────────────────────────────────────────────────────────────
bool ProductRepository::update(const Product& p) {
    const char* sql =
        "UPDATE products SET name=?, price=?, stock=?, category=? WHERE id=?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    sqlite3_bind_text  (stmt, 1, p.name.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, p.price);
    sqlite3_bind_int   (stmt, 3, p.stock);
    sqlite3_bind_text  (stmt, 4, p.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt, 5, p.id);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // sqlite3_changes() tells us how many rows were affected
    return sqlite3_changes(db_) > 0;
}

// ── DELETE ───────────────────────────────────────────────────────────────
bool ProductRepository::remove(int id) {
    const char* sql = "DELETE FROM products WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return sqlite3_changes(db_) > 0;
}