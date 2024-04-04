#include <ostream>
using namespace std;

#ifndef PLAYLIST_H
#define PLAYLIST_H

class PlaylistNode{
   public: 
    PlaylistNode();
    PlaylistNode(string inputID, string inputSongName, string inputArtistName, int inputSongLength);

    void InsertAfter(PlaylistNode *input);

    void SetNext(PlaylistNode *input);

    //accessors
    string GetID() const;
    string GetSongName() const;
    string GetArtistName() const;
    int GetSongLength() const;
    PlaylistNode* GetNext() const;

    void PrintPlaylistNode(PlaylistNode *input);

    
    ~PlaylistNode();

    

   private: 
    string uniqueID;
    string songName;
    string artistName;
    int songLength;
    PlaylistNode* nextNodePtr;
};

#endif