#include <iostream>
#include "Node.h"
using namespace std;

Node::Node()
{
    // initialize all the child pointers to null
    left = nullptr;
    middle = nullptr;
    right = nullptr;

    // set the small and large values to empty strings
    small = "";
    large = "";

    // set the parent pointer to null
    parent = nullptr;
}

/*
 * initializes the small value to the inputed value and sets the large value to an empty string
 * initializes all the child pointers to null and sets the parent pointer to null
 */
Node::Node(string val)
{
    small = val;
    large = "";

    // initialize all the child pointers to null
    left = nullptr;
    middle = nullptr;
    right = nullptr;

    // set the parent pointer to null
    parent = nullptr;
}

/* checks if all the child pointers are null, indicating that the node is a leaf */
bool Node::isLeaf() const
{
    // check if all the child pointers are null
    return (left == nullptr && right == nullptr && middle == nullptr);
}

/* returns true if both small and large contain a value and checks if both the small and large values contain a value */
bool Node::isFull() const
{
    // check if both the small and large values contain a value
    return (small != "" && large != "");
}

/**
  this function checks to see if the inputed key is already in the node by comparing the small and large values
 */
bool Node::containsKey(string key) const
{
    // check if the inputed key is already in the node by comparing the small and large values
    return (small == key || large == key);
}