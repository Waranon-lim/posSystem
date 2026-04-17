#pragma once

#include <sqlite3.h>

#include <string>
#include <vector>

namespace product_db {

struct CartItem {
  int productId;
  std::string productName;
  int quantity;
  double sellPrice;
  double lineTotal;
};

bool isNumber(const std::string& input);
std::string toLower(std::string value);

bool openProductDatabase(sqlite3** db);
bool openSellDatabase(sqlite3** sellDb);

void printProducts(sqlite3* db);
void showSalesRows(sqlite3* sellDb, const std::string& whereClause,
                   const std::string& title);

double getAllTimeTotalMoney(sqlite3* sellDb);

bool findProductBasic(sqlite3* db, const std::string& productInput,
                      int* productId, std::string* productName, int* quantity);
bool findProductForCustomer(sqlite3* productDb, const std::string& productInput,
                            int* productId, std::string* productName,
                            int* availableQty, double* sellPrice);

bool insertProduct(sqlite3* db, const std::string& productName, int quantity,
                   double buyPrice, double sellPrice);
bool updateProductInventory(sqlite3* db, int productId, int addQty,
                            double buyPrice, double sellPrice);

bool hasEnoughStock(sqlite3* productDb, int productId, int requiredQty);
bool finalizeSale(sqlite3* productDb, sqlite3* sellDb,
                  const std::vector<CartItem>& cart);

}  // namespace product_db
