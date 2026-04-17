#include <sqlite3.h>

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Dashboard_ui.h"

namespace {
void clearScreen() { std::cout << "\033[2J\033[1;1H"; }

struct CartItem {
  int productId;
  std::string productName;
  int quantity;
  double sellPrice;
  double lineTotal;
};

constexpr double kOpeningFloatBaht = 1000.0;

double getAllTimeTotalMoney(sqlite3* sellDb);

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

std::string formatBaht(double amount) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << amount;
  std::string value = oss.str();

  std::size_t dotPos = value.find('.');
  if (dotPos == std::string::npos) {
    dotPos = value.length();
  }

  std::size_t start = (value[0] == '-') ? 1 : 0;
  for (int i = static_cast<int>(dotPos) - 3; i > static_cast<int>(start);
       i -= 3) {
    value.insert(static_cast<std::size_t>(i), ",");
  }

  return "B" + value;
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
    // Ignore errors here because the column may already exist.
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

void handleOwnerSellFlow() {
  sqlite3* db = nullptr;
  if (!openProductDatabase(&db)) {
    return;
  }

  while (true) {
    clearScreen();
    printProducts(db);

    std::string productInput;
    std::cout << "\nEnter product ID or name (or 'back' to menu): ";
    std::cin >> productInput;

    std::string normalizedInput = toLower(productInput);
    if (normalizedInput == "back" || normalizedInput == "quit") {
      sqlite3_close(db);
      return;
    }

    const char* findByIdSql =
        "SELECT id, name, quantity FROM products WHERE id = ?;";
    const char* findByNameSql =
        "SELECT id, name, quantity FROM products WHERE LOWER(name) = LOWER(?);";

    sqlite3_stmt* findStmt = nullptr;
    const char* selectSql =
        isNumber(productInput) ? findByIdSql : findByNameSql;

    if (sqlite3_prepare_v2(db, selectSql, -1, &findStmt, nullptr) !=
        SQLITE_OK) {
      std::cerr << "Failed to search product." << std::endl;
      continue;
    }

    if (isNumber(productInput)) {
      sqlite3_bind_int(findStmt, 1, std::atoi(productInput.c_str()));
    } else {
      sqlite3_bind_text(findStmt, 1, productInput.c_str(), -1,
                        SQLITE_TRANSIENT);
    }

    int rc = sqlite3_step(findStmt);
    bool exists = (rc == SQLITE_ROW);

    int productId = 0;
    int currentQty = 0;
    std::string productName;
    if (exists) {
      productId = sqlite3_column_int(findStmt, 0);
      const unsigned char* productNameRaw = sqlite3_column_text(findStmt, 1);
      currentQty = sqlite3_column_int(findStmt, 2);
      productName =
          productNameRaw ? reinterpret_cast<const char*>(productNameRaw) : "";
    }
    sqlite3_finalize(findStmt);

    if (!exists) {
      std::cout << "Product does not exist." << std::endl;
      std::string addChoice;
      std::cout << "Do you want to add this product? (y/n): ";
      std::cin >> addChoice;
      std::string lowerAddChoice = toLower(addChoice);

      if (lowerAddChoice == "back" || lowerAddChoice == "quit") {
        continue;
      }

      if (lowerAddChoice != "y" && lowerAddChoice != "yes") {
        continue;
      }

      if (isNumber(productInput)) {
        std::cout << "Enter new product name: ";
        std::cin >> productName;
        if (toLower(productName) == "back" || toLower(productName) == "quit") {
          continue;
        }
      } else {
        productName = productInput;
      }

      std::string qtyInput;
      std::cout << "How many do you want to add? (or 'back'): ";
      std::cin >> qtyInput;
      if (toLower(qtyInput) == "back" || toLower(qtyInput) == "quit") {
        continue;
      }

      int addQty = 0;
      try {
        addQty = std::stoi(qtyInput);
      } catch (...) {
        std::cout << "Invalid quantity." << std::endl;
        continue;
      }
      if (addQty <= 0) {
        std::cout << "Quantity must be greater than 0." << std::endl;
        continue;
      }

      std::string buyInput;
      std::cout << "How much is buy price? (or 'back'): ";
      std::cin >> buyInput;
      if (toLower(buyInput) == "back" || toLower(buyInput) == "quit") {
        continue;
      }

      double buyPrice = 0;
      try {
        buyPrice = std::stod(buyInput);
      } catch (...) {
        std::cout << "Invalid buy price." << std::endl;
        continue;
      }
      if (buyPrice <= 0) {
        std::cout << "Buy price must be greater than 0." << std::endl;
        continue;
      }

      std::string sellInput;
      std::cout << "How much are you going to sell it? (or 'back'): ";
      std::cin >> sellInput;
      if (toLower(sellInput) == "back" || toLower(sellInput) == "quit") {
        continue;
      }

      double sellPrice = 0;
      try {
        sellPrice = std::stod(sellInput);
      } catch (...) {
        std::cout << "Invalid sell price." << std::endl;
        continue;
      }
      if (sellPrice <= 0) {
        std::cout << "Sell price must be greater than 0." << std::endl;
        continue;
      }

      sqlite3_stmt* insertStmt = nullptr;
      const char* insertSql =
          "INSERT INTO products (name, quantity, price, sell_price) VALUES (?, "
          "?, ?, ?);";
      if (sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr) !=
          SQLITE_OK) {
        std::cerr << "Failed to insert product." << std::endl;
        continue;
      }

      sqlite3_bind_text(insertStmt, 1, productName.c_str(), -1,
                        SQLITE_TRANSIENT);
      sqlite3_bind_int(insertStmt, 2, addQty);
      sqlite3_bind_double(insertStmt, 3, buyPrice);
      sqlite3_bind_double(insertStmt, 4, sellPrice);

      if (sqlite3_step(insertStmt) != SQLITE_DONE) {
        std::cout << "Could not add product (name may already exist)."
                  << std::endl;
        sqlite3_finalize(insertStmt);
        continue;
      }

      sqlite3_finalize(insertStmt);
      std::cout << "Product added successfully." << std::endl;
      continue;
    }

    std::cout << "Product exists: " << productName
              << " | quantity left: " << currentQty << std::endl;

    std::string qtyInput;
    std::cout << "How many do you want to add? (or 'back'): ";
    std::cin >> qtyInput;
    if (toLower(qtyInput) == "back" || toLower(qtyInput) == "quit") {
      continue;
    }

    int addQty = 0;
    try {
      addQty = std::stoi(qtyInput);
    } catch (...) {
      std::cout << "Invalid quantity." << std::endl;
      continue;
    }
    if (addQty <= 0) {
      std::cout << "Quantity must be greater than 0." << std::endl;
      continue;
    }

    std::string buyInput;
    std::cout << "How much is buy price? (or 'back'): ";
    std::cin >> buyInput;
    if (toLower(buyInput) == "back" || toLower(buyInput) == "quit") {
      continue;
    }

    double buyPrice = 0;
    try {
      buyPrice = std::stod(buyInput);
    } catch (...) {
      std::cout << "Invalid buy price." << std::endl;
      continue;
    }
    if (buyPrice <= 0) {
      std::cout << "Buy price must be greater than 0." << std::endl;
      continue;
    }

    std::string sellInput;
    std::cout << "How much are you going to sell it? (or 'back'): ";
    std::cin >> sellInput;
    if (toLower(sellInput) == "back" || toLower(sellInput) == "quit") {
      continue;
    }

    double sellPrice = 0;
    try {
      sellPrice = std::stod(sellInput);
    } catch (...) {
      std::cout << "Invalid sell price." << std::endl;
      continue;
    }
    if (sellPrice <= 0) {
      std::cout << "Sell price must be greater than 0." << std::endl;
      continue;
    }

    sqlite3_stmt* updateStmt = nullptr;
    const char* updateSql =
        "UPDATE products SET quantity = quantity + ?, price = ?, sell_price = "
        "? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, updateSql, -1, &updateStmt, nullptr) !=
        SQLITE_OK) {
      std::cerr << "Failed to update product." << std::endl;
      continue;
    }

    sqlite3_bind_int(updateStmt, 1, addQty);
    sqlite3_bind_double(updateStmt, 2, buyPrice);
    sqlite3_bind_double(updateStmt, 3, sellPrice);
    sqlite3_bind_int(updateStmt, 4, productId);

    if (sqlite3_step(updateStmt) != SQLITE_DONE) {
      std::cerr << "Could not update inventory." << std::endl;
      sqlite3_finalize(updateStmt);
      continue;
    }

    sqlite3_finalize(updateStmt);
    std::cout << "Inventory updated for " << productName << "." << std::endl;
  }
}

void printDashboardHeader(const std::string& ownerName) {
  std::cout << "\n=== " << ownerName << " Dashboard ===" << std::endl;
  std::cout << "1. View daily report" << std::endl;
  std::cout << "2. Manage inventory" << std::endl;
  std::cout << "3. Logout" << std::endl;
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

  const char* createTransactionsTableSql =
      "CREATE TABLE IF NOT EXISTS transactions ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "time TEXT NOT NULL DEFAULT (datetime('now', 'localtime')) ,"
      "total_sell REAL NOT NULL,"
      "paid_amount REAL NOT NULL,"
      "change_amount REAL NOT NULL"
      ");";

  if (sqlite3_exec(*sellDb, createTransactionsTableSql, nullptr, nullptr,
                   &errorMessage) != SQLITE_OK) {
    std::cerr << "Failed to create transactions table: " << errorMessage
              << std::endl;
    sqlite3_free(errorMessage);
    sqlite3_close(*sellDb);
    *sellDb = nullptr;
    return false;
  }

  return true;
}

bool saveTransaction(sqlite3* sellDb, double totalSell, double paidAmount,
                     double changeAmount) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "INSERT INTO transactions (total_sell, paid_amount, change_amount) "
      "VALUES (?, ?, ?);";

  if (sqlite3_prepare_v2(sellDb, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_double(stmt, 1, totalSell);
  sqlite3_bind_double(stmt, 2, paidAmount);
  sqlite3_bind_double(stmt, 3, changeAmount);

  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  sqlite3_finalize(stmt);
  return ok;
}

void showDailySalesReport(sqlite3* sellDb) {
  sqlite3_stmt* summaryStmt = nullptr;
  const char* summarySql =
      "SELECT COALESCE(COUNT(*), 0), "
      "COALESCE(SUM(total_sell), 0), "
      "COALESCE(SUM(paid_amount), 0), "
      "COALESCE(SUM(change_amount), 0) "
      "FROM transactions "
      "WHERE date(time) = date('now', 'localtime');";

  int totalTransactions = 0;
  double totalSales = 0;
  double totalReceived = 0;
  double totalChange = 0;

  if (sqlite3_prepare_v2(sellDb, summarySql, -1, &summaryStmt, nullptr) ==
      SQLITE_OK) {
    if (sqlite3_step(summaryStmt) == SQLITE_ROW) {
      totalTransactions = sqlite3_column_int(summaryStmt, 0);
      totalSales = sqlite3_column_double(summaryStmt, 1);
      totalReceived = sqlite3_column_double(summaryStmt, 2);
      totalChange = sqlite3_column_double(summaryStmt, 3);
    }
  }
  sqlite3_finalize(summaryStmt);

  double gainedToday = totalSales;

  std::cout << "\n+-------------------------------------------+" << std::endl;
  std::cout << "|                DAILY REPORT              |" << std::endl;
  std::cout << "+-------------------------------------------+" << std::endl;
  std::cout << "| Opening Float       : " << formatBaht(kOpeningFloatBaht)
            << std::endl;
  std::cout << "| Total Transactions  : " << totalTransactions << std::endl;
  std::cout << "| Total Sales         : " << formatBaht(totalSales)
            << std::endl;
  std::cout << "| Total Received      : " << formatBaht(totalReceived)
            << std::endl;
  std::cout << "| Total Change Given  : " << formatBaht(totalChange)
            << std::endl;
  std::cout << "| Money Gained Today  : " << formatBaht(gainedToday)
            << std::endl;
  std::cout << "+-------------------------------------------+" << std::endl;
  std::cout << "|            TRANSACTION DETAIL             |" << std::endl;
  std::cout << "+-------------------------------------------+" << std::endl;

  sqlite3_stmt* detailStmt = nullptr;
  const char* detailSql =
      "SELECT id, total_sell, paid_amount, change_amount "
      "FROM transactions "
      "WHERE date(time) = date('now', 'localtime') "
      "ORDER BY datetime(time) DESC;";

  if (sqlite3_prepare_v2(sellDb, detailSql, -1, &detailStmt, nullptr) ==
      SQLITE_OK) {
    bool hasRows = false;
    while (sqlite3_step(detailStmt) == SQLITE_ROW) {
      hasRows = true;
      int txId = sqlite3_column_int(detailStmt, 0);
      double txTotal = sqlite3_column_double(detailStmt, 1);
      double txPaid = sqlite3_column_double(detailStmt, 2);
      double txChange = sqlite3_column_double(detailStmt, 3);

      std::cout << "| #" << txId << "  " << formatBaht(txTotal) << "  paid "
                << formatBaht(txPaid) << "  chg " << formatBaht(txChange)
                << std::endl;
    }

    if (!hasRows) {
      std::cout << "| No transactions today." << std::endl;
    }
  }
  sqlite3_finalize(detailStmt);
  std::cout << "+-------------------------------------------+" << std::endl;
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

void waitForEnter() {
  std::cout << "\nPress Enter to continue...";
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::cin.get();
}

void handleOwnerSalesHistory() {
  sqlite3* sellDb = nullptr;
  if (!openSellDatabase(&sellDb)) {
    return;
  }

  while (true) {
    clearScreen();
    int historyChoice = 0;
    std::cout << "\n=== Sales History ===" << std::endl;
    std::cout << "1. Today only" << std::endl;
    std::cout << "2. Last 1 day" << std::endl;
    std::cout << "3. Last 3 days" << std::endl;
    std::cout << "4. Last 7 days" << std::endl;
    std::cout << "5. Last 30 days" << std::endl;
    std::cout << "6. All sales" << std::endl;
    std::cout << "7. Back" << std::endl;
    std::cout << "Select option: ";

    if (!(std::cin >> historyChoice)) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "Please enter a number." << std::endl;
      continue;
    }

    switch (historyChoice) {
      case 1:
        showDailySalesReport(sellDb);
        waitForEnter();
        break;
      case 2:
        showSalesRows(
            sellDb,
            "WHERE datetime(s.time) >= datetime('now', 'localtime', '-1 day')",
            "Sales in Last 1 Day");
        waitForEnter();
        break;
      case 3:
        showSalesRows(
            sellDb,
            "WHERE datetime(s.time) >= datetime('now', 'localtime', '-3 day')",
            "Sales in Last 3 Days");
        waitForEnter();
        break;
      case 4:
        showSalesRows(
            sellDb,
            "WHERE datetime(s.time) >= datetime('now', 'localtime', '-7 day')",
            "Sales in Last 7 Days");
        waitForEnter();
        break;
      case 5:
        showSalesRows(
            sellDb,
            "WHERE datetime(s.time) >= datetime('now', 'localtime', '-30 day')",
            "Sales in Last 30 Days");
        waitForEnter();
        break;
      case 6:
        showSalesRows(sellDb, "", "All Sales");
        waitForEnter();
        break;
      case 7:
        sqlite3_close(sellDb);
        return;
      default:
        std::cout << "Invalid choice, try again." << std::endl;
        break;
    }
  }
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
    std::cerr << "Failed to search product." << std::endl;
    return false;
  }

  if (isNumber(productInput)) {
    sqlite3_bind_int(stmt, 1, std::atoi(productInput.c_str()));
  } else {
    sqlite3_bind_text(stmt, 1, productInput.c_str(), -1, SQLITE_TRANSIENT);
  }

  int rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    return false;
  }

  *productId = sqlite3_column_int(stmt, 0);
  const unsigned char* productNameRaw = sqlite3_column_text(stmt, 1);
  *productName =
      productNameRaw ? reinterpret_cast<const char*>(productNameRaw) : "";
  *availableQty = sqlite3_column_int(stmt, 2);
  *sellPrice = sqlite3_column_double(stmt, 3);
  sqlite3_finalize(stmt);
  return true;
}

bool hasEnoughStock(sqlite3* productDb, int productId, int requiredQty) {
  sqlite3_stmt* stmt = nullptr;
  const char* stockSql = "SELECT quantity FROM products WHERE id = ?;";

  if (sqlite3_prepare_v2(productDb, stockSql, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, productId);
  int rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    return false;
  }

  int currentQty = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return currentQty >= requiredQty;
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
}  // namespace

void displayMainMenu(const std::string& ownerName) {
  int choice = 0;

  while (true) {
    clearScreen();
    printDashboardHeader(ownerName);
    std::cout << "Select option: ";

    if (!(std::cin >> choice)) {
      std::cin.clear();
      std::cin.ignore(1000, '\n');
      std::cout << "Please enter a number." << std::endl;
      continue;
    }

    switch (choice) {
      case 1:
        handleOwnerSalesHistory();
        break;
      case 2:
        handleOwnerSellFlow();
        break;
      case 3:
        std::cout << "You are logging out." << std::endl;
        return;
      default:
        std::cout << "Invalid choice, try again." << std::endl;
        break;
    }
  }
}

void displayCustomerService(const std::string& customerName) {
  clearScreen();
  std::cout << "\n=== Welcome to Shop, " << customerName << " ===" << std::endl;

  sqlite3* productDb = nullptr;
  if (!openProductDatabase(&productDb)) {
    return;
  }

  sqlite3* sellDb = nullptr;
  if (!openSellDatabase(&sellDb)) {
    sqlite3_close(productDb);
    return;
  }

  while (true) {
    clearScreen();
    std::cout << "\n=== Welcome to Shop, " << customerName
              << " ===" << std::endl;
    std::vector<CartItem> cart;

    while (true) {
      printProducts(productDb);
      std::string productInput;
      std::cout << "\nEnter product ID or name (type 'done' to checkout, "
                   "'logout' to exit): ";
      std::cin >> productInput;

      std::string normalized = toLower(productInput);
      if (normalized == "logout") {
        sqlite3_close(sellDb);
        sqlite3_close(productDb);
        std::cout << "Thank you for shopping! Goodbye " << customerName
                  << std::endl;
        return;
      }

      if (normalized == "done") {
        if (cart.empty()) {
          std::cout << "Cart is empty. Add at least one product first."
                    << std::endl;
          continue;
        }
        break;
      }

      int productId = 0;
      std::string productName;
      int availableQty = 0;
      double sellPrice = 0;
      if (!findProductForCustomer(productDb, productInput, &productId,
                                  &productName, &availableQty, &sellPrice)) {
        std::cout << "Product not found." << std::endl;
        continue;
      }

      if (sellPrice <= 0) {
        std::cout
            << "This product has no sell price set yet. Please ask owner first."
            << std::endl;
        continue;
      }

      int qty = 0;
      std::cout << "Enter quantity: ";
      if (!(std::cin >> qty) || qty <= 0) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a valid quantity." << std::endl;
        continue;
      }

      if (qty > availableQty) {
        std::cout << "Not enough stock. Available quantity: " << availableQty
                  << std::endl;
        continue;
      }

      bool merged = false;
      for (CartItem& item : cart) {
        if (item.productId == productId) {
          if (item.quantity + qty > availableQty) {
            std::cout << "Not enough stock for combined quantity. Available: "
                      << availableQty << std::endl;
          } else {
            item.quantity += qty;
            item.lineTotal = item.quantity * item.sellPrice;
            std::cout << "Updated cart: " << item.productName << " x"
                      << item.quantity << std::endl;
          }
          merged = true;
          break;
        }
      }

      if (!merged) {
        CartItem item;
        item.productId = productId;
        item.productName = productName;
        item.quantity = qty;
        item.sellPrice = sellPrice;
        item.lineTotal = qty * sellPrice;
        cart.push_back(item);
        std::cout << "Added to cart: " << productName << " x" << qty
                  << " (price " << sellPrice << ")" << std::endl;
      }
    }

    double totalSell = 0;
    std::cout << "\n------ Checkout ------" << std::endl;
    for (const CartItem& item : cart) {
      std::cout << item.productName << " | qty: " << item.quantity
                << " | sell price: " << item.sellPrice
                << " | line total: " << item.lineTotal << std::endl;
      totalSell += item.lineTotal;
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total sell: " << totalSell << std::endl;

    double paidAmount = 0;
    std::cout << "Customer paid amount: ";
    if (!(std::cin >> paidAmount) || paidAmount < totalSell) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "Invalid amount or not enough money." << std::endl;
      continue;
    }

    double change = paidAmount - totalSell;

    std::cout << "Paid amount: " << paidAmount << std::endl;
    std::cout << "Change: " << change << std::endl;

    if (!finalizeSale(productDb, sellDb, cart)) {
      std::cout << "Failed to complete purchase." << std::endl;
      continue;
    }

    if (!saveTransaction(sellDb, totalSell, paidAmount, change)) {
      std::cout << "Warning: transaction summary was not saved." << std::endl;
    }

    std::cout << "Purchase completed." << std::endl;
    std::cout << "Total money in sell.db: " << getAllTimeTotalMoney(sellDb)
              << std::endl;
  }
}