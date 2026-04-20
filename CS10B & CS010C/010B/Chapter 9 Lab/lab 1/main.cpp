#include <iostream>
#include <string>

using namespace std;


void flipString(string &s);

int main() {
   string line;
   cout << "Enter a sentence:" << endl;
   getline(cin, line);
   cout << endl;
   cout << line << endl;
   flipString(line);
   cout << line << endl;

   return 0;
}

//Implement the flipString function here
void flipString(string &s){
   if(s.size() <= 1)
      return;

   else{
      string temp = " ";
      temp = s.substr(1,s.size()-2);
      flipString(temp);
      s = s.at(s.size() - 1) + temp + s.at(0);
   }
   return;
}

