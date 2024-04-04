/* Single Linked List Insertion*/

SinglelyLinkedListInsertion(list, newNode, currNode)
{
   /*passes in the linked list, the new node that needs to be inserted, and the current Node "hence currNode" that 
   we will be inserting the new node after*/
   
   //check to see if the linked list is empty or not
   if(list->head ==nullptr) //if the list is empty
   {
      list->head = currNode; //the list's head is currNode
      list->tail = currNode; //the list's tail is also currNode
   }

   else if(currNode == list->tail) //if new node has to be inserted after the last 
   {
     currNode->next = newNode; //instead of the pointer pointing to null, it points to "newNode"
     newNode = list->tail; //updates "newNode" to become the new tail of the list.
   }
   
   else{
      newNode->next = currNode->next; //point newNode to the node next to currNode
      currNode->next = newNode; //point currNode to point to newNode so the insertion is complete
   }
}




// ListFindInsertionPosition(list, dataValue) {
//    curNodeA = null
//    curNodeB = list⇢head
//    while (curNodeB != null and dataValue > curNodeB⇢data) {
//       curNodeA = curNodeB
//       curNodeB = curNodeB⇢next
//    }
//    return curNodeA
// }