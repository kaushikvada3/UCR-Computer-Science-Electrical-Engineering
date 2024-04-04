#include <iostream>
#include "IntList.h"
using namespace std;


//default constructor with defualt values and sets the head to the tail and the tail's previous node to the head
IntList::IntList()
{
  dummyHead = new IntNode(0);
  dummyTail = new IntNode(0);
  dummyHead->next = dummyTail;
  dummyTail->prev = dummyHead;
}

//destructor
IntList::~IntList()
{
  while(!(empty()))
  {
    pop_front();
  }
  delete dummyHead;
  delete dummyTail;
}

//all it does is that it adds the node to the front of the list (only after the dummyhead though)
void IntList::push_front(int value) {
  IntNode*newVal = new IntNode(value);
  IntNode*prevOne = dummyHead->next;
  prevOne->prev = newVal;
  newVal->next = prevOne;
  newVal->prev = dummyHead;
  dummyHead->next = newVal;
}

//this function only removes the first node, but only after the dummyhead)
void IntList::pop_front() {
  if(!empty())
  {
    IntNode *NodeRemove = new IntNode(0);
    NodeRemove = dummyHead->next;
    IntNode *secondNodeAfterHead = dummyHead->next->next;
    IntNode *headNode = dummyHead;
    secondNodeAfterHead->prev = headNode;
    headNode->next = secondNodeAfterHead;
    delete NodeRemove;
  }
}

//inserts the node at the end of the list, but only BEFORE the dummyTail
void IntList::push_back(int value) {
    IntNode *newNode = new IntNode(value);
    IntNode *beforeTail = new IntNode(0);
    beforeTail = dummyTail->prev;
    dummyTail->prev = newNode;
    newNode->next = dummyTail;
    beforeTail->next = newNode;
    newNode->prev = beforeTail;
}


//removes the node from the end of the list (but thats only before the dummyTail)
void IntList::pop_back() {
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

//this function checks to see if there are any nodes between dummyhead or dummytail
bool IntList::empty() const {
  return dummyHead->next == dummyTail;
}

//outputs the list backwards starting from the node thats before the dummytail and 
//it seperates each with a space betwen each one. 
void IntList::printReverse() const {
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

//the cout operator overloaded where it operates the list (after the dummyhead) 
//with a space in between (and there is no spaces at the end)
#include <iostream>
ostream & operator<<(ostream &out, const IntList &rhs) {
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