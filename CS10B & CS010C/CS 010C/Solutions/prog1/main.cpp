#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

struct Node
{
    string payload;
    Node *next;
};

Node *newNode(string payload)
{
    Node *retNode = new Node; // create a new node and assign its values, assign next as null
    retNode->payload = payload;
    retNode->next = nullptr;
    return retNode;
}

Node *loadGame(int n, vector<string> names)
{
    Node *head = nullptr;
    Node *prev = nullptr;
    string name;

    for (int i = 0; i < n; ++i)
    {
        name = names.at(i);
        if (head == nullptr)
        {
            head = newNode(name); // initialize head specially
            prev = head;          // initialize prev as head as well
            /** fill in this code **/
        }
        else
        {
            prev->next = newNode(name);
            prev = prev->next; // make sure prev moves on
            /** fill in this code **/
        }
    }

    if (prev != nullptr)
    {
        /** fill in this code **/ // make circular
        prev->next = head;
    }
    return head;
}

void print(Node *start)
{ // prints list
    Node *curr = start;
    while (curr != nullptr)
    {
        cout << curr->payload << endl;
        curr = curr->next;
        if (curr == start)
        {
            break; // exit circular list
        }
    }
}

Node *runGame(Node *start, int k)
{ // josephus w circular list, k = num skips
    Node *curr = start;
    Node *prev = curr;
    while (curr != curr->next)
    { // exit condition, last person standing
        for (int i = 0; i < k; ++i)
        { // find kth node
            prev = curr;
            curr = curr->next; // make sure curr moves on
            /** fill in this code
             **/
        }

        prev->next = curr->next;
        /** fill in this code **/ // delete kth node
        delete curr;              // curr stores kth node
        /** fill in this code **/
        curr = prev->next; // reassign curr with its next (fully removes kth node from list)
    }

    return curr; // last person standing
}

/* Driver program to test above functions */
int main()
{
    int n = 1, k = 1, max; // n = num names; k = num skips (minus 1)
    string name;
    vector<string> names;

    // get inputs
    cin >> n >> k;
    while (cin >> name && name != ".")
    {
        names.push_back(name);
    } // EOF or . ends input

    // initialize and run game
    Node *startPerson = loadGame(n, names);
    Node *lastPerson = runGame(startPerson, k);

    if (lastPerson != nullptr)
    {
        cout << lastPerson->payload << " wins!" << endl;
    }
    else
    {
        cout << "error: null game" << endl;
    }

    return 0;
}
