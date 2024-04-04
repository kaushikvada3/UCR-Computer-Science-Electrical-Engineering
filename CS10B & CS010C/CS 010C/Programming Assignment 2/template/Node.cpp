#include "Node.h"

#include <iostream>

using namespace std;

Node::Node()
    : count(0),left(nullptr),right(nullptr)
{
}

Node::Node(const string& s)
    : key(s),count(1),left(nullptr),right(nullptr)
{
}

// Tear down my left and right subtrees when deleted
Node::~Node() {
  // delete is safe on a nullptr.  No need to check
  delete left;
  delete right;
}

ostream& operator<< ( ostream &os, const Node &a ) {
  os << a.key << "(" << a.count << ")";
  return os;
}

