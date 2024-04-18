BSTContains(node, BSTTree){
    currNode = BSTTree->root;

    /*search the right left side*/
    while(currNode != nullptr)
    {
       if(node->key == currNode->key)
        {
            return true;
        }

        if(node> key < currNode->left)
        {
            currNode = currNode->left;
        }

        else if (node >key > currNode->right){
            currNode = currNode->right;
        }
        return false;
    }
}

BSTIsEmpty (node, BSTTree){
    // if(BSTTree->root == nullptr)
    // {
    //     return true;
    // }

    return BSTTree->root == nullptr;
    
}


BSTInsert (node, BSTTree)
{
    //if there is no tree existing, then make the first node (the root) the new inserted node
    if(BSTTree->root == nullptr)
    {
        BSTTree->root = node;
        return;
    }

    //creating a traversing variable
    currNode = BSTTree->root;
    //insert the new node into the left of the tree
    while (currNode != nullptr)
    {
        if(node->key < currNode->key)
        {
            if(currNode->left == nullptr)
            {
                currNode->left = node;
                return;
            }
            else
            {
                currNode = currNode->left;
            }
        }

        else if(node->key > currNode->key)
        {
            if(currNode->right == nullptr)
            {
                currNode->right = node;
                return;
            }
            else
            {
                currNode = currNode->right;
            }
        }
    }
}

// Helper function to traverse Tree A and insert common values into Tree C
    void intersectTrees(Node* nodeA, BST& treeB, BST& treeC) {
        if (nodeA == nullptr) {
            return;
        }
        // Check if current node's value is present in Tree B
        if (treeB.contains(treeB.root, nodeA->data)) {
            treeC.insert(treeC.root, nodeA->data); // Insert into Tree C
        }
        // Recur for left and right subtrees
        intersectTrees(nodeA->left, treeB, treeC);
        intersectTrees(nodeA->right, treeB, treeC);
    }


