#include <iostream>
using namespace std;

#ifndef NODE_H
#define NODE_H

class Node
{
private:
    string word;
    int count;
    Node *left;
    Node *right;

public:
    // constructors
    Node();
    Node(string word);

    // accessors
    int getCount();
    string getWord();
    Node* getLeft();
    Node* getRight();
    
    //mutators
    void setCount(int count);
    void setWord(string word);
    void setLeft(Node* input);
    void setRight(Node* input);
};
#endif