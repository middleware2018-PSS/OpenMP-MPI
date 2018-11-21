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
#include "game.h"

#define SEPARATOR ','
#define TIMESEPARATOR ':'

using namespace std;

game_timestamp parse_time_picoseconds(string string)
{
    std::string element;
    stringstream lineStream(string);
    double time = 0.0;
    getline(lineStream, element, TIMESEPARATOR);
    time += stof(element) * 3600;
    if (lineStream.rdstate() == ios_base::eofbit)       //if string is 0
        return time * second;
    getline(lineStream, element, TIMESEPARATOR);
    time += stof(element) * 60;
    getline(lineStream, element, TIMESEPARATOR);
    time += stof(element);
    return time * second;
}

referee_event referee_event_parser(string line, unsigned long int begin)
{
    stringstream lineStream(line);
    std::string element;
    referee_event referee_event;

    getline(lineStream, element, SEPARATOR); // parse a single object from the csv into string
    referee_event.id = stoul(element);       // set the id of the event

    getline(lineStream, element, SEPARATOR);

    //sets the type of the referee event
    if(element.compare("gameinterruptionbegin") == 0)
        referee_event.type = referee_event::type::INTERRUPTION_BEGIN;

    else if (element.compare("gameinterruptionend") == 0)
        referee_event.type = referee_event::type::INTERRUPTION_END;

    else
        referee_event.type = referee_event::type::OTHER;

    getline(lineStream, element, SEPARATOR);
    referee_event.gts = begin + parse_time_picoseconds(element); // set the timestamp of the event, starting from begin

    getline(lineStream, element, SEPARATOR);
    referee_event.counter = stoul(element);                      // set the counter of the event

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

sensor_record sensor_record_parser(string line)
{
    sensor_record sensor_record;
    stringstream lineStream(line);
    string element;

    getline(lineStream, element, SEPARATOR);
    sensor_record.sensor_id = stoul(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.game_timestamp = stoull(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.x = stoi(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.y = stoi(element);
    getline(lineStream, element, SEPARATOR);
    sensor_record.z = stoi(element);
    return sensor_record;

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

    string string;

    while(getline(file, string)) {
        events.push_back(referee_event_parser(string, begin));
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

void load_sensors_csv(string path, vector<vector<sensor_record>> &sensors)
{
    ifstream file;
    file.open(path);
    if (!file.is_open())
        throw runtime_error("File not found: " + path);
    file.clear();
    file.seekg(0);
    vector<sensor_record> step;
    vector<string> *producer = new vector<string>();
    vector<string> *consumer = new vector<string>();
    do {
#pragma omp parallel sections
        {
#pragma omp section
            {
                producer->clear();
                string line;
                while (producer->size() < aaasize && getline(file, line))
                    producer->push_back(line);
            }
#pragma omp section
            {
                sensor_record temp;
                for (string& line: *consumer) {
                    temp = sensor_record_parser(line);
                    if (step.size() > 0) {
                        if ((temp.game_timestamp / player_sensor_sample_time) != (step[0].game_timestamp / player_sensor_sample_time)) {
                            sensors.push_back(step);
                            step.clear();
                        }
                    }
                    step.push_back(temp);
                }
            };
        };
        swap(producer, consumer);
    } while (consumer->size() > 0);

    file.close();
    delete producer;
    delete consumer;

}


void game::load(string& path)
{
    records.reserve(50000000);
    load_sensors_csv(path + "ex-full-game.csv", records);
    events.push_back({2010,referee_event::type::INTERRUPTION_END, 0, 0});
    events.push_back({2011,referee_event::type::INTERRUPTION_END, first_half_starting_time, 0});
    load_referee_csv(path + "referee-events/game-interruption/1st-half.csv", events, first_half_starting_time);
    events.push_back({2010, referee_event::type::INTERRUPTION_BEGIN, ball_not_available, 35});
    events.push_back({2011, referee_event::type::INTERRUPTION_END, second_half_starting_time, 35});
    load_referee_csv(path + "referee-events/game-interruption/2nd-half.csv", events, second_half_starting_time);
    events.push_back({6014, referee_event::type::INTERRUPTION_BEGIN, max_timestamp, 39});
    events.push_back({6015, referee_event::type ::INTERRUPTION_END, max_timestamp, 39});

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

    for (int i = 0; i < events.size(); i+=2) {
        game_interruptions.push_back({events[i].gts, events[i+1].gts});
    }



}
