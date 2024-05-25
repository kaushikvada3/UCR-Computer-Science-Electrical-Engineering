#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <fstream>
#include <list>

using namespace std;


#ifndef WORDLADDER_H
#define WORDLADDER_H

class WordLadder
{

private:
  list<string> dict;                                // list of possible words in ladder
  bool inDict(string word, list<string> inputDict); // returns true if word exists in given list
  bool offByOne (string &word1, string &word2);

public:
  /* Passes in the name of a file that contains a dictionary of 5-letter words.
       Fills the list (dict) with the words within this file.
       If the file does not open for any reason or if any word within the file
       does not have exactly 5 characters, this function should output an
       error message and return.
    */
  WordLadder(const string &inputDict);

  /* Passes in two 5-letter words and the name of an output file.
     Outputs to this file a word ladder that starts from the first word passed in
     and ends with the second word passed in.
     If either word passed in does not exist in the dictionary (dict),
     this function should output an error message and return.
     Otherwise, this function outputs to the file the word ladder it finds or outputs
     to the file, the message, "No Word Ladder Found."
  */
  void outputLadder(const string &start, const string &end, const string &outputFile);
};

#endif