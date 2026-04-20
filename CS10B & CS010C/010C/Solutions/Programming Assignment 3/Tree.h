#ifndef __TREE_H
#define __TREE_H

#include "Node.h"

class Tree
{
private:
  Node *root;

public:
// given public member functions to implement
// constructors
  Tree();
  ~Tree();

  // mutators
  void insert(const string &add);
  void remove(const string &removeWord);

  // accessors
  bool search(const string &input) const;

  // printing
  void preOrder() const;
  void inOrder() const;
  void postOrder() const;

private:
  // Add additional functions/variables here

  // insert helper functions
  void insert(Node *compareNode, const string &add);
  void insertInLeaf(Node *insertNode, const string &add);
  void insertNonFull(Node *insertNode, const string &add);
  void insertNonFullParent(Node *insertNode, const string &add);

  // remove helper functions
  void remove(Node* compareNode, const string &removeWord);
  void removeNonFull(Node *removeNode, const string &removeWord);

  // destructor helper function
  void destroyTree(Node *toDestroy);

  // search helper function
  bool search(Node *compareNode, const string &input) const;

  // printing helper functions
  void preOrder(Node *print) const;
  void inOrder(Node *print) const;
  void postOrder(Node *print) const;

};

#endif