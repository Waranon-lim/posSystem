#include <iostream>

using namespace std;

int main()
{
    int num;
    cout << "1. Register \n";
    cout << "2. Login\n";
    cout << "3. Exit " << endl;
    cin >> num;

    if(num==1)
    {
        cout << "call register function" << endl;
    }
    else if(num==2)
    {
        cout << "call login function" << endl;
    }
    else
    {
        cout << "your exit the program";
        return 0;
    }
    return 0;
}
