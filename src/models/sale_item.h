#pragma once

struct SaleItem {
    int    id          = 0;
    int    sale_id     = 0;
    int    product_id  = 0;
    int    quantity    = 0;
    double unit_price  = 0.0;   // snapshot of price at time of sale
};