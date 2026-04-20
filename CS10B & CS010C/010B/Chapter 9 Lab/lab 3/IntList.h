#ifndef __INTLIST_H__
#define __INTLIST_H__

#include <ostream>

using namespace std;

struct IntNode {
    int value;
    IntNode *next;
    IntNode(int value) : value(value), next(nullptr) {}
};


class IntList {

 private:
   IntNode *head;

 public:

   /* Initializes an empty list.
   */
   IntList();

   /* Inserts a data value to the front of the list.
   */
   void push_front(int);

   /* Outputs to a single line all of the int values stored in the list, each separated by a space. 
      This function does NOT output a newline or space at the end.
   */
   friend ostream & operator<<(ostream &out, const IntList &input);

   /* Returns true if the integer passed in is contained within the IntList.
      Otherwise returns false.
   */
   bool exists(int) const;

 private:
   bool exists(IntNode *i, int input) const;
   
};
ostream & operator<<(ostream &out, IntNode *input);

#endif