#include <iostream>
#include "BSTree.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

void run_tests() {
    cout << "Constructing BS Tree...";
    BSTree bst = BSTree();
    cout << "complete." << endl;

    int totalTests = 0,failedTests = 0;
    totalTests++;
    cout << "Test: testing largest on empty tree...";
    if (bst.largest() == "") {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }
    totalTests++;
    cout << "Test: testing smallest on empty tree...";
    if (bst.smallest() == "") {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }
    /* Test insert (assume search works) */
    totalTests++;
    cout << "Test: testing insert and search...";
    string test_input = "Hello";
    bst.insert(test_input);
    if (bst.search(test_input)) {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }

    totalTests++;
    cout << "Test: testing insert second word and search first and second...";
    string test_input2 = "World";
    bst.insert(test_input2);
    if (bst.search(test_input) && bst.search(test_input2)) {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }

    totalTests++;
    cout << "Test: testing insert duplicate and search first (duplicate) and second...";
    string test_input_duplicate = "Hello";
    bst.insert(test_input_duplicate);
    if (bst.search(test_input) && bst.search(test_input2)) {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }

    totalTests++;
    cout << "Test: testing remove duplicate and search first (duplicate) second...";
    bst.remove(test_input);
    if (bst.search(test_input)) {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }

    totalTests++;
    cout << "Test: testing remove first (now gone)...";
    bst.remove(test_input);
    if (!bst.search(test_input) && bst.search(test_input2)) {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }

    // TODO: Add tests for largest and smallest on non-empty trees
    totalTests++;
    cout << "Test: testing largest and smallest on non-empty trees...";
    string very_large = "Zebra";
    string very_small = "Apple";
    string large = "Yellow";
    string small = "Climb";
    bst.insert(very_large);
    bst.insert(very_small);
    bst.insert(large);
    bst.insert(small);
    if (bst.largest() == "Zebra" && bst.smallest() == "Apple") {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }

    cout << endl << "Constructing 3 new BS Trees...";
    BSTree mutateFuncs = BSTree();
    BSTree bst2 = BSTree();
    BSTree emptyTree = BSTree();
    cout << "complete." << endl;


    // TODO: Add tests for insert and remove on larger trees
    totalTests++;
    cout << "Test: testing for insert and remove on larger trees...";
    mutateFuncs.insert("Apple");
    mutateFuncs.insert("Banana");
    mutateFuncs.insert("Dog");
    mutateFuncs.insert("Elephant");
    mutateFuncs.insert("Ally");
    mutateFuncs.insert("Amand");
    mutateFuncs.insert("Lima");
    mutateFuncs.insert("Zebra");
    mutateFuncs.insert("Dog");
    mutateFuncs.insert("Banana");
    mutateFuncs.insert("Lima");
    mutateFuncs.insert("Echo");
    mutateFuncs.insert("Dog");

    mutateFuncs.remove("Dog");
    mutateFuncs.remove("Elephant");
    mutateFuncs.remove("Dog");
    mutateFuncs.remove("Apple");

    // checks if apple is removed, if banana replaced apple, and if dog was not removed and only decremented
    if (mutateFuncs.height("Apple") == -1 && mutateFuncs.height("Dog") == 3 && mutateFuncs.height("Banana") == 4){
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }

    // TODO: Add tests for height of nodes (empty and larger trees)
    totalTests++;
    cout << "Test: testing for height of nodes (empty and larger trees)...";
    bst2.insert("Apple");
    bst2.insert("Banana");
    bst2.insert("Dog");
    bst2.insert("Elephant");
    bst2.insert("Ally");
    bst2.insert("Amand");
    bst2.insert("Lima");
    bst2.insert("Zebra");
    // apple is root, zebra is leaf, amand is also leaf, empty tree should have height of -1
    if (bst2.height("Apple") == 5 && bst2.height("Zebra") == 0 && bst2.height("Amand") == 0 && emptyTree.height("") == -1) {
        cerr << "Passed" << endl;
    } else {
        cerr << "Failed" << endl;
        failedTests++;
    }    
    cout << endl << "Constructing new BS Tree...";
    BSTree bst3 = BSTree();
    cout << "complete." << endl;

    // Manually inspect pre post and inorder traversals
    totalTests++;
    cout << "Test: testing pre post and inorder traversals...";
    bst3.insert("Apple");
    bst3.insert("Banana");
    bst3.insert("Dog");
    bst3.insert("Elephant");
    bst3.insert("Ally");
    bst3.insert("Amand");
    bst3.insert("Lima");
    bst3.insert("Zebra");

    cout << "Expected tree (pre order) ";
    cout << "Apple(1), Ally(1), Amand(1), Banana(1), Dog(1), Elephant(1), Lima(1), Zebra(1)" << endl; 
    cout << "Preorder: ";
    bst3.preOrder();
    cout << endl << endl;

    cout << "Expected tree (post order) ";
    cout << "Amand(1), Ally(1), Zebra(1), Lima(1), Elephant(1), Dog(1), Banana(1), Apple(1)" << endl; 
    cout << "Postorder: ";
    bst3.postOrder();
    cout << endl << endl;

    cout << "Expected tree (in order) ";
    cout << "Ally(1), Amand(1), Apple(1), Banana(1), Dog(1), Elephant(1), Lima(1), Zebra(1)" << endl; 
    cout << "Inorder: ";
    bst3.inOrder();
    cout << endl << endl;

    cout << "Passed " << totalTests - failedTests << " / " << totalTests << " tests." << endl;
}
