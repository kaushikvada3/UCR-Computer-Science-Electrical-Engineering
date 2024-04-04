#ifndef __JUG_H__
#define __JUG_H__

#include <list>
#include <string>
#include <vector>

/*
 By default, we'll use the Die Hard scenario (5 gallon jug, 3 gallon jug,
// target 4 gallons in jug A, cost to make any transfer is 1),
// but you can put different scenarios in using the
// Ca, Cb, N, cfA, cfB, ceA, ceB, cpAB, cpBA
// argments described in the 9.8 PROGRAM 4: Jugs description
*/

/*from the looks of the starter-pack, I don't think we are using nameSpace STD in this*/
using namespace std;

class Vertex
{
  public:
   //this is everything in the starter pack
    std::string key; // Unique identifier for the node
    std::list<std::pair<int, int>> neighbors;  // Adjacent nodes represented as pairs
    Vertex(std::string key) : key(key) { } // Constructor initializing the node's id
    ~Vertex() { } //destructor
};

class Jug
{
  private:
  //all of the variables
  /*one represents the capacity of Jug A and Jug B, */
    int Ca;
    int Cb;
    int N;
    int cfA; // fill A
    int cfB; // fill B
    int ceA; // empty A
    int ceB; // empty B
    int cpAB; // pour A B
    int cpBA; // pour B A
    std::vector<Vertex> graph;
  public:
    Jug(int, int, int, int, int, int, int, int, int);
    ~Jug();

    // Helper Method to validate input, execute the solution algorithm, and populate the solution string
    // Returns -1 for invalid inputs, 0 if no solution exists, and 1 if a solution is found.
    int solve(std::string&);
  private:
    bool isValidInputs() const; // Check if the inputs are valid
    bool isCorrectAmount(std::list<std::pair<int, int>>); // Check the jug capacities against the goal
    int processString(std::vector<int>&, std::string&) const; // Get the total cost and optionally, use string for output
    int processTotalCost(std::vector<int> steps); // Tries to find a viable solution path, returns it
    std::vector<int> attempt(); // Attempt to find a solution, return best or none
    int doStep(int&, int&, const int&); // Apply the step at index to the jugs
    std::vector<int> traverse(int, std::vector<int>, std::list<std::pair<int, int>>); // Algorithm to traverse the graph
    bool checkIfCycled(int, int, std::list<std::pair<int, int>>); // Check if the amounts given resulted in a cycle
};

#endif