#include <iostream>
using namespace std;

#ifndef INTLIST_H
#define INTLIST_H

//given struct
struct IntNode
{
    int data;
    IntNode *prev;
    IntNode *next;
    IntNode(int data) : data(data), prev(0), next(0) {}
};

class IntList
{
private:
    IntNode *dummyHead;
    IntNode *dummyTail;

public:
    // constructor
    IntList();
    // destructor
    ~IntList();
    // mutator functions
    void push_front(int value);
    void pop_front();
    void push_back(int value);
    void pop_back();
    
    bool empty() const;
    // printing functions
    friend ostream &operator<<(ostream &out, const IntList &rhs);
    void printReverse() const;
};

#endif