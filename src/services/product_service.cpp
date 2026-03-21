#include "product_service.h"
#include <stdexcept>

ProductService::ProductService(ProductRepository& repo) : repo_(repo) {}

// ── Add a new product (validates inputs first) ───────────────────────────
Product ProductService::add_product(const std::string& name,
                                     double price,
                                     int stock,
                                     const std::string& category) {
    // Business rules — repository never checks these
    if (name.empty())
        throw std::invalid_argument("Product name cannot be empty");
    if (price <= 0)
        throw std::invalid_argument("Price must be greater than zero");
    if (stock < 0)
        throw std::invalid_argument("Stock cannot be negative");

    Product p;
    p.name     = name;
    p.price    = price;
    p.stock    = stock;
    p.category = category;
    return repo_.save(p);
}

// ── List all products ────────────────────────────────────────────────────
std::vector<Product> ProductService::list_products() {
    return repo_.find_all();
}

// ── Get one product — throws a clear error if missing ───────────────────
Product ProductService::get_product(int id) {
    auto product = repo_.find_by_id(id);
    if (!product)
        throw std::runtime_error("Product with ID " + std::to_string(id) +
                                 " not found");
    return *product;
}

// ── Update stock (qty can be negative to reduce stock) ──────────────────
bool ProductService::update_stock(int id, int qty) {
    auto product = repo_.find_by_id(id);
    if (!product)
        throw std::runtime_error("Product not found");

    int new_stock = product->stock + qty;
    if (new_stock < 0)
        throw std::runtime_error("Insufficient stock");

    product->stock = new_stock;
    return repo_.update(*product);
}

// ── Remove a product ─────────────────────────────────────────────────────
bool ProductService::remove_product(int id) {
    // Make sure it exists before trying to delete
    get_product(id);
    return repo_.remove(id);
}