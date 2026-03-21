#include "sale_repository.h"
#include <stdexcept>

SaleRepository::SaleRepository(sqlite3* db) : db_(db) {}

// ── Helper: build a Sale (header only, no items) from a statement row ────
static Sale row_to_sale(sqlite3_stmt* stmt) {
    Sale s;
    s.id             = sqlite3_column_int (stmt, 0);
    s.timestamp      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    s.total          = sqlite3_column_double(stmt, 2);
    s.payment_method = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    return s;
}

// ── Private: INSERT one SaleItem row ─────────────────────────────────────
SaleItem SaleRepository::save_item(const SaleItem& item) {
    const char* sql =
        "INSERT INTO sale_items (sale_id, product_id, quantity, unit_price) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    sqlite3_bind_int   (stmt, 1, item.sale_id);
    sqlite3_bind_int   (stmt, 2, item.product_id);
    sqlite3_bind_int   (stmt, 3, item.quantity);
    sqlite3_bind_double(stmt, 4, item.unit_price);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    SaleItem saved  = item;
    saved.id        = static_cast<int>(sqlite3_last_insert_rowid(db_));
    return saved;
}

// ── INSERT sale + all its items (wrapped in a transaction) ───────────────
Sale SaleRepository::save(const Sale& sale) {
    // Start transaction — nothing hits the DB until COMMIT
    sqlite3_exec(db_, "BEGIN;", nullptr, nullptr, nullptr);

    try {
        // 1. Insert the sale header
        const char* sql =
            "INSERT INTO sales (timestamp, total, payment_method) "
            "VALUES (?, ?, ?);";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db_));

        sqlite3_bind_text  (stmt, 1, sale.timestamp.c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 2, sale.total);
        sqlite3_bind_text  (stmt, 3, sale.payment_method.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // 2. Grab the auto-assigned sale ID
        Sale saved    = sale;
        saved.id      = static_cast<int>(sqlite3_last_insert_rowid(db_));
        saved.items.clear();

        // 3. Insert each item, linking it to this sale
        for (const auto& item : sale.items) {
            SaleItem si   = item;
            si.sale_id    = saved.id;    // link to parent sale
            saved.items.push_back(save_item(si));
        }

        // 4. Everything worked — make it permanent
        sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
        return saved;

    } catch (...) {
        // Something failed — undo everything, leave DB clean
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;   // re-throw so the caller knows it failed
    }
}

// ── SELECT one sale + fetch its items ────────────────────────────────────
std::optional<Sale> SaleRepository::find_by_id(int id) {
    const char* sql =
        "SELECT id, timestamp, total, payment_method "
        "FROM sales WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    sqlite3_bind_int(stmt, 1, id);

    std::optional<Sale> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = row_to_sale(stmt);

    sqlite3_finalize(stmt);

    if (!result) return std::nullopt;  // sale not found, stop here

    // Now fetch the items that belong to this sale
    const char* item_sql =
        "SELECT id, sale_id, product_id, quantity, unit_price "
        "FROM sale_items WHERE sale_id = ?;";

    sqlite3_stmt* istmt;
    if (sqlite3_prepare_v2(db_, item_sql, -1, &istmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    sqlite3_bind_int(istmt, 1, id);

    while (sqlite3_step(istmt) == SQLITE_ROW) {
        SaleItem si;
        si.id         = sqlite3_column_int   (istmt, 0);
        si.sale_id    = sqlite3_column_int   (istmt, 1);
        si.product_id = sqlite3_column_int   (istmt, 2);
        si.quantity   = sqlite3_column_int   (istmt, 3);
        si.unit_price = sqlite3_column_double(istmt, 4);
        result->items.push_back(si);
    }

    sqlite3_finalize(istmt);
    return result;
}

// ── SELECT all sales (headers only, no items) ────────────────────────────
std::vector<Sale> SaleRepository::find_all() {
    const char* sql =
        "SELECT id, timestamp, total, payment_method FROM sales;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    std::vector<Sale> sales;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        sales.push_back(row_to_sale(stmt));

    sqlite3_finalize(stmt);
    return sales;
}