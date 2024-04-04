#include <iostream>
#include "Node.h"
using namespace std;

#ifndef AVLTREE_H
#define AVLTREE_H

class AVLTree
{
private:
   Node *root;

public:
   /* Constructors */
   /* Default constructor
         sets root to null */
   AVLTree();
   /* destructor
         calls helper function to delete all nodes */
   ~AVLTree();

   /* Mutators */
   /* Function: insert
         parameters: const string
         description: Insert an item into the AVL tree.
            call helper rotation functions to rebalance tree
    */
   void insert(const string &newString);

   /* Accessors */
   /* Function: balance factor
      parameters: Node
      description: return the balance factor of the input node
    */
   int balanceFactor(Node *currNode) const;

   /* Printing */
   /* Function: print with balance factor
      Description: Traverse and print the tree in inorder notation.
         Print the string followed by its balance factor in parentheses followed by a , and one space.
      */
   void printBalanceFactors() const;

   /* Function: output dotty file
      Description: Traverse and print the tree in inorder notation.
         Print the string followed by its balance factor in parentheses followed by a , and one space.
      */
   void visualizeTree(const string &outputFilename);

private:
   /* Function: rotate
      parameters: parent node
      Description: checks for inbalances and rotates by calling left or right rotate
      */
   void rotate(Node *parent);

   /* Function: rotate left
      parameters: parent node
      Description: rotates to the left given the unbalanced parent node
      */
   void rotateLeft(Node *rotParent);

   /* Function: rotate right
      parameters: parent node
      Description: rotates to the right given the unbalanced parent node
      */
   void rotateRight(Node *rotParent);

   /* Function: print with balance factor helper
      Description: Traverse and print the tree in inorder notation.
         Print the string followed by its balance factor in parentheses followed by a , and one space.
      */
   void printBalanceFactors(Node *subroot) const;

   /* Function: set child
      Parameters: parent node, string side, child node
      Description: assigns parents left or right (depending on the side) with the input child
      */
   void setChild(Node *parent, string side, Node *child);

   /* Function: replace child
      Parameters: parent node, current node, new node
      Description: replaces the current child with the new child by assigning parents left or right
      */
   void replaceChild(Node *parent, Node *currentChild, Node *newChild);

   /* Function: update input nodes height
      Parameters: node that needs height to be updated
      Description: get the height of both subtrees and assign node with the higher height (longest number of edges)
      */
   void updateHeight(Node *updateNode);

   /* Function: output dotty file helper
      parameters: output file name, starting node
      Description: Traverse and print the tree in inorder notation starting at the input node
         Print the string followed by its balance factor in parentheses followed by a , and one space.
      */
   void visualizeTree(ofstream &outFS, Node *n);

   /* Function: helper destructor function
      parameters: root node
      Description: delete all nodes using post order order
      */

   void deleteAllNodes(Node *toDelete);
};
#endif