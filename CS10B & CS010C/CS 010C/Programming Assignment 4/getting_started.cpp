// This is NOT the code for program 4, but it tells you a lot about how
// to get started thinking about the problem and how to build the actual graph.
//
// By default, we'll use the Die Hard scenario (5 gallon jug, 3 gallon jug,
// target 4 gallons in jug A, cost to make any transfer is 1),
// but you can put different scenarios in using the
// Ca, Cb, N, cfA, cfB, ceA, ceB, cpAB, cpBA
// argments described in the 9.8 PROGRAM 4: Jugs description
#include <iostream>
#include <iomanip>
#include <limits>
#include <vector>

using std::cout;
using std::endl;
using std::stoi;

const unsigned infinity = std::numeric_limits<unsigned>::max();

struct State {
  int a; // Amount in jug A
  int b; // Amount in jug B

  // Where to go on each of the six actions
  unsigned fillA;
  unsigned fillB;
  unsigned emptyA;
  unsigned emptyB;
  unsigned AtoB;
  unsigned BtoA;

  // These fields are useful when we do Dijkstra's method
  unsigned previous;
  unsigned distance;
  bool visited;

  State(unsigned a=infinity,
	unsigned b=infinity,
	unsigned fillA=infinity,
	unsigned fillB=infinity,
	unsigned emptyA=infinity,
	unsigned emptyB=infinity,
	unsigned AtoB=infinity,
	unsigned BtoA=infinity)
    : a(a),
      b(b),
      fillA(fillA),
      fillB(fillB),
      emptyA(emptyA),
      emptyB(emptyB),
      AtoB(AtoB),
      BtoA(BtoA),
      previous(infinity),
      distance(infinity),
      visited(false)
  {
  }
};

unsigned state_number(unsigned a, unsigned b, unsigned Ca, unsigned Cb) {
  // Simple error handling.  In the program you'll never be out of
  // bounds
  if (a > Ca or b > Cb) {
    throw std::runtime_error("domain error");
  }

  return a*(Cb+1)+b;
}

int main(int argc, char** argv) {
  unsigned Ca=5;	// Gallons in jug A
  unsigned Cb=3;	// Gallons in jug B
  unsigned N=4;		// Target gallons to get in jug A
  unsigned cfA=1;	// Cost to fill jug A
  unsigned cfB=1;	// Cost to fill jug B
  unsigned ceA=1;	// Cost to empty jug A
  unsigned ceB=1;	// Cost to empty jug B
  unsigned cpAB=1;	// Cost to pour from A to B
  unsigned cpBA=1;	// Cost to pour from B to A

  if (argc > 1) Ca = stoi(argv[1]);
  if (argc > 2) Cb = stoi(argv[2]);
  if (argc > 3) N = stoi(argv[3]);
  if (argc > 4) cfA = stoi(argv[4]);
  if (argc > 5) cfB = stoi(argv[5]);
  if (argc > 6) ceA = stoi(argv[6]);
  if (argc > 7) ceB = stoi(argv[7]);
  if (argc > 8) cpAB = stoi(argv[8]);
  if (argc > 9) cpBA = stoi(argv[9]);

  // The first thing to note is that jug A can have
  // 0, 1, 2, ... Ca gallons in it (and sBy default, we'll use the Die Hard scenario (5 gallon jug, 3 gallon jug,
// target 4 gallons in jug A, cost to make any transfer is 1),
// but you can put different scenarios in using the
// Ca, Cb, N, cfA, cfB, ceA, ceB, cpAB, cpBA
// argments described in the 9.8 PROGRAM 4: Jugs descriptionimilarly, jug B
  // can have 0, 1, 2, ... Cb in it).  So we have (Ca+1)*(Cb+1)
  // possible different states.  We are going to number each state
  cout << "Possible states:" << endl;
  int count = 0;
  for(unsigned a=0;a<=Ca;++a) {
    for(unsigned b=0;b<=Cb;++b) {
      cout << "State " << (count++) << ": (" << a << ',' << b << ")" << endl;
    }
  }

  // Notice that there's a pattern... The first Cb+1 states all have their
  // A amount as 0, the next Cb+1 states all have their A amount as 1
  // and so on.
  // This means that I can figure out which state number belongs to any
  // particular configuration if I know Ca and Cb
  cout << "(0,0) is at " << state_number(0,0, Ca,Cb) << endl;
  cout << "(1,1) is at " << state_number(1,1, Ca,Cb) << endl;
  cout << "(2,1) is at " << state_number(2,1, Ca,Cb) << endl;
  

  // Now notice that there are only 6 possible things I can do when
  // I'm in any state:
  // * fill jug A (so jug A will now have Ca gallons in it)
  // * fill jug B (so jug B will now have Cb gallons in it)
  // * empty jug A (so jug A will now have 0 gallons in it)
  // * empty jug B (so jug B will now have 0 gallons in it)
  // * transfer from A to B (add water to B from A until B is full or all of A)
  // * transfer from B to A (add water to A from B until A is full or all of B)
  //
  // For many of these, we end up back in the same state... so, if a is zero
  // and we do the emptyA instruction... we end up in the same state
  cout << "(a,b)  fA fB eA eB a2b b2a" << endl;
  std::vector<State> states;
  for(unsigned a=0;a<=Ca;++a) {
    for(unsigned b=0;b<=Cb;++b) {
      // Compute action fill A.  A will have Ca gallons. B unchanged
      // Compute action fill B.  A unchanged, B will have Cb gallons
      unsigned fillA = state_number(Ca,b, Ca,Cb);
      unsigned fillB = state_number(a,Cb, Ca,Cb);

      // For the empties... just set the appropriate amount to zero
      unsigned emptyA = state_number(0,b, Ca,Cb);
      unsigned emptyB = state_number(a,0, Ca,Cb);

      // For the transfers, we can only pour in as much as we have
      // in the "from" jug OR as much room as we have in the "to" jug
      unsigned transferFromA = std::min(a, Cb-b);
      unsigned AtoB = state_number(a-transferFromA,b+transferFromA, Ca,Cb);
      unsigned transferFromB = std::min(b, Ca-a);
      unsigned BtoA = state_number(a+transferFromB,b-transferFromB, Ca,Cb);
      cout << "(" << a << ',' << b << "): "
	   << std::setw(2)
	   << fillA << ' '
	   << std::setw(2)
	   << fillB << ' '
	   << std::setw(2)
	   << emptyA << ' '
	   << std::setw(2)
	   << emptyB << ' '
	   << std::setw(2)
	   << AtoB << "  "
	   << std::setw(2)
	   << BtoA << endl;

      // Note: We can use emplace_back here (look it up!)
      states.push_back(State(a,b, fillA,fillB, emptyA,emptyB, AtoB,BtoA));
    }
  }

  // Now we have a vector<state> == that's the graph!  Each "node" is connected
  // to six nodes (sometimes itself).
  // Our start state is (0,0).   We can follow a command to get to a new state
  unsigned node = 0;
  cout << "We are now in state " << node
       << " (" << states[node].a << ',' << states[node].b << ')'
       << endl;

  // Fill A
  node = states[node].fillA;
  cout << "We are now in state " << node
       << " (" << states[node].a << ',' << states[node].b << ')'
       << endl;

  // Transfer some of A to B
  node = states[node].AtoB;
  cout << "We are now in state " << node
       << " (" << states[node].a << ',' << states[node].b << ')'
       << endl;

  // Empty B
  node = states[node].emptyB;
  cout << "We are now in state " << node
       << " (" << states[node].a << ',' << states[node].b << ')'
       << endl;
  
  // Move A to B
  node = states[node].AtoB;
  cout << "We are now in state " << node
       << " (" << states[node].a << ',' << states[node].b << ')'
       << endl;

  // To start Dijkstra's method, we pick a start node with both jugs empty
  // (in our numbering, that's node 0).  We set its distance to zero and its
  // previous to nothing (Here, I'm using infinity to mean no node)
  states[0].distance = 0;
  states[0].previous = infinity;

  // To do one step of Dijkstra, we look for the node with the shortest
  // distance that we have not yet visited
  unsigned distance = infinity;
  unsigned current = infinity;
  for(unsigned i=0;i<states.size();++i) {
    if (states[i].distance < distance and not states[i].visited) {
      distance = states[i].distance;
      current = i;
    }
  }
  if (distance == infinity) {
    // Appropriate here to quit
    cout << "Nobody left to visit" << endl;
    return 0;
  }

  // To start with, the start node should be selected since it
  // has distance 0
  cout << "We are going to visit node " << current << endl;
  states[current].visited = true;

  // We need to look at each of the six actions... And see if it
  // takes us a shorter distance than we've seen before.  I demonstrate
  // with the "fillA" action, but you have to do all six paths out
  // of this node.  EVEN THE ONES THAT LEAD RIGHT BACK TO IT!!
  unsigned fillA = states[current].fillA;

  // The distance there is our current distance PLUS the cost
  // of doing a fillA
  unsigned fillA_distance = distance + cfA;

  // We compare the distance we have saved there with the distance
  // that we get with this visit.  If it is shorter, we update
  // the distance AND the previous
  if (fillA_distance < states[fillA].distance) {
    cout << "fillA distance was " << states[fillA].distance
	 << " but I found a path of length " << fillA_distance
	 << endl;
    states[fillA].distance = fillA_distance;
    states[fillA].previous = current;
  }

  // We do this for all six actions and THAT is one Dijkstra's step

  // Keep doing Dijkstra steps until we have either visited all nodes
  // or the shortest remaining distance is infinity
  
  // When I get done, I have the shortest path to each node.  If it
  // is infinity, then there is NO path.
  cout << "Try to find a path to " << state_number(N,0, Ca,Cb) << endl;
  return 0;
}

