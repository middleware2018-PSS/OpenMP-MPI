#include <iostream>
#include "parsers.cpp"
#include <omp.h>

using namespace std;

long int T;
game_timestamp K;

const game_timestamp first_half_starting_time = 10753295594424116;  // Starting time of the game
const game_timestamp start_no_ball = 12398000000000000;
const game_timestamp end_no_ball = 12398999999999999;


/**
 * Computes the possession per player during a given microbatch, that is defined as the minimum distance between 2 records of the same player/ball's sensor
 * Appends the microbatched possessiont to possession_attributions
 * @param microbatch_balls a vector of pairs sensor record, delta_ts that must be attributed to the player near to it
 * @param microbatch_players a map from a sensor_id to a sensor_record
 * @param possession_attributions vector to append result to
 * @param g the given game
 */
void microbatch_possession(vector<pair<sensor_record,game_timestamp>> &microbatch_balls,
                                               map<sensor_id, sensor_record> &microbatch_players,
                                               vector<map<int, game_timestamp>> &possession_results,
                                               Game &g){

    sensor_record last_local_ball;    //the first ball of the given microbatch
    pair nearest(-1, K);
    map<int, game_timestamp> possession_attributions;  // map from player index in g.players to attributed time in the microbatch
    double dist = 0;
    unsigned int player_index = 0;
    bool first_ball = true;

    //for every ball record find the nearest player and if the distance is less than K, gives him the possession
    for (auto ball_sensor: microbatch_balls) {

        for (auto player_sensor = microbatch_players.begin();
            player_sensor != microbatch_players.end(); player_sensor++) {

            dist = player_sensor->second.calculate_3D_distance(ball_sensor.first);

            if (nearest.second > dist) {

                nearest.first = player_sensor->second.id;
                nearest.second = dist;

            }

        }
        // if a nearest sensor have been found
        if (nearest.first >= 0) {

            player_index = g.sensor_id_to_player_index[nearest.first];
            auto possession_attr_per_player = possession_attributions.find(player_index);

            //if player already in the map
            if (possession_attr_per_player != possession_attributions.end()) {

                possession_attributions[player_index] = possession_attr_per_player->second + ball_sensor.second;

            } else {

                possession_attributions.insert(pair(player_index, ball_sensor.second));

            }

        }

        //reset temporary variable
        nearest.first = -1;
        nearest.second = K;

    }

    //append microbatched possession result to possession_attributions
    #pragma omp critical
    possession_results.push_back(possession_attributions);
}

/**
 * Aggregates partial microbatches possession times
 * @param possession_results final microbatches' possessions vector
 * @param final_possession_team aggregated possession per team
 * @param final_possession aggregated possession per player
 * @param g the game
 */
void aggregate_results(vector<map<int, game_timestamp>> &possession_results,
        map<char, game_timestamp> &final_possession_team,
        map<string, game_timestamp> &final_possession,
        Game &g){

    //working variables
    player player;
    string name;

    // TODO : parallelize?
    // for every map of microbatches' results, for every key-value pair in the map, aggregates per player and team
    for (auto mb : possession_results) {

        for (auto npt = mb.begin(); npt != mb.end(); npt++) {

            player = g.players[npt->first];
            name = player.name;
            auto pa = final_possession.find(name);
            auto team = final_possession_team.find(player.team);

            if (pa != final_possession.end()) {
                final_possession[name] = pa->second + npt->second;
            } else {
                final_possession.insert(pair(name, npt->second));
            }

            if (team != final_possession_team.end()) {
                final_possession_team[player.team] = team->second + npt->second;
            } else {
                final_possession_team.insert(pair(player.team, npt->second));
            }

        }

    }

}

/**
 * prints the possession result per player and per team so far
 * @param start the game start
 * @param lineNum the number of lines read in the current window
 * @param nextUpdate the closing time of the window
 * @param final_possession possession so far per player
 * @param final_possession_team possession so far per team
 */
void print_results(double &start, int &lineNum,
        bool &playing, game_timestamp &played,
        pair<bool, game_timestamp> &curr_record, game_timestamp &last_begin,
        int &window_number, int &task_num_per_window,
        game_timestamp &nextUpdate,
        map<string, game_timestamp> &final_possession,
        map<char, game_timestamp> final_possession_team){

    //header
    cout << "======================================================" << endl
         << "WINDOW_NUM:" << window_number++ << endl
         << "WINDOW_CLOSE:" << nextUpdate/second << " s" << endl
         << "TASKS:" << task_num_per_window  << endl
         << "NR_LINES:" << lineNum << endl
         << "ELAPSED_TIME:" << (omp_get_wtime() - start) << " s" << endl
         << "TOTAL_TIME:" << (curr_record.second - first_half_starting_time)/second << " s" << endl
         << "------------------------------------------------------" << endl;

    // print players
    for (auto it = final_possession.begin(); it != final_possession.end(); it++) {
        cout << "PLAYER:" << it->first << ":" << it->second/second  << " s" << endl;
    }
    cout << "------------------------------------------------------" << endl;

    // print teams
    for (auto fpt = final_possession_team.begin(); fpt != final_possession_team.end(); fpt++) {
        cout << "TEAM:" << fpt->first << ":" << fpt->second/second  << " s"<< endl;
    }
    cout << "------------------------------------------------------" << endl;
    cout << "PLAYED:" << (playing ? played + curr_record.second - last_begin : played)/second  << " s" << endl;

}

int main (int argc, char *argv[]) {

    //working variables
    ifstream file;
    string line;
    sensor_record sensor;
    pair<bool, game_timestamp> type_ts;
    int task_num_per_window = 0 ;
    int window_number = 0;
    bool playing= false;
    sensor_record last_ball;
    game_timestamp delta_ts= 500000000;
    int lineNum = 0;
    game_timestamp played = 0;
    game_timestamp last_begin = 0;

    map<sensor_id, sensor_record> microbatch_players;               // map from a sensor_id to a sensor record, used to build up the microbatches used in the tasks
    vector<pair<sensor_record,game_timestamp>> microbatch_balls;                         // vector of balls, used to build up the microbatches used in the tasks

    vector<map<int, game_timestamp>> possession_results;      // a vector used to compute final_possession
    map<string, game_timestamp> final_possession;                   // the final possession for a single player, whose name is the key
    map<char, game_timestamp> final_possession_team;                // the final possession for both teams

    K = argc > 4 ? atoi(argv[1])*meter : 3*meter;                       // 3 meters default
    T =  argc > 4 ? atoi(argv[2])*second : 30*second;  // 30 seconds default
    string path = argc > 4 ? argv[3] : "../../data";                   // '../data' default

    game_timestamp nextUpdate = first_half_starting_time + T;       // closing time of the first window, used to compute the successive ones

    Game g(path);   //loads all csvs but game.csv into structures

    file.open(path + "/full-game.csv");

    cout << "K:" << K/meter << "m" << endl << "T:" << T/second << "s" << endl;

    if (!file.is_open())
        throw runtime_error("File not found: " + path);
        file.clear();
        file.seekg(0);

    double start = omp_get_wtime();                                 // starting walltime

    // main loop
    #pragma omp parallel
    #pragma omp single
    {
        while (!file.eof()) {

            lineNum++;

            //read and parse first line
            file >> line;
            stringstream lineStream(line);
            type_ts = type_and_timestamp_parser(lineStream);

            // prints if has passed T seconds from the last print
            if (type_ts.second >= nextUpdate) {

                // wait for the end of running tasks
                #pragma omp taskwait
                if (microbatch_balls.size() > 0){
                    microbatch_possession(microbatch_balls, microbatch_players, possession_results, g);
                }

                // create a task that aggregates partial possession results and print them
                #pragma omp task shared(g, final_possession, final_possession_team, start)  firstprivate(possession_results, playing, played, last_begin, type_ts, window_number, task_num_per_window, lineNum)
                {
                    aggregate_results(possession_results, final_possession_team, final_possession, g);

                    print_results(start, lineNum, playing, played, type_ts, last_begin, window_number, task_num_per_window, nextUpdate, final_possession, final_possession_team);

                }

                // TODO : change to pointer?
                window_number++;
                nextUpdate += T;

                //resets
                lineNum =0;
                task_num_per_window = 0;
                possession_results.clear();

            }

            //if parsed event it's referee event, sets game status accordingly
            if (type_ts.first){

                playing = is_begin_parser(lineStream);

                if (playing) {

                    last_begin = type_ts.second;

                } else {

                    played += type_ts.second - last_begin;

                }

            } else if (playing && (type_ts.second < start_no_ball || type_ts.second > end_no_ball) ) {        //if it's a sensor event and game is playing

                sensor = sensor_record_parser(lineStream);
                sensor.ts = type_ts.second;

                //if sensor is inside the game field
                if (g.is_inside_field(sensor)) {

                    //if sensor belongs to a player
                    if (g.is_player_sensor_id(sensor.id)) {

                        //if sensor has already been encountered, create task and reset microbatches
                        if (microbatch_players.find(sensor.id) != microbatch_players.end()) {

                            task_num_per_window += 1;

                            //compute microbatched possession
                            #pragma omp task shared(g, possession_results) firstprivate(microbatch_balls, microbatch_players)
                            microbatch_possession(microbatch_balls, microbatch_players, possession_results, g);

                            // reset microbatch for players and balls
                            microbatch_players.clear();
                            microbatch_balls.clear();

                        } else {

                            // else is a new player's sensor, add it to the microbatch
                            microbatch_players[sensor.id] = sensor;

                        }

                    } else if (g.balls.find(sensor.id) != g.balls.end()) {

                        // else if it's a ball
                        microbatch_balls.push_back(pair<sensor_record,game_timestamp>(sensor,delta_ts));

                    }

                }

            }

        }

    }

    cout << "================ END =====================" << endl;
    file.close();
    
    return 0;
}
