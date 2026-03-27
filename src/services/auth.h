#pragma once
#include<string>

bool isNameExit(std::string username);
void registerAccount();
long long int generateHash(std::string password);
void initDatabase();