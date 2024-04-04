#include <iostream>
#include "SortedSet.h"

using namespace std;

SortedSet::SortedSet():IntList(){}

SortedSet::SortedSet(const SortedSet &input):IntList(input){ //SortedSet copy constructor
}

SortedSet::SortedSet(const IntList &input):IntList(input){ //IntList copy constructor
    IntList::remove_duplicates();
    IntList::selection_sort();
}

SortedSet::~SortedSet(){}

bool SortedSet::in(int value){
    IntNode *temp = head;
    while(temp != nullptr){
        if(temp->value == value)
            return true;
        temp = temp->next;
    }
    return false;
}

SortedSet & SortedSet::operator|(SortedSet &input){
	SortedSet* ret = new SortedSet();
	if (input.head == nullptr){
		ret = this;
	}

	else if (head == nullptr){
		ret = &input;
	}

	else{
		IntNode* temp1 = head;
		IntNode* temp2 = input.head;
		if (temp1->value < temp2->value){
			ret->push_back(temp1->value);
			temp1 = temp1->next;
		}
		else {
			ret->push_back(temp2->value);
			temp2 = temp2->next;
		}
		while (temp1 != nullptr && temp2 != nullptr){
			if (temp1->value < temp2->value) {
				ret->push_back(temp1->value);
				temp1 = temp1->next;
			}
			else{
				ret->push_back(temp2->value);
				temp2 = temp2->next;
			}
		}
		if (temp1 != nullptr) {
			while (temp1 != nullptr) {
				ret->push_back(temp1->value);
				temp1 = temp1 -> next;
			}
		}
		else{
			while (temp2 != nullptr) {
				ret->push_back(temp2->value);
				temp2 = temp2->next;
			}
		}
	}
	return *ret;
}

SortedSet & SortedSet::operator&(SortedSet &input){
    SortedSet *ret = new SortedSet();
    IntNode *temp = head;
    while(temp != nullptr){
        if(in(temp->value) && input.in(temp->value))
            ret->push_back(temp->value);
        temp = temp->next;
    }
    temp = input.head;
    while(temp != nullptr){
        if(in(temp->value) && input.in(temp->value) && !ret->in(temp->value))
            ret->push_back(temp->value);
        temp = temp->next;
    }

    return *ret;
}


void SortedSet::add(int value){
    if(!in(value)){
        IntList::insert_ordered(value);
    }
}

void SortedSet::push_front(int value){
    add(value);
}

void SortedSet::push_back(int value){
    add(value);
}

void SortedSet::insert_ordered(int value){
    add(value);
}

SortedSet & SortedSet::operator|=(SortedSet &input){
    if(input.head != nullptr){
        SortedSet *ret = &(*this | input);
        this->clear();
        IntNode *temp = ret->head;
        while(temp != nullptr){
            this->push_back(temp->value);
            temp = temp->next;
        }
        return *ret;
    }
    else
        return *this;
}
SortedSet & SortedSet::operator&=(SortedSet &input){
    SortedSet *ret = &(*this & input);
    this->clear();
    IntNode *temp = ret->head;
    while(temp != nullptr){
        this->push_back(temp->value);
        temp = temp->next;
    }
    return *ret;
}