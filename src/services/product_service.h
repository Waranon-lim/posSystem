#pragma once
#include <vector>
#include <string>
#include "models/product.h"
#include "repositories/product_repository.h"

class ProductService {
public:
    explicit ProductService(ProductRepository& repo);

    Product              add_product(const std::string& name,
                                     double price,
                                     int stock,
                                     const std::string& category);

    std::vector<Product> list_products();
    Product              get_product(int id);            // throws if not found
    bool                 update_stock(int id, int qty);  // add or remove stock
    bool                 remove_product(int id);

private:
    ProductRepository& repo_;   // reference, not pointer — we don't own it
};