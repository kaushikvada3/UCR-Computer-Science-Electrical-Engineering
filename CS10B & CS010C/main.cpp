#include <iostream>
using namespace std;

class IntNode
{
public:
    IntNode(int value = -1, IntNode *nextLoc = nullptr);
    void InsertAfter(IntNode *nodeLoc);
    int GetValue();
    IntNode *GetNext();
    void PrintData();

private:
    int value;
    IntNode *nextIntNodePtr;
};

IntNode::IntNode(int val, IntNode *nextLoc)
{
    this->value = val;
    this->nextIntNodePtr = nextLoc;
}

void IntNode::InsertAfter(IntNode *nodeLoc)
{
    IntNode *tmpNext = nullptr;

    tmpNext = this->nextIntNodePtr;
    this->nextIntNodePtr = nodeLoc;
    nodeLoc->nextIntNodePtr = tmpNext;
}

IntNode *IntNode::GetNext()
{
    return this->nextIntNodePtr;
}

void IntNode::PrintData()
{
    cout << this->value << endl;
}

int main()
{
    IntNode *headObj = nullptr;
    IntNode *node1 = nullptr;
    IntNode *node2 = nullptr;
    IntNode *node3 = nullptr;
    IntNode *node4 = nullptr;
    IntNode *currObj = nullptr;

    headObj = new IntNode(-1);

    node1 = new IntNode(1);
    headObj->InsertAfter(node1);

    node2 = new IntNode(2);
    headObj->InsertAfter(node2);

    node3 = new IntNode(3);
    headObj->InsertAfter(node3);

    node4 = new IntNode(4);
    node3->InsertAfter(node4);

    currObj = headObj;

    while (currObj != nullptr)
    {
        currObj->PrintData();
        currObj = currObj->GetNext();
    }

    return 0;
}
