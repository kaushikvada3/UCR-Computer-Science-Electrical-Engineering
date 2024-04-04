#include <iostream>
#include "IntList.h"

using namespace std;
IntList::IntList():head(0),tail(0){}

IntList::~IntList(){
    clear();
}

void IntList::push_front(int value){
    IntNode *curr = new IntNode(value);
    if(empty()){
        curr-> next = nullptr;
        head = curr;
        tail = curr;
    }
    else{
        curr->next = head;
        head = curr;
    }
}

void IntList::pop_front(){
    IntNode *curr = head;
    if(empty()){}

    else if(head->next == nullptr){
        tail = nullptr;
        head = nullptr;
        delete curr;
    }
    else{
        head = curr->next;
        delete curr;
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

IntList::IntList(const IntList &cpy){
    IntNode *curr = cpy.head;
    head = nullptr;
    tail = nullptr;
    while(curr != nullptr){
        push_back(curr->value);
        curr = curr->next;
    }
}

IntList& IntList::operator=(const IntList &rhs){
    if(this != &rhs){
        this->clear();
        IntNode *curr = rhs.head;
        while(curr != nullptr){
            push_back(curr->value);
            //*head = *(curr);
            curr = curr->next;
        }
    }
    return *this;
}

void IntList::push_back(int value){
    IntNode *curr = new IntNode(value);
    if(empty()){
        head = curr;
        tail = curr;
    }
    else{
        tail->next = curr;
        tail = curr;
        curr->next = nullptr;
    }
}

void IntList::clear(){
    while (!empty()){
        pop_front();
    }
    head = nullptr;
    tail = nullptr;
}

void IntList::selection_sort(){
    for(IntNode *i = head; i != nullptr; i = i->next){
        for(IntNode *prev = i->next; prev != nullptr; prev = prev->next){
            if(i->value > prev->value){
                int currVal = prev->value;
                prev->value = i->value;
                i->value = currVal;
            }
        }
    }
}

void IntList::insert_ordered(int value){
    if(head == nullptr){
        push_front(value);
    }
    else if(head->next == nullptr || value < head->value){
        if(value <= head->value)
            push_front(value);
        else
            push_back(value);
    }
    else {
        for(IntNode *first = head; first->next != nullptr; first = first->next){
            IntNode *second = first->next;
            if(second->value >= value){
                IntNode *newVal = new IntNode(value);
                first->next = newVal;
                newVal->next = second;
                return;
            }
        }
        push_back(value);    
    }
}
//30 30 10
void IntList::remove_duplicates(){
    if(empty()){
        return;
    }
    for(IntNode *i = head; i != nullptr; i = i->next){
        IntNode *prev = i;
        while(prev->next != nullptr){
            IntNode *curr = prev->next;
            if(i->value == curr->value)
            {
                prev->next = curr->next;
                if(curr == tail)
                {
                    tail = prev;
                }
                delete curr;
                curr = nullptr; 
            }
            else{
                prev = prev->next;
            }
        }
    
    } 

}

