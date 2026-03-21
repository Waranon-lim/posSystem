#pragma once
#include <string>

struct Product {
    int         id       = 0;
    std::string name;
    double      price    = 0.0;
    int         stock    = 0;
    std::string category;
};
