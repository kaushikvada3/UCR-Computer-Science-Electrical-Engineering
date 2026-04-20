//include any standard libraries needed
#include <iostream>
#include <fstream>
using namespace std;

//  - Passes in an array along with the size of the array.
//  - Returns the mean of all values stored in the array.
double mean(const double array[], int arraySize);

//  - Passes in an array, the size of the array by reference, and the index of a value to be removed from the array.
//  - Removes the value at this index by shifting all of the values after this value up, keeping the same relative order of all values not removed.
//  - Reduces arraySize by 1.
void remove(double array[], int &arraySize, int index);


// - Passes in an array and the size of the array.
// - Outputs each value in the array separated by a comma and space, with no comma, space or newline at the end.
void display(const double array[], int arraySize);


const int ARR_CAP = 100;

int main(int argc, char *argv[]) {
   // verify file name provided on command line
   string filename;
   if(argc != 2){
      cout << "usage: program_name, input_file" << endl;
      exit(1);
   }
   // open file and verify it opened
   filename = argv[1];
   ifstream fin(filename);
   if(!fin.is_open()){
      cout << "Error opening " << filename << endl;
      return 1;
   }


   // Declare an array of doubles of size ARR_CAP.
   double arr[ARR_CAP];
    
   // Fill the array with up to ARR_CAP doubles from the file entered at the command line.
   int count = 0;
    while (count < ARR_CAP && fin >> arr[count]) {
        count++;
    }
    int currentSize = count;
    
   // Call the mean function passing it this array and output the value returned.
   cout << "Mean: " << mean(arr, currentSize) << endl;
   cout << endl;
    
   // Ask the user for the index (0 to size - 1) of the value they want to remove.
   int index;
   cout << "Enter index of value to be removed (0 to " << currentSize-1 << ") :" << endl;
   cin >> index;

   while (index < 0 || index >= currentSize) {
        cout << "Invalid index. Please enter a valid index: ";
        cin >> index;
    }

	
   // Call the display function to output the array.
   cout << endl;
   cout << "Before removing value: ";
   display(arr, currentSize);
   cout << endl;

   // Call the remove function to remove the value at the index provided by the user.
  remove(arr, currentSize, index);
    
   // Call the display function again to output the array with the value removed
   cout << endl;
   cout << "After removing value: ";
   display(arr, currentSize);


   // Call the mean function again to get the new mean
   cout << endl;
   cout << endl;
   cout << "Mean: " << mean(arr, currentSize) << endl;


   fin.close();
	return 0;
}

double mean(const double array[], int arraySize) {
    double sum = 0;
    for (int i = 0; i < arraySize; ++i) {
        sum += array[i];
    }
    return sum / arraySize;
}

void remove(double array[], int &arraySize, int index) {
    if (index >= 0 && index < arraySize) {
        for (int i = index; i < arraySize - 1; ++i) {
            array[i] = array[i + 1];
        }
        arraySize--; 
    }
}


void display(const double array[], int arraySize) {
    for (int i = 0; i < arraySize - 1; ++i) {
        cout << array[i] << ", ";
    }
    if (arraySize > 0) {
        cout << array[arraySize - 1];
    }
}