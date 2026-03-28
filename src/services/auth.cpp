#include "auth.h"
#include <iostream>
#include <sstream>
#include <sqlite3.h>

bool isNameExit(std::string username)
{
    sqlite3 *DB;
    sqlite3_stmt *stmt;
    int count = 0;
    bool exists = false;

    if (sqlite3_open("user.db", &DB) != SQLITE_OK)
    {
        std::cerr << "Error: Cannot open database!" << std::endl;
        return false;
    }

    std::stringstream ss;
    ss << "SELECT COUNT(*) FROM users WHERE username = '" << username << "';";
    std::string query = ss.str();

    if (sqlite3_prepare_v2(DB, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
    }
    else
    {
        std::cerr << "Error: Failed to prepare statement!" << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(DB);

    return (count > 0);
}

void initDatabase()
{
    sqlite3 *DB;
    char *errorMessage;
    if (sqlite3_open("user.db", &DB) == SQLITE_OK)
    {
        std::string sql = "CREATE TABLE IF NOT EXISTS users ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "username TEXT UNIQUE NOT NULL, "
                          "password_hash INTEGER NOT NULL, "
                          "role TEXT DEFAULT 'customer', "
                          "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

        int exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &errorMessage);

        if (exit != SQLITE_OK)
        {
            std::cerr << "Error Creating Table: " << errorMessage << std::endl;
            sqlite3_free(errorMessage);
        }
        else
        {
        }

        sqlite3_close(DB);
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

void registerAccount()
{
    std::string username, password, confirmPassword;
    bool nameExits = false;

    do
    {
        std::cout << "Enter your username : ";
        std::cin >> username;

        nameExits = isNameExit(username);
        if (nameExits)
        {
            std::cout << "this name is already exist pls try again : " << std::endl;
        }
    } while (nameExits);

    do
    {
        std::cout << "Enter your password : ";
        std::cin >> password;

        std::cout << "Confirm your password : ";
        std::cin >> confirmPassword;

        if (password != confirmPassword)
        {
            std::cout << "password does not match pls try again : " << std::endl;
        }
        else
        {
            std::cout << "Success! Your account has been created ";
        }
    } while (password != confirmPassword);

    long long int hash = generateHash(password);

    sqlite3 *DB;
    char *errorMessage;

    if (sqlite3_open("user.db", &DB) == SQLITE_OK)
    {

        std::stringstream ss;
        ss << "INSERT INTO users (username, password_hash) VALUES ('"
           << username << "', " << hash << ");";

        std::string sql = ss.str();

        int exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &errorMessage);

        if (exit != SQLITE_OK)
        {
            std::cerr << "Error: " << errorMessage << std::endl;
            sqlite3_free(errorMessage);
        }
        else
        {
            std::cout << "Successfully registered: " << username << std::endl;
        }

        sqlite3_close(DB);
    }
}