#include <iostream>
#include <vector>
using namespace std;


bool inOrder(const vector<int> &nums);

int main() {

	vector<int> nums1(5);
	nums1.at(0) = 5;
	nums1.at(1) = 6;
	nums1.at(2) = 7;
	nums1.at(3) = 8;
	nums1.at(4) = 3;


	if (inOrder(nums1) == false) {
        cout << "First vector is not in order" << endl;
    }
    else {
        cout << "First vector is in order" << endl;
    }


	vector<int> nums2(5);
   
    for(unsigned i = 0; i < nums2.size(); i++)
    {
        int input = 0; 
        cin>>input;
        nums2.at(i) = input; 
    }
  

   // Output whether second vector is in order or not in order
   if (inOrder(nums2) == false) {
		cout << "Second vector is not in order" << endl;
	}
	else {
		cout << "Second vector is in order" << endl;
	}
   


	return 0;
}

bool inOrder(const vector<int>& testVector) 
{
    for(unsigned i = 1; i < testVector.size(); i++) 
    {
        if(testVector.at(i-1) > testVector.at(i)) 
        {
            return false;
        }
    }
    return true; 
}

