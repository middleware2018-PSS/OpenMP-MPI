#ifndef OPENMPMPI_GAME_H
#define OPENMPMPI_GAME_H

#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <cctype>
#include <locale>
#include <set>
#include <map>
#include <cmath>
#include <climits>

using namespace std;

typedef unsigned long int game_timestamp;                           // Timestamp datatype, in picoseconds
typedef unsigned int sensor_id;                                     // Sensor id
const long double second = 1000000000000;
const long int meter = 1000;

//struct representing sensor records
struct sensor_record {
    sensor_id id;        // Sensor id
    game_timestamp ts; // Timestamp
    int x;                          // Position on the x-axis
    int y;                          // Position on the y-axis
    int z;                          // Position on the z-axis

    // calculate distance from a given sensor
    inline double calculate_3D_distance(sensor_record& from_sensor)
    {
        return sqrt( pow(x-from_sensor.x, 2)+ pow(y-from_sensor.y, 2) + pow(z-from_sensor.z, 2));
    }

    inline double calculate_2D_distance(sensor_record& from_sensor)
    {
        return sqrt( pow(x-from_sensor.x, 2) + pow(y-from_sensor.y, 2));
    }
};

//structure representing a player
struct player {
    string name;        // Player name & surname
    char role;          // Player role. P = Player, G = Goalkeeper, R = Referee
    char team;          // Player team. A for team 1, B for team 2.
    vector<sensor_id> sensors; //Sensor of the players. Every player but Goalkeepers has 2 sensors. Goalkeeper has 4.
    player(): sensors(4) {}
};

//class representing the game
class Game {
public:
    vector<player> players;                     // Vector of player
    set<sensor_id> balls;                       // Set of balls
    map<sensor_id, unsigned int> sensor_id_to_player_index;     //Map that maps sensor_id to player indexes
    unsigned int referee_index;

    /**
     * Loads the game CSVs/txts. All the files MUST maintain the given names and relative position.
     *      - referee-events/game-interruption/1st-half.csv
     *      - referee-events/game-interruption/2nd-half.csv
     *      - metadata.txt
     *      - full-game.csv
     *
     * @param path the path of the directory containing all the game data (basepath)
     */
    Game(string& path);

    //checks if a given sensor is attached to a player (elsewhere to a ball)
    bool is_player_sensor_id(sensor_id sensor_id)
    {

        auto temp = sensor_id_to_player_index.find(sensor_id);
        return temp != sensor_id_to_player_index.end() && temp->second != referee_index ;

    }

    //checks if a given sensor is iside the game field
    bool is_inside_field(sensor_record sensor_record)
    {

        return ( 0 < sensor_record.x < 52483 && -33960 <  sensor_record.y < 33960 );

    }

};


#endif //OPENMPMPI_GAME_H
