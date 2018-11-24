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
#define TIMESEPARATOR ':'

using namespace std;

sensor_record sensor_record_parser(string line)
{
    sensor_record sensor_record;
    stringstream lineStream(line);
    string element;
    getline(lineStream, element, SEPARATOR);
    sensor_record.id = stoul(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.ts = stoull(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.x = stoi(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.y = stoi(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.z = stoi(element);
    return sensor_record;

}

game_timestamp parse_time_picoseconds(string string)
{
    double time = 0.0;
    std::string element;
    stringstream lineStream(string);
    getline(lineStream, element, TIMESEPARATOR);
    if (string == "0"){
        return 0;
    }
    time += stof(element) * 3600;
    getline(lineStream, element, TIMESEPARATOR);
    time += stof(element) * 60;
    getline(lineStream, element, TIMESEPARATOR);
    time += stof(element);
    return time * second;
}

referee_event referee_event_parser(string line, unsigned long int begin)
{
    // cout << line << endl;
    stringstream lineStream(line);
    std::string element;
    referee_event referee_event;

    getline(lineStream, element, SEPARATOR); // parse a single object from the csv into string
    referee_event.id = stoul(element);       // set the id of the event

    getline(lineStream, element, SEPARATOR);
    //cout << "begin: " << referee_event::type::INTERRUPTION_BEGIN << ", end: " << referee_event::type::INTERRUPTION_END << endl;
    //sets the type of the referee event
    if(element.compare("begin") == 0)
        referee_event.type = referee_event::type::INTERRUPTION_BEGIN;
    else {
        if (element.compare("end") == 0)
            referee_event.type = referee_event::type::INTERRUPTION_END;
        else
            referee_event.type = referee_event::type::OTHER;
    }
    getline(lineStream, element, SEPARATOR);
    referee_event.gts = begin + parse_time_picoseconds(element); // set the timestamp of the event, starting from begin

    getline(lineStream, element, SEPARATOR);
    referee_event.counter = stoul(element);// set the counter of the event
    return referee_event;
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

void load_referee_csv(string path, vector<referee_event> &events, unsigned long int begin)
{
    ifstream file;
    file.open(path);
    if (!file.is_open()) {
        throw runtime_error("File not found: " + path);
    }

    string line;

    while(getline(file, line)) {
        events.push_back(referee_event_parser(line, begin));
    }

    file.close();
}

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
