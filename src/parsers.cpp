#ifndef PARSERS
#define PARSERS
#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <limits>
#include <climits>
#include <chrono>
#include <queue>
#include "game.h"

#define SEPARATOR ','

using namespace std;

/**
 * parses the first to element from game.csv, common to both referee and player/ball events
 * @param lineStream the input stringstream from file
 * @return a pair with a bool (true if it's a referee_event, false otherwhise) and a timestamp
 */
pair<bool, game_timestamp> type_and_timestamp_parser(stringstream &lineStream){

    bool isR;
    game_timestamp gts;
    string element;

    getline(lineStream, element, SEPARATOR);
    isR = element[0] == 'R';                //set isR to true if it's a referee_event
    getline(lineStream, element, SEPARATOR);
    gts = stoull(element);                  //parses the timestamp of the event
    return pair <bool, game_timestamp> (isR, gts);

}

/**
 * checks if a given event is a begin or an end referee event
 * @param lineStream the input stringstream from file
 * @return true if it's a begine event, false otherwhise
 */
bool is_begin_parser(stringstream &lineStream){

    string element;
    getline(lineStream, element, SEPARATOR);
    return element[0] == 'b';               //return true if element begins with b, false otherwise

}

/**
 * parses a the remaining element of player/ball events
 * @param lineStream the input stringstream from file
 * @return a sensor record with x,y and z cohordinates and his id
 */
sensor_record sensor_record_parser(stringstream &lineStream)
{
    string element;
    sensor_record sensor;

    getline(lineStream, element, SEPARATOR);
    sensor.id = stoul(element);             //sets the id of the sensor
    getline(lineStream, element, SEPARATOR);
    sensor.x = stoi(element);               //sets the x cohordinates of the sensor
    getline(lineStream, element, SEPARATOR);
    sensor.y = stoi(element);               //sets the y cohordinates of the sensor
    getline(lineStream, element, SEPARATOR);
    sensor.z = stoi(element);               //sets the z cohordinates of the sensor
    return sensor;

}

/**
 * parses the player.csv file
 * @param line a line from the given file
 * @return the parsed player
 */
player player_parser(string line)
{

    stringstream lineStream(line);
    string element;
    player player;
    int a;

    getline(lineStream, element, SEPARATOR);
    player.name = element;                  //sets the name of the player
    getline(lineStream, element, SEPARATOR);
    player.role = element[0];               //sets the role of the player
    if (player.role == 'G')                 //checks if the player is a goalkeeper
        a = 4;
    else                                    //the player is not a goolkeeper
        a = 2;
    for (int i = 0; i < a; i++) {
        getline(lineStream, element, SEPARATOR);
        player.sensors.push_back(stoul(element));   //sets the sensors
    }
    getline(lineStream, element, SEPARATOR);
    player.team = element[0];                       //sets the team
    return player;

}



/*
 ***********************************************************************************************************************
 * LOADERS
 ***********************************************************************************************************************
 */

//loads player.csv file
void load_players(string path, vector<player> &players)
{
    ifstream file;
    file.open(path);

    if (!file.is_open())
        throw runtime_error("File not found: " + path);
    string line;
    while(getline(file, line)) {
        players.push_back(player_parser(line));
    }
}

//loads balls.csv file
void load_balls(string path, set<unsigned int>& balls)
{
    ifstream file;
    file.open(path);
    if(!file.is_open())
        throw runtime_error("File not found: " + path);
    for(int i = 0; i < 2; i++) {
        string line;
        getline(file, line);
        stringstream lineStream(line);
        string ball;
        while(lineStream.rdstate() != ios_base::eofbit) {
            getline(lineStream, ball, SEPARATOR);
            balls.insert(stoul(ball));
        }
    }
}

//game loader
Game::Game(string& path)
{
    load_players(path + "/players.csv", players);
    load_balls(path + "/balls.csv", balls);
    for (int i = 0; i < players.size(); i++) {
        for (sensor_id j : players[i].sensors) {
            if(j != 0)
                sensor_id_to_player_index[j] = i;
        }
        if(players[i].role == 'R')
            referee_index = i;
    }

}
#endif
