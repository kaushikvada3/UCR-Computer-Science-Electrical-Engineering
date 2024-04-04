#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

using  std::cout;
using std::cin;
using std::string;
using std::endl;
using std::vector;

template <typename T> // declare template
unsigned min_index(const vector<T> &vals, unsigned index)
{
    unsigned int minIndex = index;                         // assign minIndex with the input index -- starting point
    T minVal = vals.at(index);                             // same with min value
    for (unsigned int i = index + 1; i < vals.size(); ++i) // loop through vector starting one after index because minIndex and val have the first one
    {
        if (vals[i] < minVal) // compare value to one stores in minVal
        {
            minVal = vals[i]; // if true, replace minVal with the newest smallest val
            minIndex = i;     // same with index
        }
    }
    return minIndex; // after loop iterates, return the smallest index
}

template <typename T> // declare template
void selection_sort(vector<T> &vals)
{
    unsigned int minIndex = 0; // assign smallest index as first value

    for (unsigned int i = 0; i < vals.size(); ++i) // outer loop of selection sort
    {
        minIndex = min_index(vals, i); // use min_index function to iterate inner loop of selection sort
        if (minIndex != i)
        { // check to see if it found a different smallest value, then swap
            std::swap(vals.at(minIndex), vals.at(i));
        }
    }
}

template <typename T> // declare template
T getElement(vector<T> vals, int index)
{
    return vals.at(index); // return element at given index
}

vector<char> createVector() // given function
{
    int vecSize = rand() % 26;
    char c = 'a';
    vector<char> vals;
    for (int i = 0; i < vecSize; i++)
    {
        vals.push_back(c);
        c++;
    }
    return vals;
}

int main()
{
    // test harness for selection sort
    vector<int> intVals{1, 1267, 4, 2, 5, 0, 3, -4, 100}; // vector of ints, including negatives, 0, and large numbers
    for (int x : intVals)
    {
        cout << x << " "; // print before selection_sort
    }
    cout << endl;
    selection_sort(intVals); // call function
    for (int x : intVals)
    {
        cout << x << " "; // print after selection_sort
    }
    cout << endl
         << endl;

    vector<double> doubleVals{1, 1234.41378, 0, 1267, 4.864, 2, 5, 0.3, -3, -4, 2.45}; // vector of doubles-- with/out decimals, just decimal, single digit etc
    for (double x : doubleVals)
    {
        cout << x << " "; // print before selection_sort
    }
    cout << endl;
    selection_sort(doubleVals); // call function
    for (double x : doubleVals)
    {
        cout << x << " "; // print after selection_sort
    }
    cout << endl
         << endl;

    vector<string> stringVals{"Apple", "apple", "Plane", "Z", "String", "Order", "is", "a verylongstring with some spaces", ""}; // vector of strings, including single letter, 2+ letters, lowercase and uppercase, and large string
    // also tested one with an empty "" and " " seperately
    for (string x : stringVals)
    {
        cout << x << " "; // print before selection_sort
    }
    cout << endl;
    selection_sort(stringVals); // call function
    for (string x : stringVals)
    {
        cout << x << " "; // print after selection_sort
    }
    cout << endl
         << endl;

    // Part B
    srand(time(0));
    vector<char> vals = createVector();
    char curChar;
    int index;
    int numOfRuns = 10;

    while (--numOfRuns >= 0)
    {
        cout << "Enter a number: " << endl;
        cin >> index;
        try
        {
            curChar = getElement(vals, index);
            cout << "Element located at " << index << ": is " << curChar << endl;
        }
        catch (const std::out_of_range &excpt)
        {
            cout << "out of range exception occured" << endl;
        }
    }

    return 0;
}
