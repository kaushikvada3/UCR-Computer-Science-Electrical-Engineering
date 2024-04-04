#include "BSTree.h"

#include <iostream>
#include <stdexcept>

using namespace std;

void BSTree::insert(const string& key) {
  // Edge case:  The tree is empty
  if (root == nullptr) {
    root = new Node(key, 1);
    return;
  }

  // Edge case: If we find the key in the tree, just increment its count
  Node* curr = root;
  while (curr != nullptr) {
    if (curr->key == key) {
      curr->count++;
      return;
    } else if (curr->key > key) {
      if (curr->left == nullptr) {
        curr->left = new Node(key, 1);
        return;
      } else {
        curr = curr->left;
      }
    } else {
      if (curr->right == nullptr) {
        curr->right = new Node(key, 1);
        return;
      } else {
        curr = curr->right;
      }
    }
  }
}
bool BSTree::search(const string& key) const {
  // Search can be done in a loop (or recursively).  A loop is best here
  Node* curr = root;
  while (curr != nullptr) {
    if (curr->key == key) {
      return true;
    } else if (curr->key > key) {
      curr = curr->left;
    } else {
      curr = curr->right;
    }
  }
  return false;
}

string BSTree::largest() const {
  // Edge case: Tree is empty (return "")
  if (root == nullptr) {
    return "";
  }

  // Largest can be done in a loop (or recursively).  A loop is best here
  Node* curr = root;
  while (curr->right != nullptr) {
    curr = curr->right;
  }
  return curr->key;
}

string BSTree::smallest() const {
  // Smallest can be done in a loop (or recursively).  A loop is best here
  // Edge case: Tree is empty
  if (root == nullptr) {
    return "";
  }

  // Typical case.  Find the smallest key in the left subtree
  Node* curr = root;
  while (curr->left != nullptr) {
    curr = curr->left;
  }
  return curr->key;
}

int BSTree::height(const string& key) const {
  // First find the node with this key, then run "height_of" to get
  // the height that zybooks wants
  Node* node = search(key);
  if (node == nullptr) {
    return -1;
  }
  return height_of(node);
}

void BSTree::remove(const string& key) {
  Node* parent = nullptr;
  Node* tree = root;
  while (tree != nullptr) {
    parent = tree;
    if (tree->key == key) {
      break;
    } else if (tree->key > key) {
      tree = tree->left;
    } else {
      tree = tree->right;
    }
  }

  if (tree == nullptr) {
    return;
  }

  // Edge case: The key is not found (do nothing)
  if (tree->count == 1) {
    if (parent == nullptr) {
      // Edge case 1: We are removing the last node from the root
      root = nullptr;
    } else if (parent->left == tree) {
      parent->left = nullptr;
    } else {
      parent->right = nullptr;
    }
    delete tree;
    return;
  }

  // Edge case.  The key count is greater than 1.  Just decrement the count
  tree->count--;

  // Edge case: leaf (no children).  Just remove from parent
  if (tree->left == nullptr && tree->right == nullptr) {
    if (parent == nullptr) {
      // Edge case 2: curr is the left child, remove it from parent
      root = tree;
    } else if (parent->left == tree) {
      parent->left = tree;
    } else {
      parent->right = tree;
    }
    delete tree;
    return;
  }

  // Typical case.  Find the target
  // It is either the largest key in the left tree (if one exists)
  // or the smallest key in the right tree (since not a leaf one will exist)
  // Copy the target information into the node we found, set the target count to
  // one, and recursively remove it from left or right subtree (current node is the parent)
  Node* target;
  if (tree->left != nullptr) {
    target = tree->left;
    while (target->right != nullptr) {
      parent = target;
      target = target->right;
    }
    tree->key = target->key;
    tree->count = target->count;
    if (parent == nullptr) {
      root = target;
    } else if (parent->left == tree) {
      parent->left = target;
    } else {
      parent->right = target;
    }
    delete target;
  } else {
    target = tree->right;
    parent = tree;
    while (parent != nullptr) {
      if (parent->right == target) {
        break;
      }
      parent = parent->right;
    }
    if (parent == nullptr) {
      root = target;
    } else {
      parent->right = target;
    }
    delete tree;
  }
}

void BSTree::preOrder() const {
  // This wants a ", " after each node and exactly one newline once done
  throw runtime_error("not done preOrder");
}

void BSTree::postOrder() const {
  // This wants a ", " after each node and exactly one newline once done
  throw runtime_error("not done postOrder");
}

void BSTree::inOrder() const {
  // This wants a ", " after each node and exactly one newline once done
  throw runtime_error("not done inOrder");
}

// void BSTree::remove(Node* parent, Node* tree, const string& key) {
//   // Hint: A good approach is to find the parent and the curr node that holds that key
//   // Edge case: The key is not found (do nothing)
//   // Edge case.  The key count is greater than 1.  Just decrement the count
//   // Edge case: leaf (no children).  Just remove from parent
//   //  ==> case 1: parent is nullptr (we are removing the last node from root)
//   //  ==> case 2: curr is the left child, remove it from parent
//   //  ==> case 3: curr is the right child, remove it from parent
//   // Typical case.  Find the target
//   // It is either the largest key in the left tree (if one exists)
//   // or the smallest key in the right tree (since not a leaf one will exist)
//   // Copy the target information into the node we found, set the target count to
//   // one, and recursively remove it from left or right subtree (current node is the parent)
//   throw runtime_error("not done remove");
// }

int BSTree::height_of(Node* tree) const {
  // The height (length of longest path to the bottom) of an empty tree is -1
  // Otherwise, you pick the larger of the left height and the right height
  // and add one to that
  throw runtime_error("not done height_of");
}

void BSTree::preOrder(Node* tree) const {
  // print key, do left, do right
  throw runtime_error("not done preOrder");
}

void BSTree::postOrder(Node* tree) const {
  // do left, do right, print key
  throw runtime_error("not done postOrder");
}

void BSTree::inOrder(Node* tree) const {
  // do left, print key, do right
  throw runtime_error("not done inOrder");
}

// This is a pre-order traversal that shows the full state of the tree
// (but it is a code turd)
void BSTree::debug(Node* tree, int indent) const {
  if (tree == nullptr) return;
  for(int i=0;i<4*indent;++i) cout << ' ';
  cout << tree << ' ' << *tree << endl;
  debug(tree->left,indent+1);
  for(int i=0;i<4*indent;++i) cout << ' ';
  cout << "-" << endl;
  debug(tree->right,indent+1);
}
