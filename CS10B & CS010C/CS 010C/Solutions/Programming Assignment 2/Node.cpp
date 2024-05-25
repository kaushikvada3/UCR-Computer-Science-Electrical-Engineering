#include <iostream>
#include "Node.h"
using namespace std;

Node::Node(string word) // constructs a node that initializes everything to null, word to parameter, and count to 1
{
    this->word = word;
    this->left = nullptr;
    this->right = nullptr;
    this->count = 1;
}

Node::Node() // constructs a node with everything as null, and count as 1
{
    this->word = nullptr;
    this->left = nullptr;
    this->right = nullptr;
    this->count = 1;
}

//accessor functions
int Node::getCount()
{
    return count;
}

string Node::getWord()
{
    return word;
}

Node *Node::getLeft()
{
    return this->left;
}

Node *Node::getRight()
{
    return this->right;
}

// mutator functions
void Node::setCount(int count)
{
    this->count = count;
}

void Node::setWord(string word)
{
    this->word = word;
}

void Node::setLeft(Node *input)
{
    this->left = input;
}

void Node::setRight(Node *input)
{
    this->right = input;
}