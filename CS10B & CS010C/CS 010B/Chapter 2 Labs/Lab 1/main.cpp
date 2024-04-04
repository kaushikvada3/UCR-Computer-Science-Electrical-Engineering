#include <fstream>
#include <iostream>
#include <cstdlib> //needed for exit function

using namespace std;

// Place fileSum prototype (declaration) here

int fileSum(string filename);

int main() {

   string filename;
   int sum = 0;
   
   cout << "Enter the name of the input file: ";
   cin >> filename;
   cout << endl;
   
   sum = fileSum(filename);

   cout << "Sum: " << sum << endl;
   
   return 0;
}

// Place fileSum implementation here
int fileSum (string filename){
   ifstream file; //create an input stream (create a pathway for the water to flow in)
   int sum = 0;
   int ints = 0;
   file.open(filename); //fill in the water into the tank to fill it up
   if(!file.is_open()){ //if there is something wrong opening the file, print our the error
      cout << "Error opening " << filename << endl;
      exit(1);
   }
   
   while(file >> ints){
      sum += ints;
   }  

   return sum; 

   file.close(); //after filling the water tank up, close the water pathway
}    