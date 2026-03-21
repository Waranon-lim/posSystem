#include "sale_service.h"
#include <stdexcept>
#include <ctime>
#include <sstream>
#include <iomanip>

SaleService::SaleService(SaleRepository&  sale_repo,
                         ProductService&  product_service)
    : sale_repo_(sale_repo)
    , product_service_(product_service) {}

// ── Helper: get current time as a formatted string ───────────────────────
static std::string current_timestamp() {
    std::time_t now = std::time(nullptr);
    std::tm*    tm  = std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ── Helper: sum up the total from all items ──────────────────────────────
double SaleService::calculate_total(const std::vector<SaleItem>& items) {
    double total = 0.0;
    for (const auto& item : items)
        total += item.unit_price * item.quantity;
    return total;
}

// ── Process a sale — the core business logic ─────────────────────────────
Sale SaleService::process_sale(const std::vector<SaleItem>& items,
                                const std::string& payment_method) {
    // Rule 1: cart cannot be empty
    if (items.empty())
        throw std::invalid_argument("Cannot process an empty sale");

    // Rule 2: payment method must be valid
    if (payment_method != "cash" && payment_method != "card")
        throw std::invalid_argument("Payment method must be 'cash' or 'card'");

    // Rule 3: validate every item and check stock
    std::vector<SaleItem> validated_items;
    for (const auto& item : items) {
        if (item.quantity <= 0)
            throw std::invalid_argument("Quantity must be greater than zero");

        // This throws if product doesn't exist
        Product p = product_service_.get_product(item.product_id);

        if (p.stock < item.quantity)
            throw std::runtime_error("Insufficient stock for: " + p.name);

        // Snapshot the price at time of sale (important!)
        SaleItem validated  = item;
        validated.unit_price = p.price;
        validated_items.push_back(validated);
    }

    // Rule 4: deduct stock for each item
    for (const auto& item : validated_items)
        product_service_.update_stock(item.product_id, -item.quantity);

    // Build and save the sale
    Sale sale;
    sale.timestamp      = current_timestamp();
    sale.payment_method = payment_method;
    sale.total          = calculate_total(validated_items);
    sale.items          = validated_items;

    return sale_repo_.save(sale);
}

// ── List all sales ────────────────────────────────────────────────────────
std::vector<Sale> SaleService::list_sales() {
    return sale_repo_.find_all();
}

// ── Get one sale — throws if not found ───────────────────────────────────
Sale SaleService::get_sale(int id) {
    auto sale = sale_repo_.find_by_id(id);
    if (!sale)
        throw std::runtime_error("Sale with ID " + std::to_string(id) +
                                 " not found");
    return *sale;
}