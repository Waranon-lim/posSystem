#pragma once
#include <sqlite3.h>
#include <vector>
#include <optional>
#include "models/sale.h"

class SaleRepository {
public:
    explicit SaleRepository(sqlite3* db);

    Sale                   save(const Sale& sale);      // INSERT sale + items
    std::optional<Sale>    find_by_id(int id);          // SELECT sale + its items
    std::vector<Sale>      find_all();                  // SELECT all sales (no items)

private:
    sqlite3* db_;

    // Private helper — inserts one SaleItem row, returns it with its new id
    SaleItem save_item(const SaleItem& item);
};