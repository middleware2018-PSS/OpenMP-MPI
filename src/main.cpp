#include <iostream>
#include "parsers.cpp"
#include <omp.h>

using namespace std;

long int T;
game_timestamp K;


map<int, game_timestamp> microbatch_possession(vector<sensor_record> &microbatch_balls, map<sensor_id, sensor_record> &microbatch_players, Game &g, game_timestamp &delta_ts){
    //cout << omp_get_thread_num() << " delta_ts is " << delta_ts << " -> " << (delta_ts*microbatch_balls.size()) << " " << microbatch_balls.size() << " " << microbatch_players.size() << endl;
    //cout << "POST players " << microbatch_players.size() << " balls " << microbatch_balls.size() << endl;
    sensor_record last_local_ball;
    map<int, game_timestamp> possession_attributions;
    for (auto ball_sensor: microbatch_balls) {
        pair nearest(-1, K); // (sensor, dist)
        for (auto player_sensor = microbatch_players.begin();
             player_sensor != microbatch_players.end(); player_sensor++) {
            auto dist = player_sensor->second.calculate_3D_distance(ball_sensor);
            if (nearest.second > dist) {
                nearest.first = player_sensor->second.id;
                nearest.second = dist;
            }
        }
        if (nearest.first >= 0) {
            auto player_index = g.sensor_id_to_player_index[nearest.first];
            auto possession_attr_per_player = possession_attributions.find(player_index);
            // cout << "Player: " << g.players[player_index].name << endl;
            if (possession_attr_per_player != possession_attributions.end()) {
                possession_attributions[player_index] = possession_attr_per_player->second + delta_ts;
            } else {
                possession_attributions.insert(pair(player_index, delta_ts));
            }
            /*} else {
            none_player_attribution += delta_ts;*/
        }
        last_local_ball = ball_sensor;
    }
    return possession_attributions;
    /*#pragma omp critical
    no_attribution.push_back(none_player_attribution);*/
}

void aggregate_results(vector<map<int, game_timestamp>> &possession_results, map<char, game_timestamp> &final_possession_team, map<string, game_timestamp> &final_possession, Game g){
    player player;
    string name;
    for (auto mb : possession_results) {
        for (auto npt = mb.begin(); npt != mb.end(); npt++) {
            player = g.players[npt->first];
            name = player.name;
            auto pa = final_possession.find(name);
            if (pa != final_possession.end()) {
                final_possession[name] = pa->second + npt->second;
            } else {
                final_possession.insert(pair(name, npt->second));
            }
            auto team = final_possession_team.find(player.team);
            if (team != final_possession_team.end()) {
                final_possession_team[player.team] = team->second + npt->second;
            } else {
                final_possession_team.insert(pair(player.team, npt->second));
            }
        }
    }
    // TODO: better printing
    // cout << T << ", " << (final_possession_team.find('A')->second) << ", " << (final_possession_team.find('B')->second) << ", " << final_no_attribution <<endl;

}
int main (int argc, char *argv[]) {
    map<int, sensor_record> players_sensors;
    vector<map<int, game_timestamp>> possession_results;
    ifstream file;
    string line;
    //vector<game_timestamp> no_attribution;
    //game_timestamp final_no_attribution = 0;
    map<string, game_timestamp> final_possession;
    map<char, game_timestamp> final_possession_team;
    map<sensor_id, sensor_record> microbatch_players;
    vector<sensor_record> microbatch_balls;
    sensor_record sensor;
    game_timestamp none_player_attribution = 0;
    int task_num_per_window = 0 ;
    int window_number = 0;
    K = argc > 4 ? atoi(argv[1])*1000 : 3000; // TODO
    T =  argc > 4 ? atoi(argv[2])*10000000000000 : 30000000000000; //30 seconds default
    string path = argc > 4 ? argv[3] : "../";
    sensor_record last_ball;
    game_timestamp delta_ts=0;

    game_timestamp nextUpdate = first_half_starting_time + T;
    //cout << lastPos << endl;
    //cout << nextUpdate << endl;
    Game g(path);   //loads csvs into structures
    int lineNum = 0;

    file.open(path + "ex-full-game.csv");
    if (!file.is_open())
        throw runtime_error("File not found: " + path);
    // file.clear();
    // file.seekg(0);

    double start = omp_get_wtime();
    cout << "STARTED AT WALLTIME:" << start << endl;
    #pragma omp parallel
    #pragma omp single
    {
        for (auto event: g.events) {
            cout << "WAITING FOR " << (event.type == referee_event::type::INTERRUPTION_BEGIN ? "BEGIN" : "END")
                 << " AT " << event.gts  - first_half_starting_time << endl;

            while (!file.eof()) {
                lineNum++;
                file >> line;
                sensor = sensor_record_parser(line);
                //cout << sensor.ts << " " << sensor.id << " " << sensor.x << " " << sensor.y << " "
                //     << sensor.z << endl;
                if (event.type == referee_event::type::INTERRUPTION_END && sensor.ts > event.gts) {
                    cout << "END AT " << event.gts  - first_half_starting_time<< ", NOW IS " << sensor.ts - first_half_starting_time << endl;
                    break;
                }
                if (((event.type == referee_event::type::INTERRUPTION_END && sensor.ts <= event.gts)
                     || (event.type == referee_event::type::INTERRUPTION_BEGIN && sensor.ts >= event.gts))
                     && g.is_inside_field(sensor)) {
                    //cout << b1 << b2 <<b3 << b4 << ", event.gts: " <<  event.gts << ", sensor.ts: " << sensor.ts << ", event.type: " << event.type <<endl;
                    if (g.is_player_sensor_id(sensor.id)) { //is a player
                        if (microbatch_players.find(sensor.id) != microbatch_players.end()) { //already seen player sensor
                            //here execute microbatch computation
                            task_num_per_window += 1;
                            delta_ts = sensor.ts - last_ball.ts;
                            //cout << "PRE players " << microbatch_players.size() << " balls" << microbatch_balls.size() << endl;
                            #pragma omp task shared(g, possession_results) firstprivate(microbatch_balls, microbatch_players, delta_ts)
                            {
                                auto poss = microbatch_possession(microbatch_balls, microbatch_players, g, delta_ts);
                                #pragma omp critical
                                possession_results.push_back(poss);
                            }
                            microbatch_players.clear();
                            microbatch_balls.clear();
                        } else {
                            last_ball = sensor;
                            microbatch_players[sensor.id] = sensor;
                        }
                    } else if (g.balls.find(sensor.id) != g.balls.end()) {
                        //is a ball
                        microbatch_balls.push_back(sensor);
                    }
                }
                if (sensor.ts >= nextUpdate) {
                    // cout << sensor.ts << " >= " << nextUpdate << " ? " << (sensor.ts >= nextUpdate)<< endl;
                    // TODO take latency time
                    if (microbatch_balls.size() > 0){
                        microbatch_possession(microbatch_balls, microbatch_players, g, delta_ts);
                    }
                    #pragma omp taskwait
                    cout << "\n\nfinished " << task_num_per_window << " tasks for window nr " << window_number++
                         << " closed at " << nextUpdate - first_half_starting_time << endl;
                    task_num_per_window = 0;
                    #pragma omp task shared(g, final_possession, final_possession_team)  firstprivate(possession_results)
                    {
                        /*game_timestamp microbatch_no_attr = 0;
                        for (auto att: no_attribution) {
                            microbatch_no_attr += att;
                        }
                        final_no_attribution += microbatch_no_attr;
                        no_attribution.clear();*/
                        //cout << "no_attribution size: " << no_attribution.size() << " total: "<< microbatch_no_attr << "/" << final_no_attribution<< endl;
                        // TODO: this loop should be parallelized
                        aggregate_results(possession_results, final_possession_team, final_possession, g);

                        cout << "process: " << omp_get_thread_num() << " lines " << lineNum << " WINDOW:" << nextUpdate - T - first_half_starting_time << " -> " << nextUpdate - first_half_starting_time<< " PRINTED AFTER "
                             << (omp_get_wtime() - start) << "s" << endl;
                        //cout << "PLAYER:NONE:" << final_no_attribution << endl;

                        for (auto it = final_possession.begin(); it != final_possession.end(); it++) {
                            cout << "PLAYER:" << it->first << ":" << it->second << endl;
                        }
                        for (auto fpt = final_possession_team.begin(); fpt != final_possession_team.end(); fpt++) {
                            cout << "TEAM:" << fpt->first << ": " << fpt->second << endl;
                        }

                    }
                    possession_results.clear();
                    nextUpdate += T;
                    lineNum =0;
                }
                if (event.type == referee_event::type::INTERRUPTION_BEGIN && sensor.ts > event.gts) {
                    cout << "BEGIN AT " << event.gts  - first_half_starting_time << ", NOW IS " << sensor.ts  - first_half_starting_time<< endl;
                    break;
                }
            }
        }
    }
    cout << "================ END =====================" << endl;
    file.close();
    
    return 0;
}
