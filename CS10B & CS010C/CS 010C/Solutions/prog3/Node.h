#ifndef __NODE_H
#define __NODE_H

#include <string>

using namespace std;

class Node
{
    friend class Tree;

private:
    string small; // left key
    string large; // right key

    Node *left; // left child
    Node *middle; // middle child
    Node *right; // right child
    Node *parent;
public:
    Node();
    Node(string val);

    // Add additional functions/variables here. Remember, you may not add any
    // Node * or string variables.
    bool isLeaf() const;
    bool isFull() const;\
    bool containsKey(string key) const;
};

#endif
