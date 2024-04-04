#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
   
   string inputFile;
   string outputFile;
   vector <int> ints; 
   unsigned int loopVal;
   
   // Assign to inputFile value of 2nd command line argument
   inputFile = argv[1];
   // Assign to outputFile value of 3rd command line argument
   outputFile = argv[2];   

   
   // Create input stream and open input csv file.
   ifstream File;
   File.open(inputFile);

   // Verify file opened correctly.
   // Output error message and return 1 if file stream did not open correctly.
   if(!File.is_open()){
      cout << "Error opening " << inputFile << endl;
      exit(1);
   }
   
   // Read in integers from input file to vector.
   int i;
   char c;
   while(File >> i){
      ints.push_back(i);
      File >> c;
   } 
   
   // Close input stream.
   File.close();

   
   // Get integer average of all values read in.
   int aveTotal = 0;
   for(loopVal = 0; loopVal < ints.size(); loopVal++){
      aveTotal += ints.at(loopVal);
   }

   int aveInts = aveTotal/ints.size();

   
   // Convert each value within vector to be the difference between the original value and the average.
    for(loopVal = 0; loopVal < ints.size(); loopVal++){
         ints.at(loopVal) = ints.at(loopVal) - aveInts;
      }
   
   // Create output stream and open/create output csv file.
   ofstream FileVer1;
   FileVer1.open(outputFile);



   // Verify file opened or was created correctly.
   // Output error message and return 1 if file stream did not open correctly.
   if(!FileVer1.is_open()){
      cout << "Error opening " << outputFile << endl;
      exit(1);
   }
   
   // Write converted values into ouptut csv file, each integer separated by a comma.

   for(loopVal = 0; loopVal< ints.size(); ++loopVal){
      if(loopVal == ints.size()-1)
         FileVer1 << ints.at(loopVal);
      else
         FileVer1 << ints.at(loopVal) << ", ";
   }
   
   // Close output stream.
   FileVer1.close();
   
   return 0;
}