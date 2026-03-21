#pragma once
#include <sqlite3.h>
#include <vector>
#include <optional>
#include "models/product.h"

class ProductRepository {
public:
    explicit ProductRepository(sqlite3* db);

    Product                  save(const Product& p);       // INSERT
    std::optional<Product>   find_by_id(int id);           // SELECT one
    std::vector<Product>     find_all();                   // SELECT all
    bool                     update(const Product& p);     // UPDATE
    bool                     remove(int id);               // DELETE
private:
    sqlite3* db_;
};