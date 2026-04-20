#ifndef __JUG_H__
#define __JUG_H__

#include <list>
#include <string>
#include <vector>

/*
 * Variable definitions:
 *
 * Ca and Cb are the capacities of the jugs A and B
 * N is the goal
 * cfA is the cost to fill A
 * cfB is the cost to fill B
 * ceA is the cost to empty A
 * ceB is the cost to empty B
 * cpAB is the cost to pour A to B
 * cpBA is the cost to pour B to A
*/

/* I guess ZyBooks doesn't like not having namespace std. 
 * So we're putting it here.
 * RIP :(
 */
using namespace std;

class Vertex
{
  public:
    std::string key;
    std::list<std::pair<int, int>> neighbors;
    Vertex(std::string key) : key(key) { }
    ~Vertex() { }
};

class Jug
{
  private:
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

    //solve is used to check input and find the solution if one exists
    //returns -1 if invalid inputs. solution set to empty string.
    //returns 0 if inputs are valid but a solution does not exist. solution set to empty string.
    //returns 1 if solution is found and stores solution steps in solution string.
    int solve(std::string&);
  private:
    bool isValidInputs() const; // Check if the inputs are valid
    bool isCorrectAmount(std::list<std::pair<int, int>>); // Check the jug capacities against the goal
    int processString(std::vector<int>&, std::string&) const; // Get the total cost and optionally, use string for output
    int processTotalCost(std::vector<int> steps);
    std::vector<int> attempt(); // Attempt to find a solution, return best or none
    int doStep(int&, int&, const int&); // Apply the step at index to the jugs
    std::vector<int> traverse(int, std::vector<int>, std::list<std::pair<int, int>>); // Algorithm to traverse the graph
    bool checkIfCycled(int, int, std::list<std::pair<int, int>>); // Check if the amounts given resulted in a cycle
};

#endif