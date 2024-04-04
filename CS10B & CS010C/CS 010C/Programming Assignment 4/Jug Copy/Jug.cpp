#include "Jug.h"

#include <iostream>
#include <sstream>

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

//constructor
Jug::Jug(int Ca, int Cb, int N, int cfA, int cfB, int ceA, int ceB, int cpAB, int cpBA) :
    Ca(Ca), Cb(Cb), N(N), cfA(cfA), cfB(cfB), ceA(ceA), ceB(ceB), cpAB(cpAB), cpBA(cpBA)
{
    // This is used to generate a graph
    Vertex cfA_v("cfA");
    Vertex cfB_v("cfB");
    Vertex ceA_v("ceA");
    Vertex ceB_v("ceB");
    Vertex cpAB_v("cpAB");
    Vertex cpBA_v("cpBA");

   /*constructing a graph data structure where each node represents
    a different state or action related to two water jugs*/

    /*Each Vertex object has a "neighbors" list, which represents 
    the possible transitions from one state/action to another*/

    cfA_v.neighbors.push_back(std::pair<int, int>(1, cfB));
    cfA_v.neighbors.push_back(std::pair<int, int>(4, cpAB));

    cfB_v.neighbors.push_back(std::pair<int, int>(0, cfA));
    cfB_v.neighbors.push_back(std::pair<int, int>(5, cpBA));

    ceA_v.neighbors.push_back(std::pair<int, int>(1, cfB));
    ceA_v.neighbors.push_back(std::pair<int, int>(3, ceB));
    ceA_v.neighbors.push_back(std::pair<int, int>(5, cpBA));

    ceB_v.neighbors.push_back(std::pair<int, int>(0, cfA));
    ceB_v.neighbors.push_back(std::pair<int, int>(2, ceA));
    ceB_v.neighbors.push_back(std::pair<int, int>(4, cpAB));

    cpAB_v.neighbors.push_back(std::pair<int, int>(0, cfA));
    cpAB_v.neighbors.push_back(std::pair<int, int>(3, ceB));
    cpAB_v.neighbors.push_back(std::pair<int, int>(5, cpBA));

    cpBA_v.neighbors.push_back(std::pair<int, int>(0, cfA));
    cpBA_v.neighbors.push_back(std::pair<int, int>(1, cfB));
    cpBA_v.neighbors.push_back(std::pair<int, int>(2, ceA));
    cpBA_v.neighbors.push_back(std::pair<int, int>(3, ceB));
    cpBA_v.neighbors.push_back(std::pair<int, int>(4, cpAB));

    graph.push_back(cfA_v);  // 0
    graph.push_back(cfB_v);  // 1
    graph.push_back(ceA_v);  // 2
    graph.push_back(ceB_v);  // 3
    graph.push_back(cpAB_v); // 4
    graph.push_back(cpBA_v); // 5
}

Jug::~Jug()
{
    graph.clear();
}


// Helper Method to validate input, execute the solution algorithm, and populate the solution string
// Returns -1 for invalid inputs, 0 if no solution exists, and 1 if a solution is found.
int Jug::solve(std::string& solution)
{
    std::vector<int> steps; // This will hold the sequence of actions (steps) that form the solution

    if (!isValidInputs()) // Check if the inputs to the problem are valid
    {
        solution = ""; // If inputs are invalid, clear the solution string
        return -1; // Return -1 indicating invalid inputs
    }

    steps = attempt(); // Try to find a solution to the problem, filling 'steps' with the sequence of actions if successful

    if (!steps.empty()) // Check if a solution was found (i.e., steps are not empty)
    {
        // Found a solution
        processString(steps, solution); // Convert the sequence of steps into a readable format and store in 'solution'
        return 1; // Return 1 indicating a successful solution
    }
    else
    {
        solution = ""; // If no solution was found, clear the solution string
        return 0; // Return 0 indicating that no solution exists for the given inputs
    }
}

/*checking to see if each input is a valid input or not*/
bool Jug::isValidInputs() const
{
    return (N >= 0 && cfA >= 0 && cfB >= 0 && ceA >= 0 && ceB >= 0 && cpAB >= 0 && cpBA >= 0)
        && (Ca > 0 && Ca <= Cb) && (N <= Cb && Cb <= 1000);
}

/*checks to see if the final state of the jugs (represented by the last element in the 'route' list) meets a specific goal condition.*/
bool Jug::isCorrectAmount(std::list<std::pair<int, int>> route)
{
    // Returns true if the final state in the 'route' meets the following conditions:
    // 1. The quantity of water in jug A (represented by 'first' in the last pair of the list) is 0.
    // 2. The quantity of water in jug B (represented by 'second' in the last pair of the list) is equal to N (the target volume).
    return route.back().first /* A */ == 0 && route.back().second /* B */ == N;
}


// Basically, what this function does is translate a the sequence of steps to human-readable instructions to calculate the total cost.
int Jug::processString(std::vector<int>& steps, std::string& solution) const
{
    std::ostringstream outSS; // Used to build the solution string.
    int totalCost = 0; // Accumulates the total cost of all steps.

    // Iterate through each step in the vector.
    for (unsigned i = 0; i < steps.size(); ++i)
    {
        // Check the action represented by each step and handle accordingly.
        if (steps.at(i) == 0) // Action to fill jug A.
        {
            totalCost += cfA; // Add cost to total.
            outSS << "fill A" << std::endl; // Append action to solution string.
        }
        else if (steps.at(i) == 1) // Action to fill jug B.
        {
            totalCost += cfB;
            outSS << "fill B" << std::endl;
        }
        else if (steps.at(i) == 2) // Action to empty jug A.
        {
            totalCost += ceA;
            outSS << "empty A" << std::endl;
        }
        else if (steps.at(i) == 3) // Action to empty jug B.
        {
            totalCost += ceB;
            outSS << "empty B" << std::endl;
        }
        else if (steps.at(i) == 4) // Action to pour from A to B.
        {
            totalCost += cpAB;
            outSS << "pour A B" << std::endl;
        }
        else if (steps.at(i) == 5) // Action to pour from B to A.
        {
            totalCost += cpBA;
            outSS << "pour B A" << std::endl;
        }
    }

    // Append the total cost to the solution string.
    outSS << "success " << totalCost;
    // Set the solution reference to the built string.
    solution = outSS.str();

    // Return the total cost of the solution.
    return totalCost;
}


// This function calculates the total cost of a sequence of steps performed on the jugs.
int Jug::processTotalCost(std::vector<int> steps)
{
    int totalCost = 0; // Initialize total cost.

    // Iterate through each step in the vector.
    for (unsigned i = 0; i < steps.size(); ++i)
    {
        // Add the cost of each step to the total cost based on the action performed:
        if (steps.at(i) == 0)
        {
            totalCost += cfA; // Cost of filling jug A.
        }
        else if (steps.at(i) == 1)
        {
            totalCost += cfB; // Cost of filling jug B.
        }
        else if (steps.at(i) == 2)
        {
            totalCost += ceA; // Cost of emptying jug A.
        }
        else if (steps.at(i) == 3)
        {
            totalCost += ceB; // Cost of emptying jug B.
        }
        else if (steps.at(i) == 4)
        {
            totalCost += cpAB; // Cost of pouring from jug A to jug B.
        }
        else if (steps.at(i) == 5)
        {
            totalCost += cpBA; // Cost of pouring from jug B to jug A.
        }
    }

    return totalCost; // Return the computed total cost.
}


// this function "Attempts" (hence the name) to find the best solution between two routes for solving the Jug problem
std::vector<int> Jug::attempt()
{
    // Initialize routes and step sequences for both jugs
    std::list<std::pair<int, int>> aRoute; // Track movements for Jug A
    std::list<std::pair<int, int>> bRoute; // Track movements for Jug B

    std::vector<int> aSteps; // Sequence of actions for Jug A
    std::vector<int> bSteps; // Sequence of actions for Jug B

    // Start both routes with the jugs being empty (0 gallons)
    aRoute.push_back(std::pair<int, int>(0, 0));
    bRoute.push_back(std::pair<int, int>(0, 0));

    // Traverse the graph to find the sequence of steps for each route
    aSteps = traverse(0, aSteps, aRoute); // Find steps for route A
    bSteps = traverse(1, bSteps, bRoute); // Find steps for route B

    // Determine the best route based on availability and cost
    if (aSteps.empty()) 
    {
        // If there are no steps for route A, return steps for route B or empty if both have no steps
        return bSteps;
    }
    else if (bSteps.empty())
    {
        // If there are no steps for route B, return steps for route A
        return aSteps;
    }

    // If both routes have steps, return the one with the lower total cost
    return processTotalCost(aSteps) < processTotalCost(bSteps) ? aSteps : bSteps;
}


//  performs a specific action (step) on the two jugs based on the stepIndex provided.
int Jug::doStep(int& aAmount, int& bAmount, const int& stepIndex)
{
    // Fill jug A to its maximum capacity.
    if (graph.at(stepIndex).key == "cfA")
    {
        aAmount = Ca;
    }
    // Fill jug B to its maximum capacity.
    else if (graph.at(stepIndex).key == "cfB")
    {
        bAmount = Cb;
    }
    // Empty jug A completely.
    else if (graph.at(stepIndex).key == "ceA")
    {
        aAmount = 0;
    }
    // Empty jug B completely.
    else if (graph.at(stepIndex).key == "ceB")
    {
        bAmount = 0;
    }
    // Pour water from jug A to jug B without exceeding jug B's capacity and ddjusts the water levels of both jugs accordingly.
    else if (graph.at(stepIndex).key == "cpAB")
    {
        if (aAmount + bAmount <= Cb) // Check if total water fits in jug B
        {
            bAmount += aAmount; // Pour all from A to B
            aAmount = 0; // Jug A becomes empty
        }
        else
        {
            aAmount -= (Cb - bAmount); // Pour only the amount needed to fill B
            bAmount = Cb; // Jug B is filled to its maximum capacity
        }
    }
    // Pour water from jug B to jug A without exceeding jug A's capacity and also adjust the water levels of both jugs accordingly.
    else if (graph.at(stepIndex).key == "cpBA")
    {
        if (aAmount + bAmount <= Ca) // Check if total water fits in jug A
        {
            aAmount += bAmount; // Pour all from B to A
            bAmount = 0; // Jug B becomes empty
        }
        else
        {
            bAmount -= (Ca - aAmount); // Pour only the amount needed to fill A
            aAmount = Ca; // Jug A is filled to its maximum capacity
        }
    }

    // Return the index of the performed step for further reference or logging.
    return stepIndex;
}


// attempts to traverse the graph to find a solution path for the water jug problem.
std::vector<int> Jug::traverse(int stepIndex, std::vector<int> steps, std::list<std::pair<int, int>> route)
{
    // Initialize vectors for holding the current best solution and an alternative solution.
    std::vector<int> vect;
    std::vector<int> vectAlt;

    // Variables to store current amounts in each jug and the last step taken.
    int aAmount;
    int bAmount;
    int stepTaken;

    // Check if the current route has reached the correct jug amounts, return current steps if so.
    if (isCorrectAmount(route))
    {
        return steps;
    }
    
    // Iterator for navigating through the neighbors of the current graph node.
    auto it = graph.at(stepIndex).neighbors.begin();

    // Iterate through all possible next steps from the current state.
    while (it != graph.at(stepIndex).neighbors.end()) // Finding the best route
    {
        // Retrieve current water levels from the last pair in the route.
        aAmount = route.back().first;
        bAmount = route.back().second;
        // Perform the next step and update the amounts in each jug.
        stepTaken = doStep(aAmount, bAmount, it->first);

        // Check if the new state has been visited before to avoid cycles.
        if (!checkIfCycled(aAmount, bAmount, route))
        {
            // Update the steps and route with the new state.
            steps.push_back(stepTaken);
            route.push_back(std::pair<int, int>(aAmount, bAmount));
            
            // Explore the new state by recursive traversal.
            if (vect.size() == 0) // If no solution has been found yet, use the new path.
            {
                vect = traverse(stepTaken, steps, route);
            }
            else // An alternative solution exists, compare the new path with the current best path.
            {
                vectAlt = traverse(stepTaken, steps, route);
                if (processTotalCost(vect) > processTotalCost(vectAlt)) // Choose the path with the lower total cost.
                {
                    vect = vectAlt;
                }
            }

            // Backtrack: Remove the last step and state to explore other possibilities.
            steps.pop_back();
            route.pop_back();
        }

        // Move to the next possible action.
        it++;
    }

    // Return the best solution found.
    return vect;
}


// The function check to see if a specific state, that has been defined by the amounts of water in two jugs (aAmount and bAmount),
// has already been encountered in the current path (route).
bool Jug::checkIfCycled(int aAmount, int bAmount, std::list<std::pair<int, int>> route)
{
    auto it = route.begin(); // Initialize an iterator to traverse through the route list from the beginning.

    while (it != route.end()) // Loop through all elements in the route.
    {
        if (it->first == aAmount && it->second == bAmount) // Check if the current state matches the given state.
        {
            return true; // Return true if a cycle is found (the state has been previously encountered).
        }

        it++; // Move to the next state in the route.
    }

    return false; // Return false if no cycle is detected (the state is unique within this route).
}
