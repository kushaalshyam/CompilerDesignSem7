#include <iostream>
#include <cmath>
using namespace std;

int main() {
    int n = 6, f = 1, k = 2;
    int p = pow(k, n);
    int q = n % 4 + (n - n);
    while (n > 1) {
        f = f * n;
        n = n - 1;
    }
    int r = f / 3 + f * 2;
    int s = f / 3 + f * 2;
    int t = -f + p * 0 + q;
    // a loop with a branch inside
    int j = 0;
    while (j < 4) {
        if (j % 2 == 0)
            cout << j * j << endl;
        else
            cout << f - j << endl;
        j = j + 1;
    }
    cout << r + s << endl;
    cout << t << endl;
    cout << p << endl;
    return 0;
}
