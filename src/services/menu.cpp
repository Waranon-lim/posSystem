#include "menu.h"
#include <iostream>

void showMenu()
{
     std::cout << "1. Register \n";
      std::cout << "2. Login\n";
      std::cout << "3. Exit " <<  std::endl;
      std::cout << "choose your number : ";
}

int getChoice()
{
    while (true)
    {

        int num;
         std::cin >> num;
        if(num >= 1 && num <=3)
        return num;
        else 
        {
             std::cout << "number is invalid please try again" <<  std::endl;
             std::cout << "choose your number again : " << std::endl;
             std::cout << "test test : " << std::endl;
        }
    }
}