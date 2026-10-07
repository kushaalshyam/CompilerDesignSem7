#include <iostream>
using namespace std;
// single line comment
/* multi
   line comment */
int main()
{
    int x = 0x1F, count = 5;
    float pi = 3.14;
    char ch = 'A';
    bool done = false;
    x += 2;
    count++;
    if (x <= 10 && count != 0) {
        cout << "ok\n" << ch << endl;
    }
    while (x > 0) {
        x--;
    }
    return 0;
}
