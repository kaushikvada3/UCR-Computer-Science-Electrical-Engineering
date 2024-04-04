#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

struct Node {
    string payload;
    Node* next;
};

//create a new node and assign its values, and assign next as null
Node* newNode(string payload) 
{
    Node *newNode = new Node;
    newNode->payload = payload;
    newNode->next = nullptr;
    return newNode;
}

Node* loadGame(int n, vector<string> names) 
{
    Node* head = nullptr;
    Node* prev = nullptr;
    string name;

    for (int i = 0; i < n; ++i) {
        name = names.at(i);
        if (head == nullptr) {
            head = newNode(name); // initialize head specially
            prev = head; //this initialize prev as head as well

        } else {
            prev->next = newNode(name);
            prev = prev->next; //make sure prev moves on
        }
    }

    if (prev != nullptr) {
        prev->next = head; //make circular
    }
    return head;
}

void print(Node* start) 
{ 
    // prints list
    Node* curr = start;
    while (curr != nullptr) //you would not be able to dereference a null pointer and if it does end up being a nullptr, then it means that the the circle broke and it doesn't returns back to the head, which completes a circle.
    {
        cout << curr->payload << endl;
        curr = curr->next;
        if (curr == start) 
        {
            break; // exit circular list
        }
    }
}

Node* runGame(Node* start, int k) 
{ 
    // josephus w circular list, k = num skips
    Node* curr = start;
    Node* prev = curr;
    while (curr != curr->next) 
    { // exit condition, last person standing
        for (int i = 0; i < k; ++i) 
        { // find kth node
          prev = curr;
          curr = curr->next; //this makes sure that Curr keeps moving on in the for loop
        }

        prev->next = curr->next;
        
        // delete kth node
        delete curr;
        
        curr = prev->next; //reassign the curr with its next node, which fully removes the kth node from list
    }

    return curr; // last person standing
    delete curr;
}

/* Driver program to test above functions */
int main() 
{
    int n=1, k=1; // n = num names; k = num skips (minus 1)
    string name;
    vector<string> names;

    // get inputs
    cin >> n >> k;
    while (cin >> name && name != ".") { names.push_back(name); } // EOF or . ends input

    // initialize and run game
    Node* startPerson = loadGame(n, names);
    Node* lastPerson = runGame(startPerson, k);

    if (lastPerson != nullptr) {
        cout << lastPerson->payload << " wins!" << endl;
    } else {
        cout << "error: null game" << endl;
    }

    return 0;
}

