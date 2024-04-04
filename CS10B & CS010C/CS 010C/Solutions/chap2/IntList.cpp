#include <iostream>
#include "IntList.h"
using namespace std;

// default constructor
IntList::IntList()
{
    dummyHead = new IntNode(0);
    dummyTail = new IntNode(0);
    dummyHead->next = dummyTail;
    dummyTail->prev = dummyHead;
}

// destructor uses pop_front to delete each node if list isn't empty, then delete dummyhead and tail
IntList::~IntList()
{
    while (!(empty()))
    {
        pop_front();
    }
    delete dummyHead;
    delete dummyTail;
}

// push_front adds node to the front of the list (after dummyhead)
void IntList::push_front(int value)
{
    IntNode *newVal = new IntNode(value);
    IntNode *prevOne = dummyHead->next; 
    prevOne->prev = newVal;
    newVal->next = prevOne;
    newVal->prev = dummyHead;
    dummyHead->next = newVal;
}

// pop_front removed first node from list (after dummyhead)
void IntList::pop_front()
{
    if (!empty())
    {
        IntNode *nodeToRemove = new IntNode(0);
        nodeToRemove = dummyHead->next; 
        IntNode *twoNodesAfterHead = dummyHead->next->next;
        IntNode *headNode = dummyHead;
        twoNodesAfterHead->prev = headNode;
        headNode->next = twoNodesAfterHead;
        delete nodeToRemove;
    }
}

// push_back inserts node at the end of list (before dummytail)
void IntList::push_back(int value)
{
    IntNode *newNode = new IntNode(value);
    IntNode *beforeTail = new IntNode(0);
    beforeTail = dummyTail->prev;
    dummyTail->prev = newNode;
    newNode->next = dummyTail;
    beforeTail->next = newNode;
    newNode->prev = beforeTail;
}

// pop_back removes node from the end of list(before dummytail)
void IntList::pop_back()
{
    if (!empty())
    {
        IntNode *nodeToRemove = new IntNode(0);
        nodeToRemove = dummyTail->prev;
        IntNode *beforeTail = new IntNode(0);
        beforeTail = dummyTail->prev->prev;
        beforeTail->next = dummyTail;
        dummyTail->prev = beforeTail;
        delete nodeToRemove;
    }
}

// checks if there's any nodes between dummyhead or dummytail
bool IntList::empty() const
{
    return dummyHead->next == dummyTail;
}

// outputs list starting with node after dummyhead with space in the middle (no space at the end)
ostream &operator<<(ostream &out, const IntList &rhs)
{
    if (rhs.dummyHead->next != rhs.dummyTail)
    {
        IntNode *outputNode = rhs.dummyHead->next;
        while (outputNode->next->next != rhs.dummyTail->next)
        {
            out << outputNode->data << " ";
            outputNode = outputNode->next;
        }
        out << outputNode->data;
    }
    return out;
}

// outputs list backwards starting at node before tail with a space between each one (no space at the end)
void IntList::printReverse() const
{
    if (dummyHead->next != dummyTail)
    {
        IntNode *outputNode = dummyTail->prev;
        while (outputNode->prev->prev != dummyHead->prev)
        {
            cout << outputNode->data << " ";
            outputNode = outputNode->prev;
        }
        cout << dummyHead->next->data;
    }
}