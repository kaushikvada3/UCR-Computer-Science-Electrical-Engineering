// PROFPAT: by convention, we sort the include by name here
#include <exception>
#include <iostream>
#include <vector>

using namespace std;

// ZYBOOKS: You are given a function that creates a vector of
// ZYBOOKS: characters with random size. You just need to put the
// ZYBOOKS: following piece of code in your main.cpp file as is:
vector<char> createVector() {
  int vecSize = rand() % 26;
  char c = 'a';
  vector<char> vals;
  for(int i = 0; i < vecSize; i++)
    {
      vals.push_back(c);
      c++;
    }
  return vals;
}

// Passes in an index of type int and a vector of type T (T could be
// string, double, int or whatever). The function returns the index
// of the min value starting from "index" to the end of the "vector".
// PROFPAT: How do we define the "template" type T here?

template <typename T>
unsigned min_index(const vector<T> &vals, unsigned index) {
    //create a new variable that has its scope that only stays within the function
    //plus this variable serves as the starting point
    unsigned int minIndex = index;
    T minVal = vals.at(index);
    for(unsigned int i = index + 1; i < vals.size();i++) //loop through vector after the first element since the first element is minVal & minIndex
    {
       if(vals[i] < minVal)
       {
          minVal = vals[i]; //replace the value if the condition is met
          minIndex = i; //replace the index if the condition is met
       }
    }
    return minIndex; //after you iterate through the loop, then return the smallest index
}

// PROFPAT: Note that vals is passed by reference, but not const
// PROFPAT: reference.  So this function will change the vector
// PROFPAT: back in main when we call it
// Passes in a vector of type T and sorts them based on the selection
// sort method. This function should utilize the min_index function
// to find the index of the min value in the unsorted portion of the
// vector.
// PROFPAT: How do we define the "template" type T here?
template <typename T>
void selection_sort(vector<T> &vals) 
{
    unsigned int minIndex = 0; //assign the smallest index as the first index

    for(unsigned int i = 0; i < vals.size(); i++)
    {
      minIndex = min_index(vals,i); //use min_index function to iterate loop of selection sort
      if(minIndex != i)
      {
        swap(vals.at(minIndex), vals.at(i));
      }
    }
}

// PROFPAT: I updated the argument passing to const&.  The function
// PROFPAT: returns a copy -- not what I would have picked (I would
// PROFPAT: return a const T& here), but hey Zybooks will do it Zybooks
// PROFPAT: way :-)
// 
// Passes in a vector of type T (T could be string, double or int) and
// an index of type int. The function returns the element located at
// the index of the vals.
// PROFPAT: How do we define the "template" type T here?
template <typename T> //declare template
T getElement(const vector<T>& vals, int index) {
  // PROFPAT: This is a stub.  Just returns a default value of type T
  // PROFPAT: Should return the value or throw out_of_range.
  // PROFPAT: Question:  Should you use .at() or [] here?
  return vals.at(index); //return the element at the given index
}

// PROFPAT: Make this and then run with % ./a.out < test.txt
// ZYBOOKS: You should write up a try catch block in main function 
// ZYBOOKS: so that when the index is out of the range of the
// ZYBOOKS: vector, the "std::outofrange" exception will be caught 
// ZYBOOKS: by the catch(const std::outofrange& excpt).
// ZYBOOKS: Once the exception is caught, it
// ZYBOOKS: should output "out of range exception occured" followed by a
// ZYBOOKS: new line.
// ZYBOOKS: 
// ZYBOOKS: You should come up with test harnesses to test your
// ZYBOOKS: selection_sort function.
int main() {
  // Seeding the random number generator so we get a different
  // run every time.
  srand(time(0));

  // Fill in a vector of some random size with some random
  // characters
  vector<char> vals = createVector();
  int numOfRuns = 10;
  while(--numOfRuns >= 0){
    cout << "Enter a number: " << endl;
    // PROFPAT: Just because zybooks doesn't think it is important
    // PROFPAT: to check inputs doesn't mean you should skip it
    int index;
    if (not (cin >> index)) {
      throw runtime_error("could not read the index");
    }

    // PROFPAT: We will use a try/catch block here to "protect"
    // PROFPAT: the call to getElement() otherwise the program
    // PROFPAT: will terminate on the throw.  Remember!  The
    // PROFPAT: throw happens where we detect the issue.  The
    // PROFPAT: try/catch occurs where we want to handle it!!!!
    char curChar = getElement(vals,index);
    cout << "Element located at " << index << ": is " << curChar << endl;
  }

  // PROFPAT:  You should test selection sort here!!!!

    vector<int> intVals{3, 78595, 9, 78, 12, 0, 13, -15, 740};
      // Print the elements of the vector before sorting
      cout << "Before Sorting: ";
      for (int element : intVals) {
          cout << element << " ";
      }
      cout << endl;

      // Sort the vector
      selection_sort(intVals);

      // Print the elements of the vector after sorting
      cout << "After Sorting: ";
      for (int element : intVals) {
          cout << element << " ";
      }
      cout << endl << endl;

    vector<double> doubleVals{1, 1234.5678, 0, 3057, 9.4783, 5, 9, 0.314, -4.19, -9, 85493.8859490}; // vector of doubles-- with/out decimals, just decimal, single digit etc
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
      cout << endl << endl;

    vector<string> stringVals{"Kaushik", "microSoft", "Apple", " ", "ComputerScience", "Instagram", "is", "ComputerScience with PatMiller is fun", ""}; // vector of strings, including single letter, 2+ letters, lowercase and uppercase, and large string
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
    cout << endl<< endl;

  return 0;
}
