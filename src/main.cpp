#include <iostream>
#include "db/database.h"
#include "repositories/product_repository.h"
#include "repositories/sale_repository.h"
#include "services/product_service.h"
#include "services/sale_service.h"

int main() {
    Database db("pos.db");
    db.initialize_schema();

    ProductRepository product_repo(db.handle());
    SaleRepository    sale_repo(db.handle());
    ProductService    product_svc(product_repo);
    SaleService       sale_svc(sale_repo, product_svc);

    // Add a product
    Product p = product_svc.add_product("Coffee", 3.50, 10, "Drinks");
    std::cout << "Added: " << p.name << " (stock: " << p.stock << ")\n";

    // Process a sale
    SaleItem item;
    item.product_id = p.id;
    item.quantity   = 2;

    Sale sale = sale_svc.process_sale({item}, "cash");
    std::cout << "Sale total: $" << sale.total << "\n";

    // Check stock was deducted
    Product updated = product_svc.get_product(p.id);
    std::cout << "Remaining stock: " << updated.stock << "\n";

    // Try to break a rule — sell more than we have
    try {
        SaleItem big_order;
        big_order.product_id = p.id;
        big_order.quantity   = 999;
        sale_svc.process_sale({big_order}, "cash");
    } catch (const std::exception& e) {
        std::cout << "Caught expected error: " << e.what() << "\n";
    }

    return 0;
}
```

Build and run — you should see:
```
Added: Coffee (stock: 10)
Sale total: $7
Remaining stock: 8
Caught expected error: Insufficient stock for: Coffee