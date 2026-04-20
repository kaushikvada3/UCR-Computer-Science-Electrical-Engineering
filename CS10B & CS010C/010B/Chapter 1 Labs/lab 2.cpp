#include <iostream>
#include <vector> 
using namespace std;

void exactChange(int num1, vector<int>& vect1);

int main() {
   int inputVal;
   vector<int> changeAmount(4); //order: pennies, nickels, dimes, quarters
   
   /* Type your code here. Your code must call the function. */
   cin>>inputVal;
   exactChange(inputVal, changeAmount);

   if(inputVal > 0)
   {
    if(changeAmount.at(0) >=2) //pennies
    {
        cout<<changeAmount.at(0)<<" pennies"<<endl;
    }
    else if(changeAmount.at(0) == 1){
        cout<<changeAmount.at(0)<<" penny"<<endl;
    }

    if(changeAmount.at(1) >=2) //Nickels
    {
        cout<<changeAmount.at(1)<<" nickels"<<endl;
    }
    else if(changeAmount.at(1) == 1){
        cout<<changeAmount.at(1)<<" nickel"<<endl;
    }
    
   if(changeAmount.at(2) >=2) //Dimes
    {
        cout<<changeAmount.at(2)<<" dimes"<<endl;
    }
    else if(changeAmount.at(2) == 1){
        cout<<changeAmount.at(2)<<" dime"<<endl;
    }
    
   if(changeAmount.at(3) >=2) //Quarters
    {
        cout<<changeAmount.at(3)<<" quarters"<<endl;
    }
    else if(changeAmount.at(3) == 1){
        cout<<changeAmount.at(3)<<" quarter"<<endl;
    }
   }
   else{
    cout<<"no change"<<endl;
   }

   return 0;
}

/* Implement your function here */ 
void exactChange(int changeInput, vector<int>& changeAmount)
{
    const int QUARTER = 25; 
    const int DIME  = 10; 
    const int NICKEL = 5; 

    //quarters 
    changeAmount.at(3) = changeInput/QUARTER;
    //dimes
    changeAmount.at(2)  = (changeInput - (changeAmount.at(3) * QUARTER))/10;
    //nickels
    changeAmount.at(1) = (changeInput - (changeAmount.at(3) * QUARTER) - (changeAmount.at(2)*DIME))/5;
    //pennies
    changeAmount.at(0) = (changeInput - (changeAmount.at(3) * QUARTER) - (changeAmount.at(2)*DIME) - (changeAmount.at(1)*NICKEL));
}