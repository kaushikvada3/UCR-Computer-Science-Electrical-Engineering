#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
using namespace std;

// hours, minutes, seconds for the timer
int hours = 0;
int minutes = 0;
int seconds = 0;

//to display the timer
void displayClock()
{
    // this is general system call to clear anything that is shown on the screen
    system("clear");

    //formatting
    cout << setfill(' ') << setw(55) << "         TIMER         \n";
    cout << setfill(' ') << setw(55) << " --------------------------\n";
    cout << setfill(' ') << setw(29);
    cout << "| " << setfill('0') << setw(2) << hours << " hrs | ";
    cout << setfill('0') << setw(2) << minutes << " min | ";
    cout << setfill('0') << setw(2) << seconds << " sec |" << endl;
    cout << setfill(' ') << setw(55) << " --------------------------\n";
}

void timer()
{
    // loop until the timer reaches 00:00:00
    while (hours > 0 || minutes > 0 || seconds > 0) {

        // display the timer
        displayClock();

        // sleep system call to sleep for 1 second
        sleep(1);

        // decrement seconds
        seconds--;

        // if seconds reaches -1
        if (seconds < 0) {
            // decrement minutes
            minutes--;

            // if minutes reaches -1
            if (minutes < 0) {
                // decrement hours
                hours--;

                // reset minutes to 59
                minutes = 59;
            }
            // reset seconds to 59
            seconds = 59;
        }
    }

    // After the loop, meaning the timer is finished
    cout << "Time's up!\n";
}

// Driver Code
int main()
{
    // input start time for timer
    cout << "Enter the start time for the timer: \n";
    cout << "Hours: ";
    cin >> hours;
    cout << "Minutes: ";
    cin >> minutes;
    cout << "Seconds: ";
    cin >> seconds;

    // start timer
    timer();
    return 0;
}
