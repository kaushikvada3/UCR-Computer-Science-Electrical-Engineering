#include <iostream>
#include "AVLTree.h"
#include <fstream>
#include <sstream>
using namespace std;

// general function descriptions in AVLTree.h file

AVLTree::AVLTree()
{
    root = nullptr;
}

AVLTree::~AVLTree(){
    deleteAllNodes(root);
    root = nullptr;
}

void AVLTree::deleteAllNodes(Node *toDelete){
    if(toDelete != nullptr){
        deleteAllNodes(toDelete->left);
        deleteAllNodes(toDelete->right);
        delete toDelete;
    }
}

void AVLTree::updateHeight(Node *updateNode)
{
    // initialize both heights as -1 because null node has -1 height
    int leftHeight = -1;
    if (updateNode->left != nullptr)
    { // get the height of the left and right subtrees
        leftHeight = updateNode->left->height;
    }
    int rightHeight = -1;
    if (updateNode->right != nullptr)
    {
        rightHeight = updateNode->right->height;
    }
    // assign height by adding one (account for the current node) to the higher number of edges
    updateNode->height = max(leftHeight, rightHeight) + 1;
}

int AVLTree::balanceFactor(Node *currNode) const
{
    // initialize both heights as -1 because null node has -1 height
    int leftHeight = -1;
    int rightHeight = -1;
    // get left and right heights
    if (currNode->left != nullptr){
        leftHeight = currNode->left->height;
    }
    if (currNode->right != nullptr){
        rightHeight = currNode->right->height;
    }
    return leftHeight - rightHeight; // return balance factor
}

void AVLTree::setChild(Node *parent, string side, Node *child)
{
    // assign parents left or right (based on side) as the new child
    if (side == "left")
    {
        parent->left = child;
    }
    else
    {
        parent->right = child;
    }
    if (child != nullptr)
    { // check if child is not null, update child's parent
        child->parent = parent;
    }
    updateHeight(parent); // update height of parent because of new child
}

void AVLTree::replaceChild(Node *parent, Node *currentChild, Node *newChild)
{
    // check for which side current is in, then call set child to replace current child with new child
    if (parent->left == currentChild)
    {
        setChild(parent, "left", newChild);
    }
    else if (parent->right == currentChild)
    {
        setChild(parent, "right", newChild);
    }
}

void AVLTree::rotateRight(Node *rotParent)
{
    Node *leftRightChild = rotParent->left->right; // get the grandchild of the unbalanced node
    if (rotParent->parent != nullptr)
    {
        replaceChild(rotParent->parent, rotParent, rotParent->left); // change the input parent to become its left
    }
    else // root case
    {
        root = rotParent->left;
        root->parent = nullptr;
    }
    setChild(rotParent->left, "right", rotParent); // change the left of the new parent to its right
    setChild(rotParent, "left", leftRightChild);   // THEN change the grandchild (to make sure the left is not lost)
}


void AVLTree::rotateLeft(Node *rotParent) {
    // Get the right child of the parent node
    Node *rightLeftChild = rotParent->right->left;

    // Check if the parent node has a parent node
    if (rotParent->parent != nullptr) {
        // Replace the parent node with its right child
        replaceChild(rotParent->parent, rotParent, rotParent->right);
    } else {
        // If the parent node is the root, set the root to the right child
        root = rotParent->right;
        root->parent = nullptr;
    }

    // Set the right child as the left child of the parent node
    setChild(rotParent->right, "left", rotParent);

    // Set the parent node as the right child of the right child
    setChild(rotParent, "right", rightLeftChild);
}


void AVLTree::rotate(Node *parent) {
    // Update the height of the parent node
    updateHeight(parent);

    // Determine the balance factor of the parent node
    int balanceFactor = balanceFactor(parent);

    // If the balance factor is -2 or 2, perform a left or right rotation, respectively
    if (balanceFactor == -2 || balanceFactor == 2) {
        if (balanceFactor(parent->right) == 1) {
            // Perform a double rotation, which involves a left rotation followed by a right rotation
            rotateRight(parent->right);
            rotateLeft(parent);
        } else if (balanceFactor(parent->right) == -1) {
            rotateLeft(parent);
        } else {
            rotateRight(parent);
        }
    }

    // Continue iterating through the tree, calling rotate on each node until the entire tree is rebalanced
    while (parent != nullptr) {
        rotate(parent);
        parent = parent->parent;
    }
}

void AVLTree::insert(const string &newString)
{
    Node *insertnode = new Node(newString);
    if (root == nullptr)
    { // if tree is empty, insert new node as root
        root = insertnode;
    }
    else
    {
        Node *compareNode = root; // find the location of the node to insert
        while (compareNode != nullptr)
        {
            if (newString < compareNode->word) // key is less than comparison -- location moves to the left
            {
                if (compareNode->left == nullptr) // left is empty so node can now be inserted here
                {
                    compareNode->left = insertnode;
                    compareNode->left->parent = compareNode;
                    break;
                }
                else
                    compareNode = compareNode->left;
            }
            else if (newString > compareNode->word) // key is more than comparison, locations moves to the right
            {
                if (compareNode->right == nullptr) // right is empty, so node can now be inserted here
                {
                    compareNode->right = insertnode;
                    insertnode->parent = compareNode;
                    break;
                }
                else
                {
                    compareNode = compareNode->right;
                }
            }
            else if (newString == compareNode->word)
            { // node already exists in tree so nothing is added (AVL tree properties)
                return;
            }
        }
    }

    insertnode = insertnode->parent; // assign to parent to traverse through and rebalance tree
    while (insertnode != nullptr)
    {
        rotate(insertnode); // use rotate function to rebalance each node in the entire tree
        insertnode = insertnode->parent;
        // cout << insertnode->word << endl;
    }
}

void AVLTree::printBalanceFactors() const
{
    printBalanceFactors(root); // call helper funtion
}
void AVLTree::printBalanceFactors(Node *subroot) const
{
    if (subroot == nullptr) // base case for recursion
        return;
    // print tree in in-order notation
    printBalanceFactors(subroot->left);
    cout << subroot->word << "(" << balanceFactor(subroot) << "), ";
    printBalanceFactors(subroot->right);
}

void AVLTree::visualizeTree(const string &outputFilename) // given function, call helper function
{
    ofstream outFS(outputFilename.c_str());
    if (!outFS.is_open())
    {
        cout << "Error" << endl;
        return;
    }
    outFS << "digraph G {" << endl;
    visualizeTree(outFS, root);
    outFS << "}";
    outFS.close();
    string jpgFilename = outputFilename.substr(0, outputFilename.size() - 4) + ".jpg";
    string command = "dot -Tjpg " + outputFilename + " -o " + jpgFilename;
    system(command.c_str());
}

void AVLTree::visualizeTree(ofstream &outFS, Node *n) // given function, helper function outputsdotty file
{
    if (n)
    {
        if (n->left)
        {
            visualizeTree(outFS, n->left);
            outFS << n->word << " -> " << n->left->word << ";" << endl;
        }

        if (n->right)
        {
            visualizeTree(outFS, n->right);
            outFS << n->word << " -> " << n->right->word << ";" << endl;
        }
    }
}