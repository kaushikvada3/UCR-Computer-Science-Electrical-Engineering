#include <iostream>
#include "BSTree.h"
using namespace std;

// constructor initializes root as nullptr
BSTree::BSTree()
{
    this->root = nullptr;
}

// traverses tree and deletes all nodes by calling helper function
BSTree::~BSTree() {
    recursiveDelete(root);
}

// recursively deletes tree in post order to make sure all nodes are removed
void BSTree::recursiveDelete(Node* toDelete) {
    if (toDelete != nullptr) {
        recursiveDelete(toDelete->getLeft());
        recursiveDelete(toDelete->getRight());
        delete toDelete;    
    }
    root = nullptr;
}

// inserts a node into tree
void BSTree::insert(const string &newString)
{
    Node *insertNode = new Node(newString);
    if (root == nullptr) // if tree is empty, insert new node as root
    {
        root = insertNode;
    }
    else
    {
        Node *compareNode = root; // find the location of the node to insert
        while (compareNode != nullptr)
        {
            if (newString == compareNode->getWord()) // case 1: node has a count more than one -- decrease count
            {/.
            0
                compareNode->setCount((compareNode->getCount()) + 1);
                break;
            }
            else if (newString < compareNode->getWord()) // key is less than comparison -- location moves to the left
            {
                if (compareNode->getLeft() == nullptr) // left is empty so node can now be inserted here
                {
                    compareNode->setLeft(insertNode);
                    break;
                }
                else
                {
                    compareNode = compareNode->getLeft();
                }
            }
            else // key is more than comparison, locations moves to the right
            {
                if (compareNode->getRight() == nullptr) // right is empty, so node can now be inserted here
                {
                    compareNode->setRight(insertNode);
                    break;
                }
                else0
                {
                    compareNode = compareNode->getRight();
                }
            }
        }
    }
}

// removes node with given key by calling helper recursive function
void BSTree::remove(const string &key)
{
    if (root == nullptr) // check if tree is empty -- nothing to remove
    {
        return;
    }
    if (root->getWord() == key && root->getLeft() == nullptr && root->getRight() == nullptr) // check if there's only a root node in the tree
    {
        root = nullptr;
        return;
    }
    Node *child = searchNode(key, root); // call helper function to find node at key
    if (child == nullptr) // node doesn't exist
    {
        return;
    }
    if (child->getCount() > 1) // if count is more than 1 for node to remove, just lower count
    {
        child->setCount((child->getCount()) - 1);
        return;
    }
    Node *parent = root;
    while (parent != nullptr) // find parent node for the node to remove
    {
        if (parent->getLeft() == child || parent->getRight() == child)
        {
            break;
        }
        else if (child->getWord() < parent->getWord())
        {
            parent = parent->getLeft();
        }
        else
        {
            parent = parent->getRight();
        }
    }
    remove(child, parent);
}

// helper function resursively removes input
void BSTree::remove(Node *input, Node *parent)
{
    if (input == nullptr) // case 0: input doesn't exist, nothing to remove
    {
        return;
    }
    if (input->getLeft() == nullptr && input->getRight() == nullptr) // case 1w: remove leaf node
    {
        if (parent->getLeft() == input) // set parent's left or right as null as well as input
        {
            parent->setLeft(nullptr);
            delete input;
        }
        else
        {
            parent->setRight(nullptr);
            delete input;
        }
    }
    else if (input->getLeft() != nullptr) // case 2: internal node with either only a left child or both left and right children
    {
        Node *par = input;            // parent is the one before succcessor (checked in the loop)
        Node *suc = input->getLeft(); // successor is left, then all the way right
        while (suc->getRight() != nullptr)
        {
            if (suc->getRight()->getRight() == nullptr)
            {
                par = suc;
            }
            suc = suc->getRight();
        }
        input->setWord(suc->getWord()); // change inputs values, then recursively remove successor with new parent
        input->setCount(suc->getCount());
        remove(suc, par);
    }
    else // case 3: internal node with only a right child : this case does the exact same thing as case 2, but the loop is slightly different
    {
        Node *par = input;
        Node *suc = input->getRight(); // successor is right, then all the way left
        while (suc->getLeft() != nullptr)
        {
            if (suc->getLeft()->getLeft() == nullptr)
            {
                par = suc;
            }
            suc = suc->getLeft();
        }
        input->setWord(suc->getWord());
        input->setCount(suc->getCount());
        remove(suc, par);
    }
}


// calls helper functions to recursively search and return if node exists in the tree
bool BSTree::search(const string &key) const
{
    Node *compare = searchNode(key, root);
    return (compare != nullptr); // searchNode function returns nullptr if node doesnt exist
}

// helper function recursively finds and returns node (or returns null if node doesnt exist)
Node *BSTree::searchNode(const string &key, Node *input) const
{
    if (input != nullptr)
    {
        if (input->getWord() == key) // case 1 (base): node is found and returned
        {
            return input;
        }
        else if (key < input->getWord()) // case 2: key is smaller than node, so nodes moves on to the left
        {
            return searchNode(key, input->getLeft());
        }
        else if (key > input->getWord()) // case 3: key is bigger than node, so nodes moves on to the right
        {
            return searchNode(key, input->getRight());
        }
    }
    return nullptr; // node was not found
}

// returns largest string in the tree
string BSTree::largest() const
{
    if (root == nullptr) // empty tree returns empty string
    {
        return "";
    }
    Node *compareNode = root;
    while (compareNode->getRight() != nullptr) // go all the way to the right to find the largest string because tree is in order
    {
        compareNode = compareNode->getRight();
    }
    return compareNode->getWord(); // return last string before hitting nullptr on the right
}

// returns smallest string in the tree
string BSTree::smallest() const // this function does the exact same thing as largest() but to the left of the tree
{
    if (root == nullptr)
    {
        return "";
    }
    Node *compareNode = root;
    while (compareNode->getLeft() != nullptr)
    {
        compareNode = compareNode->getLeft();
    }
    return compareNode->getWord();
}

// calls helper function to recursively return height of given key
int BSTree::height(const string &key) const
{
    if (search(key))
    {
        Node *currNode = searchNode(key, root); // find node at key
        return treeHeight(currNode);            // call helper function
    }
    return -1; // key doesn't exist
}

int BSTree::treeHeight(Node *input) const
{
    if (input == nullptr) // base case
    {
        return -1;
    }
    int left = 0;
    int right = 0;
    // recursively call function on subtrees under input to find each height
    left = treeHeight(input->getLeft());
    right = treeHeight(input->getRight());
    return 1 + max(left, right); // add one (for the input node) to whichever subtree is higher (since height is the max edges along the tree from input)
}

// calls helper function to print tree in preorder
void BSTree::preOrder() const
{
    preOrder(root);
}

// recursively prints tree as root, left subtree, then right subtree
void BSTree::preOrder(Node *input) const
{
    if (input == nullptr)
        return;
    cout << input->getWord() << "(" << input->getCount() << "), ";
    preOrder(input->getLeft());
    preOrder(input->getRight());
}

// calls helper function to print tree in postorder
void BSTree::postOrder() const
{
    postOrder(this->root);
}

// recursively prints tree as left subtree, right subtree, then root
void BSTree::postOrder(Node *input) const
{
    if (input == nullptr)
        return;
    postOrder(input->getLeft());
    postOrder(input->getRight());
    cout << input->getWord() << "(" << input->getCount() << "), ";
}

// calls helper function to print tree in inOrder
void BSTree::inOrder() const
{
    inOrder(this->root);
}

// recursively prints tree as left subtree, root, then right subtree
void BSTree::inOrder(Node *input) const
{
    if (input == nullptr)
        return;
    inOrder(input->getLeft());
    cout << input->getWord() << "(" << input->getCount() << "), ";
    inOrder(input->getRight());
}
