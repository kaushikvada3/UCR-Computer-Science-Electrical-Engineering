#include <iostream>
#include "Heap.h"
using namespace std;

// initialize numitems to 0 = there's nothing in the heap
Heap::Heap()
{
    numItems = 0;
}

// adds an item to the heap
void Heap::enqueue(PrintJob *input) {
  // If the heap is full, do not add the new element
  if (numItems == MAX_HEAP_SIZE) {
    return;
  }

  // Add the new element to the end of the array
  arr[numItems] = input;
  int nodeIndex = numItems; // Index of the current node being processed
  int parentIndex = 0; // Index of the parent node

  // Loop through the heap until the new element is placed in the correct position
  while (nodeIndex > 0) {
    parentIndex = (nodeIndex - 1) / 2; // Calculate the index of the parent node

    // If the new element has a higher priority than the parent node, do not swap
    if (arr[nodeIndex]->getPriority() <= arr[parentIndex]->getPriority()) {
      return;
    }

    // Otherwise, swap the parent and current node, and update the indexes for the next iteration
    else {
      PrintJob *swap = arr[nodeIndex];
      arr[nodeIndex] = arr[parentIndex];
      arr[parentIndex] = swap;

      nodeIndex = parentIndex;
    }
  }

  numItems++; // Increment the number of elements in the heap
}

// removes first item from heap (removes root)
void Heap::dequeue()
{
  if (numItems > 0)
  {

    // Shift all elements in the heap down one position
    for (int i = 0; i < numItems - 1; i++)
    {
      arr[i] = arr[i + 1];
    }

    // Decrement the number of elements in the heap
    numItems--;

    // Trickle down the new root of the heap
    trickleDown(0);

    // the temp object is not returned and can be destroyed or handled as needed here
    // possibly delete temp; if it needs to be de-allocated
  }
}

// returns highest priority
PrintJob *Heap::highest()
{
    if (numItems > 0) // check for empty heap
    {
        return arr[0]; // since heap is sorted, highest priority is the item at the top of the heap (root)
    }
    return nullptr;
}

// prints out highest priority in heap -- highest priority is root, so first value is printed
void Heap::print()
{
    if (numItems > 0) // check for empty heap
    {
        cout << "Priority: " << arr[0]->getPriority()
             << ", Job Number: " << arr[0]->getJobNumber()
             << ", Number of Pages: " << arr[0]->getPages() << endl;
    }
    return;
}

// helper function starts at nodeIndex to place it at the correct priority spot
void Heap::trickleDown(int nodeIndex) {
  // Original values
  int childIndex = 2 * nodeIndex + 1;
  PrintJob *currVal = arr[nodeIndex];

  while (childIndex < numItems) {
    // Find the index of the node with the highest value
    PrintJob *maxVal = currVal;
    int maxIndex = -1;
    for (int i = 0; i < 2 && i + childIndex < numItems; i++) {
      if (arr[i + childIndex] > maxVal) {
        maxVal = arr[i + childIndex];
        maxIndex = i + childIndex;
      }
    }

    // If the current node is already the maximum value, stop
    if (maxVal == currVal) {
      return;
    }

    // Otherwise, swap the current node and the maximum value
    else {
      PrintJob *swap = arr[nodeIndex];
      arr[nodeIndex] = arr[maxIndex];
      arr[maxIndex] = swap;

      // Update the current node and child index
      nodeIndex = maxIndex;
      childIndex = 2 * nodeIndex + 1;
    }
  }
}