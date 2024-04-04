#include "BSTree.h"

#include <iostream>
#include <stdexcept>

using namespace std;

// This function inserts a new node with the given key into a binary search tree.
// The function starts by creating a new node with the given key and setting its count to 1.
// Then, it checks if the tree is empty (if so, it sets the root to the new node).
// Otherwise, it sets a pointer to the current node (curr) and starts a while loop.
// In the loop, it checks if the given key matches the current node's key.
// If so, it increments the count of the current node and returns.
// If not, it checks if the given key is less than the current node's key.
// If so, it sets the current node to the left child of the current node.
// If not, it checks if the given key is greater than the current node's key.
// If so, it sets the current node to the right child of the current node.
// This process is repeated until a leaf node is reached, at which point a new node is created with the given key and added as a leaf node.
void BSTree::insert(const string& key) {
  Node* insertNode = new Node(key, 1); // create a new node with the given key and count of 1

  if (root == nullptr) { // check if the tree is empty
    root = insertNode;
    return;
  }

  Node* curr = root; // set a pointer to the root
  while (true) {
    if (key == curr->key) { // check if the given key matches the current node's key
      curr->count++; // increment the count of the current node if it matches
      return;
    } else if (key < curr->key) { // check if the given key is less than the current node's key
      if (curr->left == nullptr) { // if the left child is null, set it to the new node
        curr->left = insertNode;
        return;
      }
      curr = curr->left; // set the current node to the left child
    } else { // check if the given key is greater than the current node's key
      if (curr->right == nullptr) { // if the right child is null, set it to the new node
        curr->right = insertNode;
        return;
      }
      curr = curr->right; // set the current node to the right child
    }
  }
}


// This function searches for a string in a binary search tree.
// It returns true if the string is found in the tree, and false otherwise.
bool search(string key) {
  // Set a pointer to the root of the tree
  Node* curr = root;

  // Traverse the tree in a depth-first manner until a match is found or until the end of the tree is reached
  while (curr != nullptr) {
    // Check if the current key matches the key being searched for
    if (key == curr->key) {
      // If so, return true to indicate that the key was found
      return true;
    } else if (key < curr->key) {
      // If the key being searched for is less than the current key, set the current node to the left child
      curr = curr->left;
    } else {
      // If the key being searched for is greater than the current key, set the current node to the right child
      curr = curr->right;
    }
  }
  // If no match is found, return false to indicate that the key was not found
  return false;
}

// This function finds and returns the largest value in a binary search tree.
// It returns an empty string if the tree is empty.
string largest() const {
  if (root == nullptr) { // check if the tree is empty
    return "";
  }

  Node* curr = root; // set a pointer to the root
  while (curr->right != nullptr) { // while the right child is not null
    curr = curr->right; // set the current node to the right child
  }
  return curr->key; // return the largest key
}

// This function finds and returns the smallest value in a binary search tree.
// It returns an empty string if the tree is empty.
string smallest() const {
  if (root == nullptr) { // check if the tree is empty
    return "";
  }

  Node* curr = root; // set a pointer to the root
  while (curr->left != nullptr) { // while the left child is not null
    curr = curr->left; // set the current node to the left child
  }
  return curr->key; // return the smallest key
}

// This function returns the height of the node with the given key in the tree.
// It returns -1 if the key is not found in the tree.
int BSTree::height(const string& key) const {
  // First search for the node with the given key
  Node* node = search(key);
  if (node == nullptr) {
    // If the node is not found, return -1 to indicate that the key was not found in the tree
    return -1;
  }

  // Then call the height_of function to get the height of the node
  return height_of(node);
}


void BSTree::preOrder() const {
  // This wants a ", " after each node and exactly one newline once done
  preOrder(root);
}

void BSTree::postOrder() const {
  postOrder(root);
}

void BSTree::inOrder() const {
  inOrder(root);
}

void BSTree::remove(const string& key) {
    remove_helper(nullptr, root, key);
}

// This function removes a node from a binary search tree.
// It starts by searching for the node with the given key (using the search function).
// If the node is found, it is decremented its count and checked to see if it has a count of 0.
// If it does, it is determined if the node has both left and right children (if so, it is replaced with 
// the largest node in the left subtree or the smallest node in the right subtree).
// If it does not have both left and right children, it is deleted and its parent is updated accordingly.
void BSTree::remove_helper(Node* parent, Node* curr, const string& key) {
    while (curr != nullptr) {
        if (key == curr->key) {
            curr->count--;
            if (curr->count == 0) {
                if (curr->left == nullptr && curr->right == nullptr) {
                    // Leaf node
                    if (parent == nullptr) {
                        // curr is root
                        delete curr;
                        root = nullptr;
                    } else if (parent->left == curr) {
                        delete curr;
                        parent->left = nullptr;
                    } else {
                        delete curr;
                        parent->right = nullptr;
                    }
                } else if (curr->left != nullptr) {
                    // Node has a left subtree, find the largest in the left subtree
                    Node* largest = curr->left;
                    Node* largestParent = curr;
                    while (largest->right != nullptr) {
                        largestParent = largest;
                        largest = largest->right;
                    }
                    curr->key = largest->key;
                    curr->count = largest->count;
                    largest->count = 1; // Set to 1 because we will remove this node next
                    remove_helper(curr, curr->left, largest->key);
                } else {
                    // Node has only a right child, find the smallest in the right subtree
                    Node* smallest = curr->right;
                    Node* smallestParent = curr;
                    while (smallest->left != nullptr) {
                        smallestParent = smallest;
                        smallest = smallest->left;
                    }
                    curr->key = smallest->key;
                    curr->count = smallest->count;
                    smallest->count = 1; // Set to 1 because we will remove this node next
                    remove_helper(curr, curr->right, smallest->key);
                }
                return; // Node found and handled, exit the function
            } else {
                // Count was decremented, but not to 0, nothing more to do
                return;
            }
        } else if (key < curr->key) {
            parent = curr;
            curr = curr->left;
        } else {
            parent = curr;
            curr = curr->right;
        }
    }
    // If we reach here, the key was not found in the tree, do nothing
}


int BSTree::height_of(Node *input) const
{
    if (input == nullptr) // base case
    {
        return -1;
    }
    int left = 0;
    int right = 0;
    // recursively call function on subtrees under input to find each height
    left = height_of(input->left);
    right = height_of(input->right);
    return 1 + max(left, right); // add one (for the input node) to whichever subtree is higher (since height is the max edges along the tree from input)
}

void BSTree::preOrder(Node* tree) const {
  // Base case: If the tree is empty, return
  if (tree == nullptr) {
    return;
  }

  // Print the key of the current node
  cout << tree->key << ", ";

  // Recursively call the preOrder function on the left and right subtrees
  preOrder(tree->left);
  preOrder(tree->right);
}

void BSTree::postOrder() const {
  // Base case: If the root node is null, return
  if (root == nullptr) {
    return;
  }

  // Recursively call the postOrder function on the left and right subtrees
  postOrder(root->left);
  postOrder(root->right);

  // After the recursive calls are complete, print the key of the root node
  cout << root->key << ", ";
}

void BSTree::inOrder(Node* tree) const {
  // Base case: If the tree is empty, return
  if (tree == nullptr) {
    return;
  }

  // Recursively call the inOrder function on the left and right subtrees
  inOrder(tree->left);

  // After the recursive calls are complete, print the key of the current node
  cout << tree->key << ", ";

  // Recursively call the inOrder function on the left and right subtrees
  inOrder(tree->right);
}