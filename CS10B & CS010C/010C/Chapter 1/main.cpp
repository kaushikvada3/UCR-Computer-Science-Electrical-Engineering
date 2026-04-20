#include <iostream>
#include <string>
using namespace std;
#include "Playlist.h"

void PrintMenu(string playlistName);

int main()
{
    string playlistTitle; // Variable to store the title of the playlist

    char input = ' '; // Character variable to store user input. Initialized to ' ' (space) for use in the control flow of the program (e.g., while loops).

    PlaylistNode currPlaylist; // Object of PlaylistNode to represent the current playlist. This can be used to keep track of playlist details or operations.

    // Pointer variables to manage the linked list structure of the playlist:
    PlaylistNode *head = 0;     // Pointer to the first node in the playlist linked list
    PlaylistNode *tail = 0;     // Pointer to the last node in the playlist linked list
    PlaylistNode *currSong = 0; // Pointer to the current song node
    PlaylistNode *temp = 0;     // temp node pointer

    // user input for title
    cout << "Enter playlist's title:" << endl;
    getline(cin, playlistTitle);
    cout << endl;
    while (input != 'q') // check for quit first
    {
        PrintMenu(playlistTitle); // function call
        // user input for option (cout for input asked in printmenu function)
        cin >> input;
        cin.ignore(1); // to use getline in any following code

        // addsong
        if (input == 'a')
        {
            // initialize variables to take in user input to create playlistnode
            string inpUniqueID = "none";
            string inpSongName = "none";
            string inpArtistName = "none";
            int inpSongLength = 0;
            cout << "ADD SONG" << endl;
            // take user input for 4 parameters to create playlistnode
            cout << "Enter song's unique ID:" << endl;
            getline(cin, inpUniqueID);
            cout << "Enter song's name:" << endl;
            getline(cin, inpSongName);
            cout << "Enter artist's name:" << endl;
            getline(cin, inpArtistName);
            cout << "Enter song's length (in seconds):" << endl;
            cin >> inpSongLength;

            // assign currsong with song to be added
            currSong = new PlaylistNode(inpUniqueID, inpSongName, inpArtistName, inpSongLength);
            if (head == 0) // check for empty list
            {
                head = currSong;
                tail = currSong;
            }
            else // use setnext() to assign currsong after tail
            {
                temp = tail;
                tail = currSong;
                temp->SetNext(currSong);
            }
            cout << endl;
        }

        // removesong
        else if (input == 'd')
        {
            string removeID;
            string removedSongName;
            cout << "REMOVE SONG" << endl;
            cout << "Enter song's unique ID:" << endl;
            getline(cin, removeID); // getline to store entire id

            currSong = head; // assign currsong with head and a temp with head as well
            temp = currSong;
            while (currSong != 0) // loop through list to find matching song id
            {
                if (currSong->GetID() == removeID)
                {
                    removedSongName = currSong->GetSongName();
                    if (head == currSong) // check if first song is input
                    {
                        if (head == tail) // check for list with only one song
                        {
                            tail = 0;
                        }
                        temp = head;
                        head = currSong->GetNext();
                        delete temp;
                        currSong = head;
                        temp = currSong;
                    }
                    else
                    {
                        if (tail == currSong) // check if input is last song
                        {
                            tail = temp;
                        }
                        temp->SetNext(currSong->GetNext());
                        delete currSong;
                        currSong = temp;
                        currSong = currSong->GetNext();
                    }
                    cout << "\"" << removedSongName << "\" removed." << endl;
                }
                else
                {
                    temp = currSong;
                    currSong = currSong->GetNext();
                }
            }
            cout << endl;
        }

        // change song position
        else if (input == 'c')
        {
            int currSongPos, newSongPos;
            int i = 1;
            string currSongName;
            // take inputs for current song and new position
            cout << "CHANGE POSITION OF SONG" << endl;
            cout << "Enter song's current position:" << endl;
            cin >> currSongPos;
            cout << "Enter new position for song:" << endl;
            cin >> newSongPos;

            currSong = head;
            while (i < currSongPos) // find name of song at given position
            {
                temp = currSong;
                currSong = currSong->GetNext();
                ++i;
            }
            currSongName = currSong->GetSongName();
            if (head == currSong) // check if song is at the beginning
            {
                head = currSong->GetNext();
            }
            else if (tail == currSong) // check if song is at the end
            {
                temp->SetNext(currSong->GetNext());
                tail = temp;
            }
            else
            {
                temp->SetNext(currSong->GetNext());
            }

            i = 2;
            temp = head;
            while (i < newSongPos) // find current song at new position of song
            {
                temp = temp->GetNext();
                ++i;
            }

            if (newSongPos <= 1) // check if new position is less than 1 or at head (move to head)
            {
                currSong->SetNext(head);
                head = currSong;
            }
            else if (newSongPos > (currSongPos - 1)) // check if new position is at the end
            {
                tail = currSong;
                temp->InsertAfter(currSong);
            }
            else
            {
                temp->InsertAfter(currSong);
            }
            cout << "\"" << currSongName << "\" moved to position " << newSongPos << endl;
            cout << endl;
        }

        // output by specific artist
        else if (input == 's')
        {
            string currArtistName;
            int songPos = 1;
            cout << "OUTPUT SONGS BY SPECIFIC ARTIST" << endl;
            cout << "Enter artist's name:" << endl;
            getline(cin, currArtistName); // use getline to get entire artist name (including spaces)
            cout << endl;
            temp = head;
            while (temp != 0) // loop through list to match artist name with input
            {
                if (temp->GetArtistName() == currArtistName)
                {
                    cout << songPos << "." << endl;
                    currPlaylist.PrintPlaylistNode(temp);
                }
                temp = temp->GetNext();
                ++songPos; // tracks position of song
            }
        }

        // total time of playlist
        else if (input == 't')
        {
            int totalTime = 0;
            temp = head;
            while (temp != 0) //loop through playlist to += total time 
            {
                totalTime += temp->GetSongLength();
                temp = temp->GetNext();
            }
            cout << "OUTPUT TOTAL TIME OF PLAYLIST (IN SECONDS)" << endl;
            cout << "Total time: " << totalTime << " seconds" << endl;
            cout << endl;
        }

        //output full playlist
        else if (input == 'o')
        {
            cout << playlistTitle << " - OUTPUT FULL PLAYLIST" << endl;
            if (head == 0) //check for empty playlist
            {
                cout << "Playlist is empty" << endl;
                cout << endl;
            }
            else //use printplaylistnode function and loop to loop through and print each node
            {
                currSong = head;
                int i = 1;
                while (currSong != 0)
                {
                    cout << i << "." << endl;
                    currPlaylist.PrintPlaylistNode(currSong);
                    currSong = currSong->GetNext();
                    ++i;
                }
            }
        }
    }

    return 0;
}

//print menu function only prints the menu with the choose an option at the end-- main takes and stores input
void PrintMenu(string playlistName) {
    cout << playlistName << " PLAYLIST MENU" << endl;
    cout << "a - Add song" << endl;
    cout << "d - Remove song" << endl;
    cout << "c - Change position of song" << endl;
    cout << "s - Output songs by specific artist" << endl;
    cout << "t - Output total time of playlist (in seconds)" << endl;
    cout << "o - Output full playlist" << endl;
    cout << "q - Quit" << endl;
    cout << endl;
    cout << "Choose an option:" << endl;
}