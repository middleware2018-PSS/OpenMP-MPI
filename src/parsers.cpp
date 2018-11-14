//
// Created by Lorenzo Petrangeli on 13/11/18.
//

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

#define SEPARATOR ';'
#define SEPARATOR1 ','

const unsigned int size = 100000;

using namespace std;

/**
 * convert a given string @param string in a game_timestamp in picoseconds.
 * @param string a given string
 * @return string in picoseconds
 */
game_timestamp parse_time_picoseconds(string string)
{
    string element;
    stringstream lineStream(string);
    double time = 0.0;
    getline(lineStream, element, SEPARATOR);
    time += stof(element) * 3600;
    if (lineStream.rdstate() == ios_base::eofbit)       //if string is 0
        return time * second;
    getline(lineStream, element, SEPARATOR);
    time += stof(element) * 60;
    getline(lineStream, element, SEPARATOR);
    time += stof(element);
    return time * second;
}


//TODO change from referee event to game interruption?

/**
 * Parses a single line from a game-interruption file.
 * The format must remain: event_id; event_name; event_time; event_counter.
 * The comment column and the data relative to the statics are not stored because are not in the scope of the project.
 * @param line a single string from file
 * @param begin base timestamp from which start counting
 * @return parsed referee event
 */
referee_event referee_event_parser(string line, unsigned long int begin)
{
    stringstream lineStream(line);
    string element;
    referee_event referee_event;

    getline(lineStream, element, SEPARATOR); // parse a single object from the csv into string
    referee_event.id = stoul(element);       // set the id of the event

    getline(lineStream, element, SEPARATOR);

    //TODO change this part
    element.erase(remove_if(element.begin(), element.end(), isspace), element.end());   // removes spaces from string
    transform(element.begin(), element.end(), element.begin(), ::tolower);             // lowercase string

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

/**
 * Parses a single line from full-game.csv file.
 * The format must remain: sid, ts, x, y, z.
 * The other columns are not stored because are not in the project's scope.
 * @param line is a single line of the file
 * @return the parsed sensor record
 */
sensor_record sensor_record_parser(string line)
{
    sensor_record sensor_record;
    stringstream lineStream(line);
    string element;

    getline(lineStream, element, SEPARATOR1);
    sensor_record.sensor_id = stoul(element);
    getline(lineStream, element, SEPARATOR1);
    sensor_record.game_timestamp = stoull(element);
    getline(lineStream, element, SEPARATOR1);
    sensor_record.x = stoi(element);
    getline(lineStream, element, SEPARATOR1);
    sensor_record.y = stoi(element);
    getline(lineStream, element, SEPARATOR1);
    sensor_record.z = stoi(element);
    return sensor_record;

}

/**
 * Parses a single line from players.csv, that is obtained from metadata.txt using metadata.py
 * @param line is a single line of the file
 * @return a parsed player
 */
player player_parser(string line)
{
    stringstream lineStream(line);
    string element;
    player player;
    int a;

    getline(lineStream, element, SEPARATOR1);
    player.name = element;
    getline(lineStream, element, SEPARATOR1);
    player.role == element[0];
    if (player.role == 'G')
        a = 4;
    else
        a = 2;
    for (i = 0; i < a; i++) {
        getline(lineStream, element, SEPARATOR1);
        player.sensors.push_back(stoul(element));
    }
    getline(lineStream, element, SEPARATOR1);
    player.team = element[0];
    return player;


}



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
                while (produce->size() < size && getline(file, line))
                    producer->push_back(line);
            }
            #pragma omp section
            {
                sensor_record temp;
                for (string& line: *consumer) {
                    temp = sensor_record_parser(line);
                    if (step.size() > 0) {
                        if ((temp.game_timestamp / player_sensor_sample_time) != (step[0].game_timestamp / player_sensor_sample_time)) {
                            vector.push_back(step);
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

/**
 * file created by metadata.txt using balls.py
 * @param path
 * @param balls
 */
void load_balls(string path, set<unsigned int>& balls)
{
    ifstream file;
    file.open(path);
    if(!file.is_open())
        throw new runtime_error("File not found: " + path)
    for(i = 0; i < 2; i++) {
        string line;
        getline(file, line);
        stringstream lineStream(line);
        string ball;
        while(lineStream.rdstate() != ios_base::eofbit) {
            getline(lineStream, ball, SEPARATOR1);
            balls.insert(stoul(ball));
        }
    }

}