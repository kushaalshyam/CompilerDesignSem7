#include <iostream>
#include <cmath>
using namespace std;

int main() {
    int a = 10;
    int b = 20;
    int c = 2 * 3 + a * 1;
    int d = pow(b, 2);
    int sum = 0, i = 1;
    while (i <= 5) {
        sum = sum + i * 2;
        i = i + 1;
    }
    int x1 = sum + i;
    int x2 = sum + i;
    int e = sum * 2;
    if (a < b) {
        cout << "b is greater" << endl;
    } else {
        cout << "a is greater" << endl;
    }
    cout << c + d << endl;
    return 0;
}
