#include <iostream>
#include <vector>
using namespace std;


int DuplicateArray(vector<int> vector) {
  int duplicate = 0;
  for (int i = 0; i < vector.size(); i++) {
    for (int j = i + 1; j < vector.size(); j++) {
      if (vector[i] == vector[j]) {
        duplicate = vector[j];
        break;
      }
    }
    break;
  }
  if (duplicate == 0) {
    return -1;
  } else {
    return duplicate;
  }
}


int main() {
    // Test the DuplicateArray function
    vector<int> testVector = {4, 5, 6, 7, 8, 8, 9}; // Change this vector to test different scenarios
    int result = DuplicateArray(testVector);
    if (result == -1) {
        cout << "No duplicates found." << endl;
    } else {
        cout << "First duplicate found: " << result << endl;
    }
    return 0;
}

