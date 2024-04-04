#ifndef SORTEDSET_H
#define SORTEDSET_H

#include <ostream>
#include "IntList.h"

using namespace std;

class SortedSet:public IntList {
    public:
        SortedSet();
        SortedSet(const SortedSet &input);
        SortedSet(const IntList &input);
        ~SortedSet();

        bool in(int value);
        SortedSet & operator|(SortedSet &input);
        SortedSet & operator&(SortedSet &input);

        void add(int value);
        void push_front(int value);
        void push_back(int value);
        void insert_ordered(int value);
        SortedSet & operator|=(SortedSet &input);
        SortedSet & operator&=(SortedSet &input);
};

#endif