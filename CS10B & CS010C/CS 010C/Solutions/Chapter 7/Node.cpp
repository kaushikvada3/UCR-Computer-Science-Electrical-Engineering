#include <iostream>
#include "Node.h"
using namespace std;

Node::Node(string word) // constructs a node that initializes everything to null, word to parameter, and count to 1
{
    this->word = word;
    this->left = nullptr;
    this->right = nullptr;
    this->parent = nullptr;
    height = 0;
}

Node::Node() // constructs a node with everything as null, and count as 1
{
    this->word = nullptr;
    this->left = nullptr;
    this->right = nullptr;
    this->parent = nullptr;
    height = 0;
}
