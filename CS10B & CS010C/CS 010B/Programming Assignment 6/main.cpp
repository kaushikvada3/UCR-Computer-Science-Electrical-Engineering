#include "SortedSet.h"
#include <iostream>
using namespace std;

int main() {
    SortedSet set1;

    cout << "Adding elements to set1" << endl;
    set1.add(5);
    set1.add(3);
    set1.add(9);

   
    cout << "Checking if 3 is in set1: ";
    if (set1.in(3)) {
        cout << "Yes" << endl;
    } else {
        cout << "No" << endl;
    }

    cout << "Checking if 4 is in set1: ";
    if (set1.in(4)) {
        cout << "Yes" << endl;
    } else {
        cout << "No" << endl;
    }

    return 0;
}
