#pragma once
#include <string>
#include <vector>
#include "sale_item.h"

struct Sale {
    int         id             = 0;
    std::string timestamp;           // stored as "YYYY-MM-DD HH:MM:SS"
    double      total          = 0.0;
    std::string payment_method;      // "cash" or "card"

    std::vector<SaleItem> items;     // the line items that belong to this sale
};