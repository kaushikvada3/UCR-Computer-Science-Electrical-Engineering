#include <iostream>
#include "IntList.h"

using namespace std;
IntList::IntList():head(0),tail(0){}

IntList::~IntList(){
    while (!empty()){
        pop_front();
    }
    head = nullptr;
    tail = nullptr;
}

void IntList::push_front(int value){
    IntNode *temp = new IntNode(value);
    if(empty()){
        temp-> next = nullptr;
        head = temp;
        tail = temp;
    }
    else{
        temp->next = head;
        head = temp;
    }
}

void IntList::pop_front(){
    IntNode *temp = head;
    if(empty()){}

    else if(head->next == nullptr){
        tail = nullptr;
        head = nullptr;
        delete temp;
    }
    else{
        head = temp->next;
        delete temp;
    }
}

bool IntList::empty() const{
    return head == nullptr;
}

const int & IntList::front() const{
    return head->value;

}
const int & IntList::back() const{
    return tail->value;
}

ostream & operator<<(ostream &out, const IntList &inputVal){
    IntNode *currVec = inputVal.head;
    if(currVec != nullptr){
        out << currVec->value;
        currVec = currVec->next;
    }
    while(currVec != nullptr){
        out << " " << currVec->value;
        currVec = currVec->next;
    }
    return out;
}

void IntList::clear() {
    while (!empty()) {
        pop_front();
    }
}



//no loop in push_back or back