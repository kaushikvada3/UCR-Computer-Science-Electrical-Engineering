/*Percolate Up*/

HeapPercolateUp(nodeIndex, heapArray, arraySize)
{
    //as long as nodeIndex is greater than zero
    while(nodeIndex > 0)
    {
        /*Find the parent*/
        parentIndex = (nodeIndex - 1)/2;
        if(heapArray[nodeIndex] <= heapArray[parentIndex]) //if the parent is greater than or equal to the nodeIndex, then keep it as it is
        {
            return;
        }
        /*if the nodeIndex end up being greater, than swap them and update the nodeIndex to be the parentIndex*/
        else
        {
            swap(heapArray[nodeIndex], heapArray[parentIndex]); //implement the swap helper function
            nodeIndex = parentIndex;
        }

    }
}




/*Percolate Down (don't need to know for the final)*/ 
/*

Process: 
1. find the child node and the value of the nodeIndex
2. then loop through the children to see if there are any children greater than the parent, and if so replace the maxVal and maxIndex
3. then swap the children so that way it trickles down to maintain the property of a maxHeap

*/

HeapPercolateDown(nodeIndex, heapArray, arraySize){ 
    //"nodeIndex" is that index of the node that violates the maxHeap property and needs to be moved down after removal
    childIndex = 2*nodeIndex +1; //this is the index of the left child node
    value = heapArray[nodeIndex]; //this is the value of the node that violates the maxHeap property

    while(childIndex < arraySize){
       /*set the temporary values to make sure the maxVal is value and 
       set the index to -1 until we find the index in the for loop*/
        maxVal = value; 
        maxIndex = -1;

        /*traverse to find the left and right child and see which one is greater 
        and replace the "maxVal" and "maxIndex" with that child*/
        for(int i = 0; i < 2; i++)
        {
            /*if the greater index has been found*/
            if(heapArray[i + childIndex] > maxVal)
            {
                maxVal = heapArray[i + childIndex];
                maxIndex = i + childIndex;
            }
        }
    }
    
    if(maxVal = value)
    {
        return;
    }
    //once you found the maxIndex, use the swap function to percolate down
    else{
        swap(heapArray[nodeIndex], heapArray[maxIndex]);
        index = maxIndex;
    }   

    

    
}