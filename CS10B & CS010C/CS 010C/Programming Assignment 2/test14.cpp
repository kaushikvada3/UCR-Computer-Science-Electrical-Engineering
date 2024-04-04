#include <iostream>
#include <fstream>
#include "BSTree.h"

bool testPassed() {
   bool passed = true;
   
   cout << "Testing ctor, insert, remove, search, smallest, largest, height..." << endl;
   // redirect stdout to a file
   freopen("file.txt", "w", stdout);
   freopen("fileErr.txt", "w", stderr);
      
   BSTree tree;
   tree.insert("Alpha");
   tree.remove("Bravo");
   tree.remove("Alpha");
   tree.insert("Bravo");
   tree.insert("Charlie");
   bool searchAlpha = tree.search("Alpha");
   bool searchBravo = tree.search("Bravo");
   bool smallestError = tree.smallest() != "Bravo";
   bool largestError = tree.largest() != "Charlie";
   bool heightError = tree.height("Bravo") != 1 || tree.height("Charlie") != 0;
   tree.insert("Alpha");
   tree.insert("Charlie");
      
   //redirect stdout back to normal
   freopen("CON", "w", stdout);
   
   // grab anything that went to stdout
   ifstream t("file.txt");
   string str((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());      
   
   //check cout buffer
   if (!str.empty()) {
      cout << "Error: One of your functions output something it should not have output." << endl;
      passed = false;
   }
   
   t.close();

   if (searchAlpha) {
      cout << "Search returned true when it should have returned false" << endl;
      passed = false;
   }
   if (!searchBravo) {
      cout << "Search returned false when it should have returned true" << endl;
      passed = false;
   }
   if (smallestError) {
      cout << "smallest did not return the correct value" << endl;
      passed = false;
   }
   if (largestError) {
      cout << "largest did not return the correct value" << endl;
      passed = false;
   }
   if (heightError) {
      cout << "height did not return the correct value" << endl;
      passed = false;
   }

   cout << "Testing preOrder, inOrder, postOrder functions..." << endl;   
   // redirect stdout to a file
   freopen("file.txt", "w", stdout);
   
   tree.inOrder();
   tree.postOrder();
   tree.preOrder();
      
   //redirect stdout back to normal
   freopen("CON", "w", stdout);
   
   // grab anything that went to stdout
   t.open("file.txt");
   str = string((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());      
   string expected1 = "Alpha(1), Bravo(1), Charlie(2), \nAlpha(1), Charlie(2), Bravo(1), \nBravo(1), Alpha(1), Charlie(2), \n";
   string expected2 = "Alpha(1), Bravo(1), Charlie(2), Alpha(1), Charlie(2), Bravo(1), Bravo(1), Alpha(1), Charlie(2), ";
   if (str != expected1 && str != expected2) {
      cout << "\tError: At least one of the print order functions did not print the correct value." << endl;
      passed = false;
   }

   t.close();
   
   return passed;
}

int main() {
  if (testPassed()) {
    cout << "It worked" << endl;
    return 0;
  }
  cout << "It failed" << endl;
  return 1;
}
  
