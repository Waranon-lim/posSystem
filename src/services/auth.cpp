#include "auth.h"
#include <iostream>
#include <sstream>
#include <sqlite3.h>

extern sqlite3* db; // tell this file that db will import from another file

bool isNameExit(std::string username)
{
    sqlite3_stmt *stmt;
    int count = 0;

    std::stringstream ss;
    ss << "SELECT COUNT(*) FROM users WHERE username = '" << username << "';";
    std::string query = ss.str();

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
    }
    
    sqlite3_finalize(stmt);
    
    return (count > 0);
}

void initDatabase()
{
    char *errorMessage;
    if (sqlite3_open("db/user.db", &db) == SQLITE_OK)
    {
        std::string sql = "CREATE TABLE IF NOT EXISTS users ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "username TEXT UNIQUE NOT NULL, "
                          "password TEXT NOT NULL, " 
                          "role TEXT DEFAULT 'customer', "
                          "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

        int exit = sqlite3_exec(db, sql.c_str(), NULL, 0, &errorMessage);

        if (exit != SQLITE_OK)
        {
            std::cerr << "Error Creating Table: " << errorMessage << std::endl;
            sqlite3_free(errorMessage);
        }
        
    }
}

long long int generateHash(std::string password)
{

    long int hash = 0;
    long int prime = 31;
    for (int i = 0; i < password.length(); i++)
    {
        hash = (hash * prime) + password[i];
    }
    return hash;
}


void registerAccount() {
    std::string username, password, confirmPassword;
    bool nameExits = false;

   
    do {
        std::cout << "Enter your username : ";
        std::cin >> username;
        nameExits = isNameExit(username); 
        if (nameExits) {
            std::cout << "This name already exists, please try again." << std::endl;
        }
    } while (nameExits);

    do {
        std::cout << "Enter your password : ";
        std::cin >> password;
        std::cout << "Confirm your password : ";
        std::cin >> confirmPassword;

        if (password != confirmPassword) {
            std::cout << "Passwords do not match, please try again." << std::endl;
        }
    } while (password != confirmPassword);

    if (db == nullptr) {
        std::cerr << "Database not initialized!" << std::endl;
        return;
    }

   
    std::stringstream ss;
    ss << "INSERT INTO users (username, password) VALUES ('" 
       << username << "', '" << password << "');"; 

    std::string sql = ss.str();
    char* errorMessage = nullptr;

    int exit = sqlite3_exec(db, sql.c_str(), NULL, 0, &errorMessage);

    if (exit != SQLITE_OK) {
        std::cerr << "SQL Error: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
    } else {
        std::cout << "Success! Your account has been created: " << username << std::endl;
    }
   
}

// -------------------------------login function -------------------------------

bool loginUser(sqlite3 *db, const std::string &username, const std::string &password)
{
    sqlite3_stmt *stmt;

    std::string sql = "SELECT id FROM users WHERE username = ? AND password = ?;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    bool success = false;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        success = true;
    }

    sqlite3_finalize(stmt);
    return success;
}