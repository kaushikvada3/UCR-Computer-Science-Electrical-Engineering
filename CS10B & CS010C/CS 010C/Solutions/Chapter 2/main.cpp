#include <iostream>
#include "IntList.h"
using namespace std;

int main() 
{
    int tests;
    cout << "Enter test number" << endl;
    cin >> tests; // take in test number to check different functions -- all check constructor, cout, printreverse
    cout << endl;
    if (tests == 1) // tests constructor, push_front, cout, printreverse
    {
        IntList list1;
        cout << "list1 constructor called" << endl; // constructor line didn't error
        cout << "push_front 15" << endl;            // print before every push_front to see if there's an error following it
        list1.push_front(15);
        cout << "push_front 10" << endl;
        list1.push_front(10);
        cout << "push_front 0" << endl; // check for case when data is 0
        list1.push_front(0);
        cout << "push_front -5" << endl; // check for case when data is negative
        list1.push_front(-5);
        cout << "list 1: " << list1 << endl; // print entire list in order using out function (0 5 10 15)
        cout << "Print list backwards: ";
        list1.printReverse(); // print entire list backwards (15 10 5 0)
        cout << endl;
    }

    // tests constructor, pop_front, cout, printreverse
    if (tests == 2)
    {
        IntList list2;
        cout << "list2 constructor called" << endl;
        list2.push_front(15); // didn't print push_front statements because it was tested in test1
        list2.push_front(10);
        list2.push_front(0);
        list2.push_front(-5);
        cout << "list 2: " << list2 << endl; // print list to make sure it turned out correctly
        cout << "Print list backwards: ";
        list2.printReverse();
        cout << endl;
        cout << endl
             << "pop_front" << endl; // print each pop front statement, folllowed by entire list to make sure node was removed correctly
        list2.pop_front();
        cout << "list 2: " << list2 << endl;
        cout << endl
             << "pop_front" << endl;
        list2.pop_front();
        cout << "list 2: " << list2 << endl;
        cout << endl
             << "pop_front" << endl;
        list2.pop_front();
        cout << "list 2: " << list2 << endl;
        cout << endl
             << "pop_front" << endl;
        list2.pop_front();
        cout << "Empty list 2: " << list2 << endl; // print empty list to check if print and reverse dont crash with empty list
        cout << "Print empty list backwards: ";
        list2.printReverse();
        cout << endl;
    }
    if (tests == 3) // tests constructor, push_back, cout, printreverse
    {
        IntList list3;
        cout << "list3 constructor called" << endl; // constructor line didn't error
        cout << "push_back -5" << endl;             // print before every push_back to see if there's an error following it
        list3.push_back(-5);                        // test for case when data is negative
        cout << "push_back 0" << endl;              // test for case when data is 0
        list3.push_back(0);
        cout << "push_back 10" << endl;
        list3.push_back(10);
        cout << "push_back 15" << endl;
        list3.push_back(15);
        cout << "list 3: " << list3 << endl; // print entire list in order using out function (0 5 10 15)
        cout << "Print list backwards: ";
        list3.printReverse(); // print entire list backwards (15 10 5 0)
        cout << endl;
    }

    // tests constructor, pop_back, cout, printreverse
    if (tests == 4)
    {
        IntList list4;
        cout << "list4 constructor called" << endl;
        list4.push_back(-5); // didn't print push_back statements because it was tested in test1
        list4.push_back(0);
        list4.push_back(10);
        list4.push_back(15);
        cout << "list 4: " << list4 << endl; // print list to make sure it turned out correctly
        cout << "Print list backwards: ";
        list4.printReverse();
        cout << endl;
        cout << endl
             << "pop_back" << endl; // print each pop back statement, folllowed by entire list to make sure node was removed correctly
        list4.pop_back();
        cout << "list 4: " << list4 << endl;
        cout << endl
             << "pop_back" << endl;
        list4.pop_back();
        cout << "list 4: " << list4 << endl;
        cout << endl
             << "pop_back" << endl;
        list4.pop_back();
        cout << "list 4: " << list4 << endl;
        cout << endl
             << "pop_back" << endl;
        list4.pop_back();
        cout << "Empty list 4: " << list4 << endl; // print empty list to check if print and reverse dont crash with empty list
        cout << "Print empty list backwards: ";
        list4.printReverse();
        cout << endl;
    }

    return 1;
}