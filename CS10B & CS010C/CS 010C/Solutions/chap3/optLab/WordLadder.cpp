#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <fstream>
#include <list>
#include <stack>
#include <queue>
#include "WordLadder.h"

using namespace std;

WordLadder::WordLadder(const string &inputDict)
{
    ifstream inFS(inputDict);
    string word;
    int index = 0;
    if (!inFS.is_open())
    {
        cout << "Error: Unable to open " << inputDict << endl;
        return;
    }
    while (inFS >> word)
    {
        if (word.length() != 5)
        {
            cout << "Dictionary contains words that are not 5 letters long." << endl;
            return;
        }
        dict.push_back(word);
        ++index;
    }
    inFS.close();
}

void WordLadder::outputLadder(const string &start, const string &end, const string &outputFile)
{
    if (!inDict(start, dict) || !inDict(end, dict))
    {
        cout << "Error: one or more inputs do not exist in this list" << endl;
        return;
    }

    ofstream oFS(outputFile);
    if (!oFS.is_open())
    {
        cout << outputFile << "file is unable to open" << endl;
        return;
    }

    stack<string> firstStack;
    firstStack.push(start);
    queue<stack<string>> q;
    q.push(firstStack);

    while (!q.empty())
    {
        stack<string> currStack = q.front();
        string currword = currStack.top();
        q.pop();

        for (string word : dict)
        {
            if (offByOne(currword, word))
            {
                stack<string> outStack = currStack;
                outStack.push(word);
                cout << "off by one before the end" << endl;
                if (word == end)
                {
                    while (!outStack.empty())
                    {
                        cout << "is this it" << endl;
                        oFS << outStack.top() << endl;
                        outStack.pop();
                    }
                    oFS.close();
                    return;
                }
                else
                {
                    q.push(outStack);
                }
            }
        }
    }

    cout << "No word ladder found" << endl;
}

bool WordLadder::inDict(string word, list<string> inputDict)
{
    bool inDict = false;
    for (string s : inputDict)
    {
        if (word == s)
        {
            inDict = true;
            break;
        }
    }
    return inDict;
}

bool WordLadder::offByOne(string &word1, string &word2)
{
    int diffChars = 0;
    for (unsigned int i = 0; i < word1.length(); ++i)
    {
        if (word1[i] != word2[i])
        {
            ++diffChars;
        }
    }
    return diffChars == 1;
}

