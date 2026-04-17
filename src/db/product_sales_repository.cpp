#include "product_sales_repository.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace product_db {

bool isNumber(const std::string& input) {
  if (input.empty()) {
    return false;
  }

  for (unsigned char c : input) {
    if (!std::isdigit(c)) {
      return false;
    }
  }

  return true;
}

std::string toLower(std::string value) {
  for (char& c : value) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return value;
}

bool openProductDatabase(sqlite3** db) {
  if (sqlite3_open("db/product.db", db) != SQLITE_OK) {
    std::cerr << "Cannot open db/product.db: " << sqlite3_errmsg(*db)
              << std::endl;
    return false;
  }

  const char* createTableSql =
      "CREATE TABLE IF NOT EXISTS products ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "name TEXT NOT NULL UNIQUE,"
      "quantity INTEGER NOT NULL DEFAULT 0,"
      "price REAL NOT NULL DEFAULT 0,"
      "sell_price REAL NOT NULL DEFAULT 0"
      ");";

  char* errorMessage = nullptr;
  if (sqlite3_exec(*db, createTableSql, nullptr, nullptr, &errorMessage) !=
      SQLITE_OK) {
    std::cerr << "Failed to create products table: " << errorMessage
              << std::endl;
    sqlite3_free(errorMessage);
    sqlite3_close(*db);
    *db = nullptr;
    return false;
  }

  const char* addSellPriceColumnSql =
      "ALTER TABLE products ADD COLUMN sell_price REAL NOT NULL DEFAULT 0;";
  if (sqlite3_exec(*db, addSellPriceColumnSql, nullptr, nullptr, nullptr) !=
      SQLITE_OK) {
    // Ignore errors because column may already exist.
  }

  return true;
}

bool openSellDatabase(sqlite3** sellDb) {
  if (sqlite3_open("db/sell.db", sellDb) != SQLITE_OK) {
    std::cerr << "Cannot open db/sell.db: " << sqlite3_errmsg(*sellDb)
              << std::endl;
    return false;
  }

  const char* createSalesTableSql =
      "CREATE TABLE IF NOT EXISTS sales ("
      "time TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),"
      "quantity INTEGER NOT NULL,"
      "id INTEGER NOT NULL,"
      "product_name TEXT NOT NULL DEFAULT '',"
      "total_sell REAL NOT NULL"
      ");";

  char* errorMessage = nullptr;
  if (sqlite3_exec(*sellDb, createSalesTableSql, nullptr, nullptr,
                   &errorMessage) != SQLITE_OK) {
    std::cerr << "Failed to create sales table: " << errorMessage << std::endl;
    sqlite3_free(errorMessage);
    sqlite3_close(*sellDb);
    *sellDb = nullptr;
    return false;
  }

  const char* addProductNameColumnSql =
      "ALTER TABLE sales ADD COLUMN product_name TEXT NOT NULL DEFAULT '';";
  if (sqlite3_exec(*sellDb, addProductNameColumnSql, nullptr, nullptr,
                   nullptr) != SQLITE_OK) {
    // Ignore errors because the column may already exist.
  }

  return true;
}

void printProducts(sqlite3* db) {
  const char* query =
      "SELECT id, name, quantity, price, sell_price FROM products ORDER BY id;";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "Failed to load products." << std::endl;
    return;
  }

  bool hasRows = false;
  constexpr int kIdWidth = 4;
  constexpr int kNameWidth = 18;
  constexpr int kQtyWidth = 10;
  constexpr int kPriceWidth = 10;
  constexpr int kSellPriceWidth = 12;

  std::cout << "\n"
            << std::left << std::setw(kIdWidth) << "ID"
            << " | " << std::setw(kNameWidth) << "Name"
            << " | " << std::right << std::setw(kQtyWidth) << "Quantity"
            << " | " << std::setw(kPriceWidth) << "Price"
            << " | " << std::setw(kSellPriceWidth) << "Sell Price" << std::endl;
  std::cout << std::string(kIdWidth + kNameWidth + kQtyWidth + kPriceWidth +
                               kSellPriceWidth + 12,
                           '-')
            << std::endl;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    hasRows = true;
    int id = sqlite3_column_int(stmt, 0);
    const unsigned char* name = sqlite3_column_text(stmt, 1);
    int quantity = sqlite3_column_int(stmt, 2);
    double price = sqlite3_column_double(stmt, 3);
    double sellPrice = sqlite3_column_double(stmt, 4);

    std::string productName = name ? reinterpret_cast<const char*>(name) : "";
    if (productName.length() > static_cast<std::size_t>(kNameWidth)) {
      productName = productName.substr(0, kNameWidth - 3) + "...";
    }

    std::cout << std::left << std::setw(kIdWidth) << id << " | "
              << std::setw(kNameWidth) << productName << " | " << std::right
              << std::setw(kQtyWidth) << quantity << " | "
              << std::setw(kPriceWidth) << std::fixed << std::setprecision(2)
              << price << " | " << std::setw(kSellPriceWidth) << sellPrice
              << std::defaultfloat << std::endl;
  }

  if (!hasRows) {
    std::cout << "No products found yet." << std::endl;
  }

  sqlite3_finalize(stmt);
}

double getAllTimeTotalMoney(sqlite3* sellDb) {
  sqlite3_stmt* stmt = nullptr;
  const char* totalSql = "SELECT COALESCE(SUM(total_sell), 0) FROM sales;";

  if (sqlite3_prepare_v2(sellDb, totalSql, -1, &stmt, nullptr) != SQLITE_OK) {
    return 0;
  }

  double total = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    total = sqlite3_column_double(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return total;
}

void showSalesRows(sqlite3* sellDb, const std::string& whereClause,
                   const std::string& title) {
  std::string query =
      "SELECT s.time, COALESCE(NULLIF(s.product_name, ''), '[deleted]'), "
      "s.quantity, s.total_sell "
      "FROM sales s " +
      whereClause + " ORDER BY datetime(s.time) DESC;";
  std::string totalQuery =
      "SELECT COALESCE(SUM(s.total_sell), 0) FROM sales s " + whereClause + ";";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(sellDb, query.c_str(), -1, &stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "Failed to fetch sales history." << std::endl;
    return;
  }

  constexpr int kTimeWidth = 19;
  constexpr int kProductWidth = 20;
  constexpr int kQtyWidth = 10;
  constexpr int kTotalSellWidth = 12;

  std::cout << "\n=== " << title << " ===" << std::endl;
  std::cout << std::left << std::setw(kTimeWidth) << "Time"
            << " | " << std::setw(kProductWidth) << "Product Name"
            << " | " << std::right << std::setw(kQtyWidth) << "Quantity"
            << " | " << std::setw(kTotalSellWidth) << "Total Sell" << std::endl;
  std::cout << std::string(
                   kTimeWidth + kProductWidth + kQtyWidth + kTotalSellWidth + 9,
                   '-')
            << std::endl;

  bool hasRows = false;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    hasRows = true;
    const unsigned char* timeText = sqlite3_column_text(stmt, 0);
    const unsigned char* productName = sqlite3_column_text(stmt, 1);
    int quantity = sqlite3_column_int(stmt, 2);
    double totalSell = sqlite3_column_double(stmt, 3);

    std::string timeValue =
        timeText ? reinterpret_cast<const char*>(timeText) : "";
    std::string productValue =
        productName ? reinterpret_cast<const char*>(productName) : "";
    if (productValue.length() > static_cast<std::size_t>(kProductWidth)) {
      productValue = productValue.substr(0, kProductWidth - 3) + "...";
    }

    std::cout << std::left << std::setw(kTimeWidth) << timeValue << " | "
              << std::setw(kProductWidth) << productValue << " | " << std::right
              << std::setw(kQtyWidth) << quantity << " | "
              << std::setw(kTotalSellWidth) << std::fixed
              << std::setprecision(2) << totalSell << std::defaultfloat
              << std::endl;
  }

  if (!hasRows) {
    std::cout << "No sales found for this period." << std::endl;
  }
  sqlite3_finalize(stmt);

  sqlite3_stmt* totalStmt = nullptr;
  if (sqlite3_prepare_v2(sellDb, totalQuery.c_str(), -1, &totalStmt, nullptr) ==
      SQLITE_OK) {
    if (sqlite3_step(totalStmt) == SQLITE_ROW) {
      std::cout << "Period total: " << sqlite3_column_double(totalStmt, 0)
                << std::endl;
    }
    sqlite3_finalize(totalStmt);
  }

  std::cout << "All-time total: " << getAllTimeTotalMoney(sellDb) << std::endl;
}

bool findProductBasic(sqlite3* db, const std::string& productInput,
                      int* productId, std::string* productName, int* quantity) {
  const char* findByIdSql =
      "SELECT id, name, quantity FROM products WHERE id = ?;";
  const char* findByNameSql =
      "SELECT id, name, quantity FROM products WHERE LOWER(name) = LOWER(?);";

  sqlite3_stmt* stmt = nullptr;
  const char* query = isNumber(productInput) ? findByIdSql : findByNameSql;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  if (isNumber(productInput)) {
    sqlite3_bind_int(stmt, 1, std::atoi(productInput.c_str()));
  } else {
    sqlite3_bind_text(stmt, 1, productInput.c_str(), -1, SQLITE_TRANSIENT);
  }

  bool found = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    *productId = sqlite3_column_int(stmt, 0);
    const unsigned char* productNameRaw = sqlite3_column_text(stmt, 1);
    *productName =
        productNameRaw ? reinterpret_cast<const char*>(productNameRaw) : "";
    *quantity = sqlite3_column_int(stmt, 2);
    found = true;
  }

  sqlite3_finalize(stmt);
  return found;
}

bool findProductForCustomer(sqlite3* productDb, const std::string& productInput,
                            int* productId, std::string* productName,
                            int* availableQty, double* sellPrice) {
  const char* findByIdSql =
      "SELECT id, name, quantity, sell_price FROM products WHERE id = ?;";
  const char* findByNameSql =
      "SELECT id, name, quantity, sell_price "
      "FROM products WHERE LOWER(name) = LOWER(?);";

  sqlite3_stmt* stmt = nullptr;
  const char* query = isNumber(productInput) ? findByIdSql : findByNameSql;
  if (sqlite3_prepare_v2(productDb, query, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  if (isNumber(productInput)) {
    sqlite3_bind_int(stmt, 1, std::atoi(productInput.c_str()));
  } else {
    sqlite3_bind_text(stmt, 1, productInput.c_str(), -1, SQLITE_TRANSIENT);
  }

  bool found = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    *productId = sqlite3_column_int(stmt, 0);
    const unsigned char* productNameRaw = sqlite3_column_text(stmt, 1);
    *productName =
        productNameRaw ? reinterpret_cast<const char*>(productNameRaw) : "";
    *availableQty = sqlite3_column_int(stmt, 2);
    *sellPrice = sqlite3_column_double(stmt, 3);
    found = true;
  }

  sqlite3_finalize(stmt);
  return found;
}

bool insertProduct(sqlite3* db, const std::string& productName, int quantity,
                   double buyPrice, double sellPrice) {
  sqlite3_stmt* stmt = nullptr;
  const char* insertSql =
      "INSERT INTO products (name, quantity, price, sell_price) VALUES (?, ?, "
      "?, ?);";

  if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_text(stmt, 1, productName.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, quantity);
  sqlite3_bind_double(stmt, 3, buyPrice);
  sqlite3_bind_double(stmt, 4, sellPrice);

  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  sqlite3_finalize(stmt);
  return ok;
}

bool updateProductInventory(sqlite3* db, int productId, int addQty,
                            double buyPrice, double sellPrice) {
  sqlite3_stmt* stmt = nullptr;
  const char* updateSql =
      "UPDATE products SET quantity = quantity + ?, price = ?, sell_price = ? "
      "WHERE id = ?;";

  if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, addQty);
  sqlite3_bind_double(stmt, 2, buyPrice);
  sqlite3_bind_double(stmt, 3, sellPrice);
  sqlite3_bind_int(stmt, 4, productId);

  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  sqlite3_finalize(stmt);
  return ok;
}

bool hasEnoughStock(sqlite3* productDb, int productId, int requiredQty) {
  sqlite3_stmt* stmt = nullptr;
  const char* stockSql = "SELECT quantity FROM products WHERE id = ?;";

  if (sqlite3_prepare_v2(productDb, stockSql, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, productId);
  bool ok = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int currentQty = sqlite3_column_int(stmt, 0);
    ok = currentQty >= requiredQty;
  }

  sqlite3_finalize(stmt);
  return ok;
}

bool finalizeSale(sqlite3* productDb, sqlite3* sellDb,
                  const std::vector<CartItem>& cart) {
  for (const CartItem& item : cart) {
    if (!hasEnoughStock(productDb, item.productId, item.quantity)) {
      std::cout << "Stock changed. Not enough quantity for " << item.productName
                << "." << std::endl;
      return false;
    }
  }

  for (const CartItem& item : cart) {
    sqlite3_stmt* updateStmt = nullptr;
    const char* updateSql =
        "UPDATE products SET quantity = quantity - ? WHERE id = ?;";

    if (sqlite3_prepare_v2(productDb, updateSql, -1, &updateStmt, nullptr) !=
        SQLITE_OK) {
      return false;
    }

    sqlite3_bind_int(updateStmt, 1, item.quantity);
    sqlite3_bind_int(updateStmt, 2, item.productId);

    if (sqlite3_step(updateStmt) != SQLITE_DONE) {
      sqlite3_finalize(updateStmt);
      return false;
    }
    sqlite3_finalize(updateStmt);

    sqlite3_stmt* insertSaleStmt = nullptr;
    const char* insertSaleSql =
        "INSERT INTO sales (quantity, id, product_name, total_sell) "
        "VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(sellDb, insertSaleSql, -1, &insertSaleStmt,
                           nullptr) != SQLITE_OK) {
      return false;
    }

    sqlite3_bind_int(insertSaleStmt, 1, item.quantity);
    sqlite3_bind_int(insertSaleStmt, 2, item.productId);
    sqlite3_bind_text(insertSaleStmt, 3, item.productName.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_double(insertSaleStmt, 4, item.lineTotal);

    if (sqlite3_step(insertSaleStmt) != SQLITE_DONE) {
      sqlite3_finalize(insertSaleStmt);
      return false;
    }
    sqlite3_finalize(insertSaleStmt);
  }

  return true;
}

}  // namespace product_db
