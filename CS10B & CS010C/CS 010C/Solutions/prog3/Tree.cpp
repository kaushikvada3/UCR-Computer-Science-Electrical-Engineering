#include <fstream>
#include <sstream>
#include <iostream>
#include "Tree.h"

using namespace std;

Tree::Tree() // initializes root
{
    root = nullptr;
}

Tree::~Tree() // calls helper function to traverse and delete all the nodes
{
    destroyTree(root);
    root = nullptr;
}

void Tree::destroyTree(Node *toDestroy) // helper function recursively goes to each node and deletes it from left to right
{
    // Base case: if the node to be destroyed is null, return
    if (toDestroy == nullptr)
    {
        return;
    }

    // Recursively destroy the left, middle, and right children of the node
    destroyTree(toDestroy->left);
    destroyTree(toDestroy->middle);
    destroyTree(toDestroy->right);

    // Delete the node itself
    delete toDestroy;
}

void Tree::insert(const string &add) // required insert function calls helper insert
{
    // Base case: if the root is null, create a new node and make it the root
    if (root == nullptr)
    {
        Node *insertNode = new Node(add);
        root = insertNode;
        return;
    }

    // Recursively call insert on the root node and the new word to be added
    insert(root, add);
}

void Tree::insert(Node *compareNode, const string &add) // helper function to insert
{
    if (compareNode->containsKey(add)) // no insert occurs if word already in tree
    {
        return;
    }
    if (!compareNode->isLeaf()) // traverse until theres a leaf node that would allow the word to legally exist (recursively)
    {
        if (add < compareNode->small)
        {
            insert(compareNode->left, add);
        }
        else if (compareNode->large == "" || add < compareNode->large)
        {
            insert(compareNode->middle, add);
        }
        else
        {
            insert(compareNode->right, add);
        }
    }
    else // calls helper to insert into a leaf
    {
        insertInLeaf(compareNode, add);
        return;
    }
}

void Tree::insertInLeaf(Node *insertNode, const string &add) // check for the 3 cases -- node with one element, full node with non full parent, full node iwth full parent
{
    if(insertNode->containsKey(add))
    {
        return;
    }
    if (!insertNode->isFull()) // node has space, calls helper function to insert it
    {
        insertNonFull(insertNode, add);
        return;
    }
    if (insertNode->isFull() && insertNode->parent == nullptr) // special root case for a full node
    {
        Node *leftNode = new Node(insertNode->small);
        Node *rightNode = new Node(insertNode->large);
        if (add < insertNode->large && add > insertNode->small) // new node is the new root
        {
            root = new Node(add);
            root->left = leftNode;
            root->middle = rightNode;
        }
        else if (add < insertNode->small) // middle + new root node is insert->small
        {
            leftNode->small = add;
            rightNode->small = insertNode->large;
            root = new Node(insertNode->small);
            root->left = leftNode;
            root->middle = rightNode;
        }
        else // middle + new root is insert->large
        {
            leftNode->small = insertNode->small;
            rightNode->small = add;
            root = new Node(insertNode->large);
            root->left = leftNode;
            root->middle = rightNode;
        }
        // assigning parent pointers to the new root
        root->left->parent = root;
        root->middle->parent = root;
        return;
    }

    if (insertNode->isFull() && !insertNode->parent->isFull()) // node is full and parent is not full (and not a root)
    {                                                          // calls helper function to insert with a non full parent
        insertNonFullParent(insertNode, add);
        return;
    }

    if (insertNode->isFull() && insertNode->parent->isFull()) // throw an exception for when both node and parent node are full (not being checked for this checkpoint)
    {
        throw std::runtime_error("Both node and parent node are full");
    }
}

    void Tree::insertNonFull(Node *insertNode, const string &add) // helper function for a node with space
{
    if (insertNode->small != "" && add < insertNode->small) // add is less than the current small or small is empty, need to be inserted to the left
    {
        insertNode->large = insertNode->small;
        insertNode->small = add;
    }
    else if (insertNode->large != "" && add > insertNode->large) // add is bigger than current large or large is empty, needs to be inserted to the right
    {
        insertNode->small = insertNode->large;
        insertNode->large = add;
    }
    else if (insertNode->small == "" && insertNode->large != "") // both values are empty
    {
        insertNode->small = add;
    }
    else // large value is empty
    {
        insertNode->large = add;
    }
}

void Tree::insertNonFullParent(Node *insertNode, const string &add) // helper function for when parent has space but node does not
{
    Node *leftNode = new Node(insertNode->small);
    Node *rightNode = new Node(insertNode->large);
    if (add < insertNode->small) // middle is small node
    {
        leftNode->small = add;
        insertNonFull(insertNode->parent, insertNode->small); // move the middle value into parent using insertnonfull function
        if (insertNode == insertNode->parent->left)           // split the remaining nodes to the corresponding places
        {
            insertNode->parent->left = leftNode;
            if (insertNode->parent->middle == nullptr)
            {
                insertNode->parent->middle = rightNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->middle, insertNode->large);
            }
        }
        else if (insertNode == insertNode->parent->middle)
        {
            insertNode->parent->middle = leftNode;
            if (insertNode->parent->right == nullptr)
            {
                insertNode->parent->right = rightNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->right, insertNode->large);
            }
        }
        else
        {
            insertNode->parent->right = rightNode;
            if (insertNode->parent->middle == nullptr)
            {
                insertNode->parent->middle = leftNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->middle, add);
            }
        }
    }

    else if (add > insertNode->large) // middle is large node
    {
        insertNonFull(insertNode->parent, insertNode->large); // move middle value into the parent node
        rightNode->small = add;
        if (insertNode == insertNode->parent->left) // split the remaining nodes
        {
            insertNode->parent->left = leftNode;
            if (insertNode->parent->middle == nullptr)
            {
                insertNode->parent->middle = rightNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->middle, insertNode->large);
            }
        }
        else if (insertNode == insertNode->parent->middle)
        {
            insertNode->parent->middle = leftNode;
            if (insertNode->parent->right == nullptr)
            {
                insertNode->parent->right = rightNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->right, insertNode->large);
            }
        }
        else
        {
            insertNode->parent->right = rightNode;
            if (insertNode->parent->middle == nullptr)
            {
                insertNode->parent->middle = leftNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->middle, insertNode->small);
            }
        }
    }
    else // new node is going to parent
    {
        insertNonFull(insertNode->parent, add);     // move middle value to parent node
        if (insertNode == insertNode->parent->left) // split the remaining 2 nodes
        {
            insertNode->parent->left = leftNode;
            if (insertNode->parent->middle == nullptr)
            {
                insertNode->parent->middle = rightNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->middle, insertNode->large);
            }
        }
        else if (insertNode == insertNode->parent->middle)
        {
            insertNode->parent->middle = leftNode;
            if (insertNode->parent->right == nullptr)
            {
                insertNode->parent->right = rightNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->right, insertNode->large);
            }
        }
        else
        {
            insertNode->parent->right = rightNode;
            if (insertNode->parent->middle == nullptr)
            {
                insertNode->parent->middle = leftNode;
            }
            else
            {
                insertInLeaf(insertNode->parent->middle, insertNode->small);
            }
        }
    }
}

void Tree::remove(const string &removeWord) // required remove function calls helper remove function
{
    remove(root, removeWord);
}

void Tree::remove(Node *compareNode, const string &removeWord)
{
    if (compareNode == nullptr) // node is not found -- nothing to remove
    {
        return;
    }
    if (compareNode->containsKey(removeWord)) // compare node contains the key
    {
        if (!compareNode->isFull()) // case when node is going to become nullptr
        {
            removeNonFull(compareNode, removeWord); // helper function
            return;
        }
        else if (!compareNode->isFull() && compareNode->parent->isFull()) // case when node is going to be null and the parent is full (combining nodes different case not handled for this program)
        {
            throw std::runtime_error("Parent Node is full.");
        }
        else if (compareNode->small == removeWord) // when node has 2 values, just set whichever value it is to be empty
        {
            compareNode->small = "";
        }
        else
        {
            compareNode->large = "";
        }
        return;
    }
    if (removeWord < compareNode->small) // traverse through tree to look for the node
    {
        remove(compareNode->left, removeWord);
    }
    else if (compareNode->large == "" || removeWord < compareNode->large)
    {
        remove(compareNode->middle, removeWord);
    }
    else
    {
        remove(compareNode->right, removeWord);
    }
}

void Tree::removeNonFull(Node *removeNode, const string &removeWord)
{
    if (removeNode == root && removeNode->isLeaf()) // root has the single key and nothing else
    {
        root->small = "";
        root = nullptr;
    }
    else if (removeNode == root && !removeNode->isLeaf()) // root has nodes that will be combined
    {
        if (removeNode->left != nullptr && !removeNode->left->isFull()) // root has a left with one key
        {
            root->small = removeNode->left->small; // move up key
            root->left = nullptr;
        }
        if (removeNode->middle != nullptr && !removeNode->middle->isFull()) // root has a middle with one key
        {
            if (root->small != removeWord) // left was moved up to replace the word
            {
                root->large = root->middle->small;
            }
            else // left was null
            {
                root->small = removeNode->middle->small;
            }
            root->middle = nullptr;
        }
        if (root->right != nullptr && !root->right->isFull()) // root has a right with one key
        {
            if (root->small != removeWord) // middle was moved up to replace the word
            {
                root->large = root->right->small;
            }
            else // middle was null
            {
                root->small = removeNode->right->small;
            }
            root->right = nullptr;
        }
    }
    else if (removeNode == removeNode->parent->left) // not a root node, just need to remove and then move up the other values to be combined
    {
        Node *parent = removeNode->parent;
        if (parent->middle != nullptr && !parent->isFull()) // middle has a value to move up and parent has space
        {
            insertNonFull(parent, parent->middle->small);
            removeNode = nullptr;
            parent->middle = nullptr;
        }
        else if (parent->right != nullptr && !parent->isFull()) // right has a value and parent still has space
        {
            insertNonFull(parent, parent->right->small);
            removeNode = nullptr;
            parent->right = nullptr;
        }
        parent->left = nullptr;
    }
    else if (removeNode == removeNode->parent->middle) // same thing for the next two else if and else statement, but in the case when the node is middle or right
    {
        Node *parent = removeNode->parent;
        if (parent->left != nullptr && !parent->isFull())
        {
            insertNonFull(parent, parent->left->small);
            removeNode = nullptr;
            parent->middle = nullptr;
            parent->left = nullptr;
        }
    }
    else
    {
        removeNode->parent->right = nullptr;
        removeNode = nullptr;
    }
}

void Tree::preOrder() const // required pre order function calls helper function
{
    preOrder(root);
}

void Tree::preOrder(Node *print) const // helper
{
    if (print == nullptr) // base case
    {
        return;
    }
    if (print->small != "") // first print the parent's small
    {
        cout << print->small << ", ";
    }
    preOrder(print->left);  // then print everything on the left node
    if (print->large != "") // then print parent's large
    {
        cout << print->large << ", ";
    }
    preOrder(print->middle); // then print everything else from middle to right
    preOrder(print->right);
}

void Tree::inOrder() const // required in order function calls helper function
{
    inOrder(root);
    
}

void Tree::inOrder(Node *print) const // helper function, prints everything alphabetically
{
    if (print == nullptr) // base case
        return;
    inOrder(print->left);   // first print everything on the left
    if (print->small != "") // then print parent's small
    {
        cout << print->small << ", ";
    }
    inOrder(print->middle); // then everything in the middle
    if (print->large != "") // then parent's large
    {
        cout << print->large << ", ";
    }
    inOrder(print->right); // then everything on the right
}

void Tree::postOrder() const // required post order function calls helper function
{
    postOrder(root);
}

void Tree::postOrder(Node *print) const // helper function
{
    if (print == nullptr) // base case
        return;
    postOrder(print->left); // first print everything on the left and middle
    postOrder(print->middle);
    if (print->small != "") // then print parent's small
    {
        cout << print->small << ", ";
    }
    postOrder(print->right); // then print everything on the right
    if (print->large != "") // then parent's large
    {
        cout << print->large << ", ";
    }
}

bool Tree::search(const string &input) const // search function calls helper search
{
    return search(root, input);
}

bool Tree::search(Node *compareNode, const string &input) const // helper search
{
    if (compareNode == nullptr) // base case: key was not found
    {
        return false;
    }
    if (compareNode->containsKey(input)) // base case: if key is found, return true
    {
        return true;
    }
    else if (input < compareNode->small) // traverse through tree and recursively call it based on where input could go until it hits one of the base cases
    {
        return search(compareNode->left, input);
    }
    else if (input > compareNode->large)
    {
        return search(compareNode->right, input);
    }
    else
    {
        return search(compareNode->middle, input);
    }
}