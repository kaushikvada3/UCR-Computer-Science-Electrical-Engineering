/*----------Linear Search----------*/

bool LinearSearch(numbers[], numbersSize, numKey){
  /* traverse through the array and check 
  to see if numKey matches with the values
  in the array */

  for(unsigned int i = 0l i < numbersSize; i++){
    if(numbers[i] = numKey)
    {
        return true;
    }
  }
  return false;
}


