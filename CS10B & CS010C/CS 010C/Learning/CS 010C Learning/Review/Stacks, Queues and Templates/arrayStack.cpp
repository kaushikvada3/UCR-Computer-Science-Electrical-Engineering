/* An array-based stack implementation can support both bounded and unbounded stack operations by using maxLength as follows:

If maxLength is negative, the stack is unbounded.
If maxLength is nonnegative, the stack is bounded.

The push operation does not allow a push when length == maxLength. The stack's length is always nonnegative, so:

If maxLength is negative, the condition is never true. So all push operations are allowed, and therefore the stack is unbounded.
If maxLength is nonnegative, the condition is true when the stack is full. So push operations are not allowed when full, and therefore the stack is bounded. */

ArrayStackPush(stack, item) {
   /* if the length of the stack is equal to the maxLength of the stack, then it cannot push */
   if (stack⇢length == stack⇢maxLength)  {
      return false
   }

   /*
   once the length of the stack is EQUAL to the allocationSize of the stack, then you have 
   to make sure to resize the stack to make sure that the push function can accomodate the addition 
   of more items 
   */

   if (stack⇢length == stack⇢allocationSize) { //the length of the stack is the controlling factor, but the maxLength is the hardstop
      ArrayStackResize(stack)
  }

   // Push the item and return true
   stack⇢array[stack⇢length] = item //add the item to the stack. Starts with the first one being of length = 0, and then leaving space for another item to be added.
   stack⇢length = stack⇢length + 1 //increases the length of the stack to accomodate the addition of more items
   return true
}


ArraySizeResize(stack){
// to resize the array, we need to create a new variable that increases the allocationSize (standard is DOUBLE)
newAllocSize = stack->allocationSize *2;

if(stack->maxLength > 0)
{
    //need to check to see which is smaller becuase the newAllocSize cannot exceed the maxLength, so it 
    //either has to be the maxLength or the newAllocSize.
    newAllocSize = min(newAllocsize, stack->maxLength);
}

newAllocArray = new int[newAllocSize]; //create a new array with the newAllocSize
//copies over the old array into the new arry
for(int i = 0; i < stack->length; i++)
{
    newAllocArray[i] = stack->array[i]; 
}

delete[] stack->array; //deallocate the old array
stack->array = newAllocArray; //update the pointer to point to the new array instead of the old array
stack->allocationSize = newAllocSize;
}


/*----------------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*Resize a queue*/

QueueRezise(queue){
   //find the new adjusted size (usually the double of the existing allocationSize)
   newSize = queue->allocationSize *2;
   /*make sure that if newSize if greater than the maxLength, then use maxLength as the newSize (you don't want to exceed the maxLength, its your hard stop)*/
   if(newSize > queue->maxLength && queue->maxLength > 0) 
   {
      newSize = queue->maxLength;
   }

   //copy over the existing allocated array into a new expanded array
   newArray = new array[newsize];

   for(int i = 0; i < queue->length; i++) //copying over all the elements that were in the length of the queue
   {
      /*generally you do newArray[i] = oldArray[i] to copy over each element in the array*/
      indexItem = (frontIndex + i) % queue->allocationSize; //this is to implement the circular queue
      newArray[i] = queue->array[indexItem]; //operation that copies over the array. 
   }

   queue->array = newArray; //update the pointer of the queue's array to point to newArray instead of the old array
   queue->allocationSize = newSize;
   queue->frontIndex = 0; //after resizing the array, the frontIndex defaults to 0, so you have to reassign it to zero 
}


/*Enqueue function*/

QueueEnqueue(queue, item) //passing in an item to be inserted into the queue
{
   //check to see if the queue is full
   if(queue->length == queue->maxLength)
   {
      return false;
   }

   //check to see if the queue needs to be resized (if the length of the queue reaches the allocation size, you need to extend the allocationSize to the maxLength)
   if(queue->length == queue->allocationSize)
   {
      QueueResize(queue);
   }

   index = (frontIndex + queue->length) % queue->allocationSize;
   queue->array[index] = item;
   queue->length += 1;
   return true;
}

/*Dequeue Function*/
DequueQueue(queue)
{
   returnItem = queue->array[frontIndex]; //make a copy of the item in the fron to the queue
   queue->length -= 1; //reduce the length of the queue
   frontIndex = (frontIndex + 1 /*did that to make sure you remove the first value and make the next value the new "first" value*/) % queue->allocLength;
   return returnItem;
}



