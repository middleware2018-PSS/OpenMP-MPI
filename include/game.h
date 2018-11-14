/**
 * @file
 * @brief data structures relative to the game
 *
 * @author Lorenzo Petrangeli
*/

#ifndef OPENMP_MPI_PROJECT_GAME_H
#define OPENMP_MPI_PROJECT_GAME_H

//TODO check includes (unneeded 'cause of namespace?)
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

//TODO debug COUT

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



// Referee events structure (interruptions, goal)
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

// Sensor record structure. Unrelevant data are not stored
struct sensor_record {
    sensor_id sensor_id;        // Sensor id
    game_timestamp game_timestamp; // Timestamp
    int x;                          // Position on the x-axis
    int y;                          // Position on the y-axis
    int z;                          // Position on the z-axis

    /**
     *  Calculate 3D distance from a given sensor_record @param from_sensor
     *  @param from_sensor the sensor_record from which the 3D distance is computed
     *  @return calculated 3D distance
     */
    inline double calculate_3D_distance(sensor_record& from_sensor)
    {
        return sqrt( (x-from_sensor.x) ^ 2 + (y-from_sensor.y) ^ 2 + (z-from_sensor.z) ^ 2);
    }

    /**
     * Calculate 2D distance from a given sensor_record @param from_sensor
     * @param from_sensor the sensor_record from ehich the 2D distance is computed
     * @return calculated 2D distance
     */
    inline double calculate_2D_distance (sensor_record& from_sensor)
    {
        return sqrt ( ( x - from_sensor.x ) ^ 2 + ( y - from_sensor.y ) ^ 2);
    }

};

// Player structure
struct player {
    string name;        // Player name & surname
    char role;          // Player role. P = Player, G = Goalkeeper, R = Referee
    char team;          // Player team. A for team 1, B for team 2.
    vector<sensor_id> sensors; //Sensor of the players. Every player but Goalkeepers has 2 sensors. Goalkeeper has 4.
    player(): sensors(4) {}     //
};

// Game interruption structure
struct game_interruption {
    game_timestamp begin;
    game_timestamp end;
};

class game {
public:
    vector<vector<sensor_record>> records;      // Vector of vectors of sensor_records
    vector<referee_event> events;               // Vector of referee_events
    vector<player> players;                     // Vector of player
    set<sensor_id> balls;                       // Set of balls
    vector<game_interruption> game_interruptions;        // Vector of game_interruption
    map<sensor_id, unsigned int> sensor_id_to_player_index;     //Map that maps sensor_id to player indexes
    unsigned int referee_index = 0;             // Referee index is 0

    /**
     * Loads the game CSVs/txts. All the files MUST maintain the given names and relative position.
     *      - referee-events/game-interruption/1st-half.csv
     *      - referee-events/game-interruption/2nd-half.csv
     *      - metadata.txt
     *      - full-game.csv
     *
     * @param path the path of the directory containing all the game data (basepath)
     */
    void load(string& path);

    game_timestamp total_game_length()
    {
        game_timestamp total_game_length;
        total_game_length = records.back().back().game_timestamp - records.front().front().game_timestamp;
        return total_game_length;
    }

    /**
     * Checks if the game is interrupted or not at a given timestamp @param timestamp.
     * @param timestamp a given timestamp to check
     * @return true if the game is interrupted, flase otherwise
     */
    bool is_interrupted(game_timestamp timestamp) const
    {
        for(game_interruption game_interruption : game_interruptions){
            if (timestamp >= game_interruption.begin && timestamp <= game_interruption.end)
                return true;
        }
        return false;
    }

    /**
     * Checks if a given sensor_id @sensor_id is attached to a player or not
     * @param sensor_id a given sensor_id to check
     * @return true if the sensor_id is attached to a player, false otherwise
     */
    bool is_player_sensor_id(sensor_id sensor_id)
    {
        auto temp = sensor_id_to_player_index.find(sensor_id);
        if (temp == sensor_id_to_player_index.end() || temp-> second == referee_index )
            return false;
        return true;

    }

    /**
     * Checks if the cohordinates of a  given sensor_record @param sensor_record are inside the field or not
     * @param sensor_record a given sensor_record to check
     * @return true if the sensor_record cohordinates are inside the field, false otherwise
     */
    bool is_inside_field(sensor_record sensor_record)
    {
        int x = sensor_record.x;
        int y = sensor_record.y;

        if ( 0 < x < 52477 && -33960 < y < 33941 )
            return true;
        return false;
    }

};

/**
 * Parses a string in hh:mm:ss.sss format and returns
 * its value in picoseconds from 00:00:00.000.
 * @param field String representing the time to parse
 * @return Timestamp in picoseconds
 */
game_timestamp parse_time_picoseconds(string field);

/**
 * Loads a referee event csv into a vector of @param referee_events
 * @param events    Vector of parsed events
 * @param path      Csv path
 * @param begin     Starting timestamp
 */
void load_referee_csv( vector<referee_event> &events, string path, unsigned long int begin = 0);



#endif //OPENMP_MPI_PROJECT_GAME_H