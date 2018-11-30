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

pair<bool, game_timestamp> check_type_and_timestamp_parser(stringstream &lineStream){
    bool isR;
    game_timestamp gts;
    string element;
    getline(lineStream, element, SEPARATOR);
    isR = element[0] == 'R';
    getline(lineStream, element, SEPARATOR);
    gts = stoull(element);
    return pair <bool, game_timestamp> (isR, gts);
}

bool is_begin_parser(stringstream &lineStream){
    string element;
    getline(lineStream, element, SEPARATOR);
    return element[0] == 'b';
}

sensor_record sensor_record_parser(stringstream &lineStream)
{
    string element;
    getline(lineStream, element, SEPARATOR);
    sensor_record.id = stoul(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.x = stoi(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.y = stoi(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.z = stoi(element);
    return sensor_record;

}

player player_parser(string line)
{
    stringstream lineStream(line);
    string element;
    player player;
    int a;

    getline(lineStream, element, SEPARATOR);
    player.name = element;
    getline(lineStream, element, SEPARATOR);
    player.role = element[0];
    if (player.role == 'G')
        a = 4;
    else
        a = 2;
    for (int i = 0; i < a; i++) {
        getline(lineStream, element, SEPARATOR);
        player.sensors.push_back(stoul(element));
    }
    getline(lineStream, element, SEPARATOR);
    player.team = element[0];
    return player;
}



/*
 ***********************************************************************************************************************
 * LOADERS
 ***********************************************************************************************************************
 */

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


Game::Game(string& path)
{
    load_referee_csv(path + "referee-events/game-interruption/1st-half.csv", events, first_half_starting_time);
    load_referee_csv(path + "referee-events/game-interruption/2nd-half.csv", events, second_half_starting_time);
    load_players(path + "players.csv", players);
    load_balls(path + "balls.csv", balls);
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
