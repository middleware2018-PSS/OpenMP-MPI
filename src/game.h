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

const game_timestamp max_timestamp = ULONG_MAX;                     // Max timestamp limit
const game_timestamp second = 1000000000000;                        // 1 second in picoseconds, as timestamp
const game_timestamp player_sensor_sample_time = second / 200;      // Player sensor sample time
const game_timestamp ball_sensor_sample_time = second / 2000;       // Ball sensor sample time
const game_timestamp first_half_starting_time = 10753295594424116;  // Starting time of the game
const game_timestamp ball_not_available = 12398000000000000;        // The ball is not available for a certain amount of time. This is the starting time.
const game_timestamp first_half_ending_time = 12557295594424116;    // Ending time of the 1st half of the game
const game_timestamp second_half_starting_time = 13086639146403495; // Starting time of the 2nd half of the game
const game_timestamp second_half_ending_time = 14879639146403495;   // Ending time of the 2nd half of the game
const double MAX_DISTANCE = 87106.0;



struct game_interruption {
    game_timestamp begin;
    game_timestamp end;
};


struct sensor_record {
    sensor_id id;        // Sensor id
    game_timestamp ts; // Timestamp
    int x;                          // Position on the x-axis
    int y;                          // Position on the y-axis
    int z;                          // Position on the z-axis


    inline double calculate_3D_distance(sensor_record& from_sensor)
    {
        return sqrt( (x-from_sensor.x) ^ 2 + (y-from_sensor.y) ^ 2 + (z-from_sensor.z) ^ 2);
    }


    /*
    inline double calculate_2D_distance (sensor_record& from_sensor)
    {
        return sqrt ( ( x - from_sensor.x ) ^ 2 + ( y - from_sensor.y ) ^ 2);
    }
     */
};

struct player {
    string name;        // Player name & surname
    char role;          // Player role. P = Player, G = Goalkeeper, R = Referee
    char team;          // Player team. A for team 1, B for team 2.
    vector<sensor_id> sensors; //Sensor of the players. Every player but Goalkeepers has 2 sensors. Goalkeeper has 4.
    player(): sensors(4) {}     //
};

struct referee_event {
    enum type {
        INTERRUPTION_BEGIN,
        INTERRUPTION_END,
        OTHER
    };
    unsigned int id;                                             // Event ID
    type type;                                          // Event type
    game_timestamp gts;                                 // Event timestamp
    unsigned int counter;                                        // Event counter

};

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

    /*
        game_timestamp total_game_length()
    {
        game_timestamp total_game_length;
        total_game_length = records.back().back().game_timestamp - records.front().front().game_timestamp;
        return total_game_length;
    }

     */


    bool is_player_sensor_id(sensor_id sensor_id)
    {
        auto temp = sensor_id_to_player_index.find(sensor_id);
        return !(temp == sensor_id_to_player_index.end() || temp->second == referee_index );
    }


    bool is_inside_field(sensor_record sensor_record)
    {
        int x = sensor_record.x;
        int y = sensor_record.y;

        if ( 0 < x < 52477 && -33960 < y < 33941 )
            return true;
        return false;
    }



};


#endif //OPENMPMPI_GAME_H
