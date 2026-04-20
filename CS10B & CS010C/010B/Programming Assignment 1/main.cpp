#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

void readData(const string &, vector<double> &, vector<double> &);
double interpolation(double, const vector<double> &, const vector<double> &);
bool isOrdered(const vector<double> &);
void reorder(vector<double> &, vector<double> &);

//1.Get from the command line the name of the file that contains the wind tunnel data.

//2.Read wind-tunnel data into two parallel vectors, one vector stores the flight-path angle
//and the other stores the corresponding coefficient of lift for that angle. Both vectors
//should store doubles
//(use the readData() for this)

//3.Ask the user for a flight-path angle. If the angle is within the bounds of the data set, the
//program should then use linear interpolation (see explanation of linear interpolation
//below) to compute the corresponding coefficient of lift and output it.

//4. Finally, ask the user if they want to enter another flight-path angle. Repeat steps 3 and 4
//if they answer Yes, otherwise end the program if they answer No.


//5. for interpolation, the flight path angles in the data file must be in acending order. 

int main(int argc, char* argv[]) // Step 1 Complete
{
    string fileName;
    vector<double> FlightpathAngles;
    vector<double> coffOfLift;
    double userFlightAngleInput = 0;
    fileName = argv[1];


    readData(fileName, FlightpathAngles, coffOfLift);
    cout << "Enter flight-path angle" << endl;
    cin >> FlightpathAnglesInput;
    if(!isOrdered(FlightpathAngles)){
        reorder(FlightpathAngles, coffOfLift);
    }

    if(FlightpathAnglesInput > FlightpathAngles.size()-1 || FlightpathAnglesInput < FlightpathAngles.at(0))
        cout << "Entry out of bounds" << endl;
    else {
        double lift = interpolation(FlightpathAnglesInput, FlightpathAngles, coffOfLift);
        cout << "The coefficient of lift is " << lift << endl;
    }

    string answer; 
    cout << "Do you want to enter another flight-path angle?" << endl;
    cin >> answer;

    while(answer == "Yes"){
        cout << "Enter flight-path angle" << endl;
        cin >> FlightpathAnglesInput;

        if(!isOrdered(FlightpathAngles))
        {
            reorder(FlightpathAngles, coffOfLift); 
        }

        if(FlightpathAnglesInput > FlightpathAngles.size()-1 || FlightpathAnglesInput < FlightpathAngles.at(0))
        {
            cout << "Entry out of bounds" << endl;
        }
            
        else {
            double lift = interpolation(FlightpathAnglesInput, FlightpathAngles, coffOfLift);
            cout << "The coefficient of lift is " << lift << endl;
        }
        cout << "Do you want to enter another flight-path angle?" << endl;
        cin >> answer;
    }


void readData(const string &fileName, vector<double>& flightPathAngles, vector<double>& coefficientsOfLift)
{
    ifstream fileStream;
    fileStream.open(fileName);

    if(!fileStream.is_open())
    {
        cout<<"Error opening "<<fileName<<endl;
        exit(1);
    }

    double val1 = 0.0;
    double val2 = 0.0; 

    while(fileStream >> val1 >> val2)
    {
        flightPathAngles.push_back(val1);
        coefficientsOfLift.push_back(val2);
    }

    fileStream.close();
}


double interpolation(double userAngleInput, const vector<double>& flightPathAngles, const vector<double>& coefficientsOfLift)
{
    unsigned int i; //this is used for the loop
    double answer  = 0;
    int upperBoundVal, lowerBoundVal;
    for(i = 0; i < flightPathAngles.size(); i++)
    {
        //check to see if the userAngleInput is already an angle in the vector
        if(flightPathAngles.at(i) == userAngleInput)
        {
            return coefficientsOfLift.at(i);
        }

        //given that the userAngleInput is angle B, now find the values immedately above and below the userAngleInput in the vector
        if(userAngleInput < flightPathAngles.at(i))
        {
               upperBoundVal = i;
               lowerBoundVal = i-1;
               break;
 
        }

    }
    //f(b) = f(a) + ((b - a)/(c - a))*(f(c) - f(a))
    answer = coefficientsOfLift.at(lowerBoundVal) + ((userAngleInput - flightPathAngles.at(lowerBoundVal))/(flightPathAngles.at(upperBoundVal) - flightPathAngles.at(lowerBoundVal)))*(coefficientsOfLift.at(upperBoundVal) - coefficientsOfLift.at(lowerBoundVal));
    return answer;
}

bool isOrdered(const vector<double>& flightPathAngles)
{
    for(unsigned int i = 1; i < flightPathAngles.size(); i++)
    {
        if(flightPathAngles.at(i) < flightPathAngles.at(i-1))
        {
            return false;  // Data is NOT in ascending order
        }
    }
    return true;  // Data is in ascending order
}


void reorder(vector<double>& flightPathAngles, vector<double>& coefficientsOfLift)
{
    unsigned int i, j;
    for(i = 0; i < flightPathAngles.size(); i++) {
        for(j = 1; j < flightPathAngles.size() - i; j++) {
            if(flightPathAngles.at(j-1) > flightPathAngles.at(j)) {
                swap(flightPathAngles.at(j-1), flightPathAngles.at(j));
                swap(coefficientsOfLift.at(j-1), coefficientsOfLift.at(j));
            }
        }
    }
}