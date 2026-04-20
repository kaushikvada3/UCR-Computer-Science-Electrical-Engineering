#pragma once

#include "Node.h"

class BSTree {
  Node* root;

public:
  BSTree()
    : root(nullptr)
  {
  }
  ~BSTree() {
    // Safe to delete nullptr, so no check needed
    // The Node struct recursively deletes its children
    delete root;
  }
  BSTree(const BSTree&) = delete;
  BSTree& operator=(const BSTree&) = delete;
  
  void insert(const string& key);
  bool search(const string& key) const;
  string largest() const;
  string smallest() const;
  int height(const string& key) const;
  void remove(const string& key);

  void preOrder() const;
  void postOrder() const;
  void inOrder() const;
  
  // PROFPAT: This is a helpful thing to see what your tree really looks like
  // PROFPAT: You should delete it before submitting (code turd)
  void debug() const {
    debug(root,0);
  }

private:
  // PROFPAT: You will probably want to write these helper functions to do the
  // PROFPAT: work above recursively since it is more natural
  void remove_helper(Node* parent, Node* curr, const string& key);
  int height_of(Node* tree) const;
  void preOrder(Node* tree) const;
  void postOrder(Node* tree) const;
  void inOrder(Node* tree) const;

};
