#include "IntList.h"

#include <ostream>

using namespace std;

IntList::IntList() : head(nullptr) {}


void IntList::push_front(int val) {
   if (!head) {
      head = new IntNode(val);
   } 
   else {
      IntNode *temp = new IntNode(val);
      temp->next = head;
      head = temp;
   }
}


ostream & operator<<(ostream &out, const IntList &input){
   if (input.head == nullptr)
      return out;
   operator<<(out,input.head);
   return out;
}


bool IntList::exists(int input) const{
   if(head == nullptr)
      return false;
   if(head ->value == input)
      return true;
   return exists(head->next, input);
}


//helper functions

bool IntList::exists(IntNode *i, int input) const{
   if(i == nullptr){
      return false;
   }
   if(i->value == input){
         return true;
   }
   return this->exists(i->next, input);
}

ostream & operator<<(ostream &out, IntNode *input){
   if(input == nullptr)
      return out;
   out << input->value;
   if(input->next != nullptr){
      out << " ";
   }
   operator<< (out, input->next);
   return out;
}