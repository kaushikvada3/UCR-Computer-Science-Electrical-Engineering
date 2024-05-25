#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;

const int MAX_SIZE = 20; // constant variable

#ifndef STACK_H
#define STACK_H

template <class T> // defined class template to use type T
class stack
{
private:
    T *data;
    int size;

public:
    /*
    function: stack constructor
    description: constructs an empty stack
        -initializes data with type T and size of MAX_SIZE
        -initializes size with 0 (no elements in array)
    */
    stack()
    {
        data = new T[MAX_SIZE];
        size = 0;
    }

    /*
    function: push
        void return type
    parameters: type T val, value to be added to the back of the array
    description: inserts a new element (val) of type T (T could be integer or string) into the data
        -If the data array is full, this function should throw an overflow_error exception
            -error message: "Called push on full stack.".
        -adds val to end of array (index size)
        -increases size by one to consider the new added value
    */
    void push(T val)
    {
        if (size == MAX_SIZE)
        {
            throw overflow_error("Called push on full stack.");
        }
        data[size] = val;
        size++;
    }

    /*
    function: pop
        void return type
    description: removes the last element from data
        -If the data array is empty, this function should throw an outofrange exception
            -error message: "Called pop on empty stack.".
        -reduces size by one for removed value
    */
    void pop()
    {
        if (empty())
        {
            throw out_of_range("Called pop on empty stack.");
        }
        size--;
    }

    /*
    function: pop_two
        void return type
    description: removes the last two element from data
        -If the data array is empty or is of size 1, this function should throw an out_of_range exception
            -If empty then the error message should be "Called pop_two on empty stack."
            -If the size is 1 then the error message should be "Called pop_two on a stack of size 1.".
        -reduces size by two for removed values
    */
    void pop_two()
    {
        if (empty())
        {
            throw out_of_range("Called pop_two on empty stack.");
        }
        if (size == 1)
        {
            throw out_of_range("Called pop_two on a stack of size 1.");
        }

        size = size - 2;
    }

    /*
    function: top
        return type T
    description: returns the top element of stack (last inserted element).
        -If stack is empty, this function should throw an underflow_error exception
            -error message: "Called top on empty stack."
        -else, returns data at size-1 (last value)
    */
    T top()
    {
        if (size == 0)
        {
            throw underflow_error("Called top on empty stack.");
        }
        return data[size - 1];
    }

    /*
    function: empty
        boolean return type
    description: returns true if the stack is empty otherwise it returns false
        -if size is 0, then there's no elements in the stack
    */
    bool empty()
    {
        return (size == 0);
    }
};
#endif