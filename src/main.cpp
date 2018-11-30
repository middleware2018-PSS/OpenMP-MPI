#include <iostream>
#include "parsers.cpp"
#include <omp.h>

using namespace std;

long int T;
game_timestamp K;

/**
 * Computes the possession per player during a given microbatch, that is defined as the minimum distance between 2 records of the same player/ball's sensor
 * Appends the microbatched possessiont to possession_attributions
 * @param microbatch_balls a vector of sensor records
 * @param microbatch_players a map from a sensor_id to a sensor_record
 * @param possession_attributions vector to append result to
 * @param g the given game
 * @param delta_ts the time between the first player's sensor record and the last ball in the previous window
 */
void microbatch_possession(vector<sensor_record> &microbatch_balls,
                                               map<sensor_id, sensor_record> &microbatch_players,
                                               vector<map<int, game_timestamp>> &possession_results,
                                               Game &g,
                                               game_timestamp init_delta_ts){

    sensor_record last_local_ball;    //the first ball of the given microbatch
    int no_possession = 0;
    pair nearest(-1, K);
    map<int, game_timestamp> possession_attributions;  // map from player index in g.players to attributed time in the microbatch
    double dist = 0;
    unsigned int player_index = 0;
    bool first_ball = true;
    game_timestamp delta_ts=0;
    //for every ball record find the nearest player and if the distance is less than K, gives him the possession
    for (auto ball_sensor: microbatch_balls) {
        if ( !first_ball ){
            delta_ts = ball_sensor.ts - last_local_ball.ts;
            //cout << delta_ts << endl;
        }
        for (auto player_sensor = microbatch_players.begin();
             player_sensor != microbatch_players.end(); player_sensor++) {

            dist = player_sensor->second.calculate_3D_distance(ball_sensor);

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

                possession_attributions[player_index] = possession_attr_per_player->second + delta_ts;

            } else {

                possession_attributions.insert(pair(player_index, delta_ts));

            }

        } else {
            // TODO : remove
            no_possession += delta_ts;

        }
        last_local_ball = ball_sensor;
        first_ball = false;
        //reset temporary variable
        nearest.first = -1;
        nearest.second = K;

    }

    //append microbatched possession result to possession_attributions
    #pragma omp critical
    possession_results.push_back(possession_attributions);
}

// TODO : g method?
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
void print_results(double start, int lineNum, game_timestamp nextUpdate, map<string, game_timestamp> &final_possession, map<char, game_timestamp> &final_possession_team){

    //header
    cout << "lines " << lineNum << " WINDOW:" << nextUpdate - T - first_half_starting_time << " -> " << nextUpdate - first_half_starting_time<< " PRINTED AFTER "
         << (omp_get_wtime() - start) << "s" << endl;

    // print players
    for (auto it = final_possession.begin(); it != final_possession.end(); it++) {
        cout << "PLAYER:" << it->first << ":" << it->second<< endl;
    }

    // print teams
    for (auto fpt = final_possession_team.begin(); fpt != final_possession_team.end(); fpt++) {
        cout << "TEAM:" << fpt->first << ": " << fpt->second << endl;

    }
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
    game_timestamp delta_ts=0;
    int lineNum = 0;
    map<int, game_timestamp> poss;
    map<int, game_timestamp> temp_poss;

    map<sensor_id, sensor_record> microbatch_players;               // map from a sensor_id to a sensor record, used to build up the microbatches used in the tasks
    vector<sensor_record> microbatch_balls;                         // vector of balls, used to build up the microbatches used in the tasks

    vector<map<int, game_timestamp>> possession_results;      // a vector used to compute final_possession
    map<string, game_timestamp> final_possession;                   // the final possession for a single player, whose name is the key
    map<char, game_timestamp> final_possession_team;                // the final possession for both teams

    K = argc > 4 ? atoi(argv[1])*1000 : 1000;                       // 3 meters default
    T =  argc > 4 ? atoi(argv[2])*1000000000000 : 30000000000000;  // 30 seconds default
    string path = argc > 4 ? argv[3] : "../data";                   // '../data' default

    game_timestamp nextUpdate = first_half_starting_time + T;       // closing time of the first window, used to compute the successive ones

    Game g(path);   //loads all csvs but game.csv into structures

    file.open(path + "/full-game.csv");

    if (!file.is_open())
        throw runtime_error("File not found: " + path);
        file.clear();
        file.seekg(0);

    double start = omp_get_wtime();                                 // starting walltime

    cout << "STARTED AT WALLTIME:" << start << endl;

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
                    microbatch_possession(microbatch_balls, microbatch_players, possession_results, g, delta_ts);
                }

                cout << "\n\nfinished " << task_num_per_window << " tasks for window nr " << window_number++
                     << " closed at " << nextUpdate - first_half_starting_time << endl;

                task_num_per_window = 0;        //resets

                // TODO : parallelize?
                // create a task that aggregates partial possession results and print them
                #pragma omp task shared(g, final_possession, final_possession_team, start)  firstprivate(possession_results)
                {
                    aggregate_results(possession_results, final_possession_team, final_possession, g);
                    print_results(start, lineNum, nextUpdate, final_possession, final_possession_team);

                }

                // TODO : change to pointer?
                possession_results.clear();
                nextUpdate += T;
                lineNum =0;

            }

            //if parsed event it's referee event, sets game status accordingly
            if (type_ts.first){

                playing = is_begin_parser(lineStream);
                // TODO : remove
                cout << (playing ? "BEGIN " : "END ") << (type_ts.second - first_half_starting_time)<< endl;
            } else if (playing) {        //if it's a sensor event and game is playing

                sensor = sensor_record_parser(lineStream);
                sensor.ts = type_ts.second;

                //if sensor is inside the game field
                if (g.is_inside_field(sensor)) {

                    //if sensor belongs to a player
                    if (g.is_player_sensor_id(sensor.id)) {

                        //if sensor has already been encountered, create task and reset microbatches
                        if (microbatch_players.find(sensor.id) != microbatch_players.end()) {

                            task_num_per_window += 1;
                            delta_ts = sensor.ts - last_ball.ts;

                            //compute microbatched possession
                            #pragma omp task shared(g, possession_results) firstprivate(microbatch_balls, microbatch_players, delta_ts) private(temp_poss)
                            microbatch_possession(microbatch_balls, microbatch_players, possession_results, g, delta_ts);

                            // reset microbatch for players and balls
                            microbatch_players.clear();
                            microbatch_balls.clear();

                        } else {

                            // else is a new player's sensor, add it to the microbatch
                            microbatch_players[sensor.id] = sensor;

                        }

                    } else if (g.balls.find(sensor.id) != g.balls.end()) {
                        // else if is a ball

                        last_ball = sensor;
                        microbatch_balls.push_back(sensor);

                    }

                }

            }

        }

    }
    cout << "================ END =====================" << endl;
    file.close();
    
    return 0;
}
