#include <iostream>
#include "Node.h"
using namespace std;

#ifndef BSTREE_H
#define BSTREE_H

class BSTree
{
private:
   Node *root;

public:
   /* Constructors */
   /* Default constructor
         sets root to new Node */
   BSTree();
   ~BSTree();

   /* Mutators */
   /* Function: insert
         parameters: const string
         description: Insert an item into the binary search tree.
            When an item is first inserted into the tree the count should be set to 1.
            When adding a duplicate string, the count variable should be incremented
    */
   void insert(const string &newString);

   /* Remove a specified string from the tree.
      Be sure to maintain all bianry search tree properties.
      If removing a node with a count greater than 1, just decrement the count, otherwise,
      if the count is simply 1, remove the node.
       You MUST follow the remove algorithm shown in the slides and discussed in class or else
       your program will not pass the test functions.
       When removing,
           if removing a leaf node, simply remove the leaf. Otherwise,
           if the node to remove has a left child, replace the node to remove with the largest
           string value that is smaller than the current string to remove
           (i.e. find the largest value in the left subtree of the node to remove).
           If the node has no left child, replace the node to remove with the smallest value
           larger than the current string to remove
           (i.e. find the smallest value in the right subtree of the node to remove.
    */
   void remove(const string &key);

   /* Accessors */
   /* function: search
      parameters: const string
      description: searching for a string in the tree
         It should return true if the string is in the tree, and false otherwise.
         recursively loops through list.
            if key is less than current node, calls helper searchnode function to recursively look through left side of tree
            else, call helper function to recursively look through right side
    */
   bool search(const string &key) const;

   /*
   Find and return the largest value in the tree. Return an empty string if the tree is empty */
   string largest() const;
   /* Find and return the smallest value in the tree. Return an emtpy string if the tree is empty */
   string smallest() const;

   /* Compute and return the height of a particular string in the tree.
      The height of a leaf node is 0 (count the number of edges on the longest path).
      Return -1 if the string does not exist.
    */
   int height(const string &key) const;

   /* Printing */
   /* For all printing orders, each node should be displayed as follows:
      <string> (<count>)
      e.g. goodbye(1), Hello World(3)
      */
   void preOrder() const;
   void postOrder() const;
   void inOrder() const;

   private:
   Node *searchNode(const string &key, Node *input) const;

   void remove(Node *input, Node *parent);

   void preOrder(Node *input) const;
   void postOrder(Node *input) const;
   void inOrder(Node *input) const;

   int treeHeight(Node *input) const;
   void recursiveDelete(Node* toDelete);
};
#endif