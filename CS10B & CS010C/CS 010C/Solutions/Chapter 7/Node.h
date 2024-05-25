#include <iostream>
using namespace std;

#ifndef NODE_H
#define NODE_H

class Node
{
    friend class AVLTree; // AVLTree can access call private functions as a friend class
private:
    string word;
    Node *left;
    Node *right;
    Node *parent;
    int height;

public:
    // constructors
    Node();
    Node(string word);

};
#endif