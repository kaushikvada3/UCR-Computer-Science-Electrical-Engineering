#include <iostream>
#include "IntVector.h"
using namespace std;

int main(){
    int cap = 3, vals = 1;
    IntVector myIntVectorPars(cap,vals);
    IntVector myIntVectorNoPars;
    
    cout << ".size(): " << endl;
    cout << "Parameter size: " << myIntVectorPars.size() << endl;
    cout << "No parameter size:" << myIntVectorNoPars.size() << endl;

    cout << endl;
    cout << ".capacity(): " << endl;
    cout << "Parameter capacity: " << myIntVectorPars.capacity() << endl;
    cout << "No parameter capacity: " << myIntVectorNoPars.capacity() << endl;

    cout << endl;

    cout << ".empty(): " << endl;
    if(myIntVectorPars.empty())
        cout << "Parameter vector is empty" << endl;
    else 
        cout << "Parameter vector is not empty" << endl;
    if(myIntVectorNoPars.empty())
        cout << "No parameter vector is empty" << endl;
    else 
        cout << "No parameter vector is not empty" << endl;
    cout << endl;

    cout << ".at(): " << endl;
    cout << myIntVectorPars.at(0) << endl;
    cout << myIntVectorPars.at(1) << endl;
    cout << myIntVectorPars.at(2) << endl;
    cout << endl;
    int siz = 10, val = 1;
    IntVector myVecAt(siz, val);
    cout << "Legal index: " << myVecAt.at(8) << endl;
    cout << myVecAt.at(-1) << endl;
    cout << myVecAt.at(10) << endl;
    cout << endl;

    cout << ".front(): " << endl;
    cout << myIntVectorPars.front() << endl;
    cout << ".back():" << endl;
    cout << myIntVectorPars.back() << endl;
    cout << endl;

    cout << ".insert()" << endl;
    myVecAt.insert(10,3);
    
    myIntVectorNoPars.push_back(3);
    myIntVectorNoPars.push_back(7);
    myIntVectorNoPars.push_back(10);

    myIntVectorNoPars.erase(1);
    myIntVectorNoPars.pop_back();

    cout << myIntVectorNoPars.at(1) << endl;

    myIntVectorNoPars.clear();
    myIntVectorNoPars.push_back(1);
    cout << myIntVectorNoPars.at(0) << endl;

    myIntVectorNoPars.push_back(3);
    myIntVectorNoPars.push_back(7);
    myIntVectorNoPars.push_back(10);

    myIntVectorNoPars.assign(5,10);
    cout << myIntVectorNoPars.at(3) << endl;
}