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

Jug::Jug(int Ca, int Cb, int N, int cfA, int cfB, int ceA, int ceB, int cpAB, int cpBA) :
    Ca(Ca), Cb(Cb), N(N), cfA(cfA), cfB(cfB), ceA(ceA), ceB(ceB), cpAB(cpAB), cpBA(cpBA)
{
    // Generate the graph
    Vertex cfA_v("cfA");
    Vertex cfB_v("cfB");
    Vertex ceA_v("ceA");
    Vertex ceB_v("ceB");
    Vertex cpAB_v("cpAB");
    Vertex cpBA_v("cpBA");

    cfA_v.neighbors.push_back(std::pair<int, int>(1, cfB));
    // cfA_v.neighbors.push_back(std::pair<int, int>(3, ceB));
    cfA_v.neighbors.push_back(std::pair<int, int>(4, cpAB));

    cfB_v.neighbors.push_back(std::pair<int, int>(0, cfA));
    // cfB_v.neighbors.push_back(std::pair<int, int>(2, ceA));
    cfB_v.neighbors.push_back(std::pair<int, int>(5, cpBA));

    ceA_v.neighbors.push_back(std::pair<int, int>(1, cfB));
    ceA_v.neighbors.push_back(std::pair<int, int>(3, ceB));
    ceA_v.neighbors.push_back(std::pair<int, int>(5, cpBA));

    ceB_v.neighbors.push_back(std::pair<int, int>(0, cfA));
    ceB_v.neighbors.push_back(std::pair<int, int>(2, ceA));
    ceB_v.neighbors.push_back(std::pair<int, int>(4, cpAB));

    cpAB_v.neighbors.push_back(std::pair<int, int>(0, cfA));
    // cpAB_v.neighbors.push_back(std::pair<int, int>(1, cfB));
    // cpAB_v.neighbors.push_back(std::pair<int, int>(2, ceA));
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

int Jug::solve(std::string& solution)
{
    std::vector<int> steps;

    if (!isValidInputs())
    {
        solution = "";
        return -1;
    }

    steps = attempt();

    if (!steps.empty())
    {
        // Found a solution
        processString(steps, solution);
        return 1;
    }
    else
    {
        solution = "";
        return 0;
    }
}

bool Jug::isValidInputs() const
{
    return (N >= 0 && cfA >= 0 && cfB >= 0 && ceA >= 0 && ceB >= 0 && cpAB >= 0 && cpBA >= 0)
        && (Ca > 0 && Ca <= Cb) && (N <= Cb && Cb <= 1000);
}

bool Jug::isCorrectAmount(std::list<std::pair<int, int>> route)
{
    return route.back().first /* A */ == 0 && route.back().second /* B */ == N;
}

int Jug::processString(std::vector<int>& steps, std::string& solution) const
{
    std::ostringstream outSS;
    int totalCost = 0;

    for (unsigned i = 0; i < steps.size(); ++i)
    {
        if (steps.at(i) == 0)
        {
            totalCost += cfA;
            outSS << "fill A" << std::endl;
        }
        else if (steps.at(i) == 1)
        {
            totalCost += cfB;
            outSS << "fill B" << std::endl;
        }
        else if (steps.at(i) == 2)
        {
            totalCost += ceA;
            outSS << "empty A" << std::endl;
        }
        else if (steps.at(i) == 3)
        {
            totalCost += ceB;
            outSS << "empty B" << std::endl;
        }
        else if (steps.at(i) == 4)
        {
            totalCost += cpAB;
            outSS << "pour A B" << std::endl;
        }
        else if (steps.at(i) == 5)
        {
            totalCost += cpBA;
            outSS << "pour B A" << std::endl;
        }
    }

    // Append total cost
    outSS << "success " << totalCost;
    solution = outSS.str();

    return totalCost;
}

int Jug::processTotalCost(std::vector<int> steps)
{
    int totalCost = 0;

    for (unsigned i = 0; i < steps.size(); ++i)
    {
        if (steps.at(i) == 0)
        {
            totalCost += cfA;
        }
        else if (steps.at(i) == 1)
        {
            totalCost += cfB;
        }
        else if (steps.at(i) == 2)
        {
            totalCost += ceA;
        }
        else if (steps.at(i) == 3)
        {
            totalCost += ceB;
        }
        else if (steps.at(i) == 4)
        {
            totalCost += cpAB;
        }
        else if (steps.at(i) == 5)
        {
            totalCost += cpBA;
        }
    }

    return totalCost;
}

std::vector<int> Jug::attempt()
{
    std::list<std::pair<int, int>> aRoute;
    std::list<std::pair<int, int>> bRoute;

    std::vector<int> aSteps;
    std::vector<int> bSteps;

    aRoute.push_back(std::pair<int, int>(0, 0));
    bRoute.push_back(std::pair<int, int>(0, 0));

    aSteps = traverse(0, aSteps, aRoute);
    bSteps = traverse(1, bSteps, bRoute);

    if (aSteps.empty()) 
    {
        // We're guaranteed to return either bSteps or empty steps
        return bSteps;
    }
    else if (bSteps.empty())
    {
        // We're guaranteed to return either aSteps or empty steps
        return aSteps;
    }

    // Return which of the two is better in cost
    return processTotalCost(aSteps) < processTotalCost(bSteps) ? aSteps : bSteps;
}

int Jug::doStep(int& aAmount, int& bAmount, const int& stepIndex)
{
    if (graph.at(stepIndex).key == "cfA")
    {
        aAmount = Ca;
    }
    else if (graph.at(stepIndex).key == "cfB")
    {
        bAmount = Cb;
    }
    else if (graph.at(stepIndex).key == "ceA")
    {
        aAmount = 0;
    }
    else if (graph.at(stepIndex).key == "ceB")
    {
        bAmount = 0;
    }
    else if (graph.at(stepIndex).key == "cpAB")
    {
        if (aAmount + bAmount <= Cb)
        {
            bAmount += aAmount;
            aAmount = 0;
        }
        else
        {
            aAmount -= (Cb - bAmount);
            bAmount = Cb;
        }
    }
    else if (graph.at(stepIndex).key == "cpBA")
    {
        if (aAmount + bAmount <= Ca)
        {
            aAmount += bAmount;
            bAmount = 0;
        }
        else
        {
            bAmount -= (Ca - aAmount);
            aAmount = Ca;
        }
    }

    return stepIndex;
}

std::vector<int> Jug::traverse(int stepIndex, std::vector<int> steps, std::list<std::pair<int, int>> route)
{
    std::vector<int> vect;
    std::vector<int> vectAlt;

    int aAmount;
    int bAmount;
    int stepTaken;

    if (isCorrectAmount(route))
    {
        return steps;
    }
    
    auto it = graph.at(stepIndex).neighbors.begin();

    while (it != graph.at(stepIndex).neighbors.end()) // Finding best route
    {
        aAmount = route.back().first;
        bAmount = route.back().second;
        stepTaken = doStep(aAmount, bAmount, it->first);

        if (!checkIfCycled(aAmount, bAmount, route))
        {
            steps.push_back(stepTaken);
            route.push_back(std::pair<int, int>(aAmount, bAmount));
            
            if (vect.size() == 0)
            {
                vect = traverse(stepTaken, steps, route);
            }
            else
            {
                vectAlt = traverse(stepTaken, steps, route);
                if (processTotalCost(vect) > processTotalCost(vectAlt))
                {
                    vect = vectAlt;
                }
            }

            steps.pop_back();
            route.pop_back();
        }

        it++;
    }

    return vect;
}

bool Jug::checkIfCycled(int aAmount, int bAmount, std::list<std::pair<int, int>> route)
{
    auto it = route.begin();

    while (it != route.end())
    {
        if (it->first == aAmount && it->second == bAmount)
        {
            return true;
        }

        it++;
    }

    return false;
}