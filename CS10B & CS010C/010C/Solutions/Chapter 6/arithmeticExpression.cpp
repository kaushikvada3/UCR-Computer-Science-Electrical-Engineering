#include <iostream>
#include <stack>
#include <sstream>
#include <fstream>
#include "arithmeticExpression.h"
using namespace std;

arithmeticExpression::arithmeticExpression(const string &value) // initializes private variables
{
    root = nullptr;
    infixExpression = value;
}

void arithmeticExpression::infix() // calls helper function
{
    infix(root);
}

void arithmeticExpression::infix(TreeNode *input) // outputs tree in in order notation
{
    if (input == nullptr) // base case
    {
        return;
    }

    if (priority(input->data) != 0) // check for priority -- if there's priority, call recursively until leaf, and add parenthesis
    {
        cout << "(";
        infix(input->left);
        cout << input->data;
        infix(input->right);
        cout << ")";
    }
    else // otherwise, print recursively without parenthesis
    {
        infix(input->left);
        cout << input->data;
        infix(input->right);
    }
}

void arithmeticExpression::prefix() // calls helper function
{
    prefix(root);
}

void arithmeticExpression::prefix(TreeNode *input) // outputs tree in pre order notation
{
    if (input == nullptr) // base case
    {
        return;
    }
    // does not need to consider priority -- preorder means first root, then its subtrees recuesively
    cout << input->data;
    prefix(input->left);
    prefix(input->right);
}

void arithmeticExpression::postfix() // calls helper function
{
    postfix(root);
}

void arithmeticExpression::postfix(TreeNode *input) // outputs tree in post order recursively
{
    if (input == nullptr) // base case
    {
        return;
    }
    // does not need to consider priority -- postorder means root last, its subtrees recuesively first
    postfix(input->left);
    postfix(input->right);
    cout << input->data;
}

void arithmeticExpression::buildTree() // creates a tree with the given expression
{
    infixExpression = infix_to_postfix(); // calls helper function to create string expression to turn into a tree
    stack<TreeNode *> st;                 // create stack that will be used to keep track of whats being inserted

    for (unsigned i = 0; i < infixExpression.size(); ++i)
    {
        TreeNode *insert = new TreeNode(infixExpression.at(i), 'a' + i); // create node to be inserted
        if (priority(infixExpression.at(i)) == 0)                        // check priority -- if 0, then it's just a char that gets inserted as a leaf and will not be a subroot
        {
            st.push(insert);
        }
        else // if it's an operator, then operator will have 2 leaf nodes attached to it, which can be gotten from the stack
        {
            insert->right = st.top();
            st.pop();
            insert->left = st.top();
            st.pop();
            st.push(insert);
            root = insert;
        }
    }
}

// given function returns priority (0 for chars, 1-3 for operators)
int arithmeticExpression::priority(char op)
{
    int priority = 0;
    if (op == '(')
    {
        priority = 3;
    }
    else if (op == '*' || op == '/')
    {
        priority = 2;
    }
    else if (op == '+' || op == '-')
    {
        priority = 1;
    }
    return priority;
}

// given function that returns postfix version of infix expression
string arithmeticExpression::infix_to_postfix()
{
    stack<char> s;
    ostringstream oss;
    char c;
    for (unsigned i = 0; i < infixExpression.size(); ++i)
    {
        c = infixExpression.at(i);
        if (c == ' ')
        {
            continue;
        }
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')')
        { // c is an operator
            if (c == '(')
            {
                s.push(c);
            }
            else if (c == ')')
            {
                while (s.top() != '(')
                {
                    oss << s.top();
                    s.pop();
                }
                s.pop();
            }
            else
            {
                while (!s.empty() && priority(c) <= priority(s.top()))
                {
                    if (s.top() == '(')
                    {
                        break;
                    }
                    oss << s.top();
                    s.pop();
                }
                s.push(c);
            }
        }
        else
        { // c is an operand
            oss << c;
        }
    }
    while (!s.empty())
    {
        oss << s.top();
        s.pop();
    }
    return oss.str();
}

// prints initialize digraph function and calls helper function to print the actual contents
void arithmeticExpression::visualizeTree(const string &outputFilename)
{
    ofstream outFS(outputFilename.c_str());
    if (!outFS.is_open())
    {
        cout << "Error opening " << outputFilename << endl;
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

// helper function prints the contents of the dot file to visualize the tree recursively
void arithmeticExpression::visualizeTree(ofstream &out, TreeNode *output)
{
    if (output != nullptr) // base case 
    { 
        // first prints the actual nodes to be labeled and printed
        out << output->key << "[ label = "
            << "\"" << output->data << "\""
            << " ]" << endl;
        visualizeTree(out, output->left);
        visualizeTree(out, output->right);

        // then, if not null, prints the key and the edge that leads to the next key all the way left and all the way right
        if (output->left != nullptr)
        {
            out << output->key << " -> " << output->left->key << endl;
        }
        if (output->right != nullptr)
        {
            out << output->key << " -> " << output->right->key << endl;
        }
    }
}