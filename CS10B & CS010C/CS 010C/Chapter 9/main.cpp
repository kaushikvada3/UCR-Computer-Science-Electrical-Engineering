#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

// global variables to use in main
const int NUMBERS_SIZE = 50000;
const int CLOCKS_PER_MS = CLOCKS_PER_SEC / 1000; //clocks per milliseconds

// Need to declare the functions in front of the main function
// sorting functions
void Quicksort_midpoint(int numbers[], int i, int k);
void Quicksort_medianOfThree(int numbers[], int i, int k);
void InsertionSort(int numbers[], int numbersSize);

//these are just the different helper functions that I used for main functions 
// helper functions
int medianOfThree(int numbers[], int i, int k);
int genRandInt(int low, int high);
void fillArrays(int arr1[], int arr2[], int arr3[]); //helper function to fill arrays randomly

int main()
{
    // declare 3 random arrays to be sorted accordingly
    int midpointArr[NUMBERS_SIZE];
    int medianThreeArr[NUMBERS_SIZE];
    int insertionSortArr[NUMBERS_SIZE];

    fillArrays(midpointArr, medianThreeArr, insertionSortArr); // called helper function to fill arrays randomly

    clock_t Start = clock();
    Quicksort_midpoint(medianThreeArr, 0, NUMBERS_SIZE - 1); //testing the Quicksort_midpoint fucntion
    clock_t End = clock();
    int elapsedTime = (End - Start) / CLOCKS_PER_MS; // converts elapsed time from microseconds to milliseconds.
    cout << "Quicksort_midpoint elapsed time: " << elapsedTime << " milliseconds" << endl;

    Start = clock();
    Quicksort_medianOfThree(medianThreeArr, 0, NUMBERS_SIZE - 1);
    End = clock();
    elapsedTime = (End - Start) / CLOCKS_PER_MS; // converts elapsed time from microseconds to milliseconds.
    cout << "Quicksort_medianOfThree elapsed time: " << elapsedTime << " milliseconds" << endl;

    Start = clock();
    InsertionSort(insertionSortArr, NUMBERS_SIZE);
    End = clock();
    elapsedTime = (End - Start) / CLOCKS_PER_MS; // converts elapsed time from microseconds to milliseconds.
    cout << "InsertionSort elapsed time: " << elapsedTime << " milliseconds" << endl;

    return 0;
}

// given helper functions
int genRandInt(int low, int high)
{
    return low + rand() % (high - low + 1); //generating a random number between low and high
}
void fillArrays(int arr1[], int arr2[], int arr3[])
{
    for (int i = 0; i < NUMBERS_SIZE; ++i)
    {
        arr1[i] = genRandInt(0, NUMBERS_SIZE);
        arr2[i] = arr1[i]; //making copies for various random copies of the genRandInt array
        arr3[i] = arr1[i]; //making copies for various random copies of the genRandInt array
    }
}

// sorts the array through the quicksort method with a pivot that is the center of the array
void Quicksort_midpoint(int numbers[], int i, int k)
{
    if (i >= k) // in case the wrong bounds are inputed
    {
        return;
    }
    int j;
    int pivot = (i + k) / 2; // finds the center index
    int pVal = numbers[pivot]; //finds the pivot value
    swap(numbers[pivot], numbers[k]); // swaps the last and pivot value to make the pivot the last value
    int mid = i;
    for (j = i; j < k; ++j)
    {
        if (numbers[j] <= pVal) // moves values smaller than the pivot to the left partition, leaving the larger values to the right
        {
            swap(numbers[j], numbers[mid]);
            ++mid;
        }
    }
    swap(numbers[k], numbers[mid]); // finally, moves pivot value where it should be, then sort the left and right partitions recursively

    //calling it recursively
    Quicksort_midpoint(numbers, i, mid - 1);
    Quicksort_midpoint(numbers, mid + 1, k);
}

// helper function returns the median of three to find the correct pivot point  
int medianOfThree(int numbers[], int i, int k) {
    // Find the midpoint of the given range
    int midpoint = (i + k) / 2;

    // Check if the first element is greater than the midpoint, and if so, swap them
    if (numbers[i] > numbers[midpoint]) {
        swap(numbers[i], numbers[midpoint]);
    }

    // Check if the midpoint is greater than the last element, and if so, swap them
    if (numbers[midpoint] > numbers[k]) {
        swap(numbers[midpoint], numbers[k]);
    }

    // Check if the first element is greater than the midpoint, and if so, swap them again
    if (numbers[i] > numbers[midpoint]) {
        swap(numbers[i], numbers[midpoint]);
    }

    // Return the midpoint, which is now the median of the three elements
    return midpoint;
}

// sorts the array using quicksort but uses a different pivot point
void Quicksort_medianOfThree(int numbers[], int i, int k)
{
    if (i >= k) // in case the wrong values are inputed
    {
        return;
    }
    int j = i;
    int pIndex = medianOfThree(numbers, i, k); // find the pivot point and value by using the helper function
    int pivot = numbers[pIndex];
    swap(numbers[pIndex], numbers[k]); // swap to make the pivot the last value
    for (int l = i; l < k; ++l)        // use the same method as the previous quicksort but with the differing pivot point
    {
        if (numbers[l] <= pivot)
        {
            swap(numbers[j], numbers[l]);
            ++j;
        }
    }
    swap(numbers[k], numbers[j]);
    Quicksort_medianOfThree(numbers, i, j - 1);
    Quicksort_medianOfThree(numbers, j + 1, k);
}

// sorts the array using insertion sort
void InsertionSort(int numbers[], int numbersSize)
{
    for (int i = 1; i < numbersSize; ++i)
    {
        int j = i;
        while (j > 0 && numbers[j] < numbers[j - 1]) // loop through the array to compare to the i value and swap until teh lowest value is at i
        {
            swap(numbers[j], numbers[j - 1]);
            --j;
        } // then do that for each value, loop through and replace by smallest value, i++, repeat
    }
}