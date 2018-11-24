#include <iostream>
#include "parsers.cpp"
#include <omp.h>

using namespace std;

int main (int argc, char *argv[]) {
    const game_timestamp K = argc > 4 ? atoi(argv[1])*1000 : 3000; // TODO
    long int T =  argc > 4 ? atoi(argv[2])*100000000000 : 3000000000000; //30 seconds default
    string path = argc > 4 ? argv[3] : "../";
    sensor_record last_ball;
    game_timestamp delta_ts;
    game_timestamp nextUpdate = first_half_starting_time + T;
    //cout << lastPos << endl;
    //cout << nextUpdate << endl;
    game_timestamp tempdelta = 0;
    Game g(path);   //loads csvs into structures
    ifstream file;
    string line;
    file.open(path + "ex-full-game.csv");
    if (!file.is_open())
        throw runtime_error("File not found: " + path);
    file.clear();
    file.seekg(0);
    map<int, sensor_record> players_sensors;
    vector<map<int, game_timestamp>> possession_results;
    map<string, game_timestamp> final_possession;
    map<char, game_timestamp> final_possession_team;
    for (auto event: g.events) {
        cout << "WAITING FOR "<< (event.type == referee_event::type::INTERRUPTION_BEGIN ? "BEGIN" : "END") << " AT " << event.gts << endl;
        #pragma omp parallel
        #pragma omp single
        {
            map<sensor_id, sensor_record> microbatch_players;
            vector<sensor_record> microbatch_balls;
            sensor_record sensor;
            while (!file.eof()) {
                file >> line;
                sensor = sensor_record_parser(line);
                //cout << sensor.ts << " " << sensor.id << " " << sensor.x << " " << sensor.y << " "
                //     << sensor.z << endl;
                if (event.type == referee_event::type::INTERRUPTION_END && sensor.ts > event.gts) {
                    cout << "stopped game at " << event.gts << ", now " << sensor.ts << endl;
                    break;
                }
               /* bool b1 = first_half_starting_time <= sensor.ts <= second_half_ending_time;
                bool b2 = g.is_inside_field(sensor);
                bool b3 = event.type == referee_event::type::INTERRUPTION_END && sensor.ts <= event.gts;
                bool b4 = event.type == referee_event::type::INTERRUPTION_BEGIN && sensor.ts >= event.gts;*/
                //cout << b1 << b2 <<b3 << b4 << ", event.gts: " <<  event.gts << ", sensor.ts: " << sensor.ts << ", event.type: " << event.type <<endl;
                if (first_half_starting_time <= sensor.ts <= second_half_ending_time
                    && g.is_inside_field(sensor)
                    && ((event.type == referee_event::type::INTERRUPTION_END && sensor.ts <= event.gts)
                        || (event.type == referee_event::type::INTERRUPTION_BEGIN && sensor.ts >= event.gts))) {
                    //cout << b1 << b2 <<b3 << b4 << ", event.gts: " <<  event.gts << ", sensor.ts: " << sensor.ts << ", event.type: " << event.type <<endl;
                    if (g.is_player_sensor_id(sensor.id)) { //is a player
                        if (microbatch_players.find(sensor.id) != microbatch_players.end()) { //never seen player sensor
                            //here execute microbatch computation
                            #pragma omp task firstprivate(microbatch_players, microbatch_balls, delta_ts)
                            {
                                //cout << omp_get_thread_num() << " delta_ts is " << delta_ts << " -> " << (delta_ts*microbatch_balls.size()) << " " << microbatch_balls.size() << " " << microbatch_players.size() << endl;
                                map<int, game_timestamp> possession_attributions;
                                sensor_record last_local_ball;
                                bool first_ball = true;
                                for(auto ball: microbatch_balls){
                                    if (!first_ball)
                                        delta_ts = ball.ts - last_local_ball.ts;
                                    pair nearest(-1, K); // (sensor, dist)
                                    for(auto it = microbatch_players.begin(); it != microbatch_players.end(); it++){
                                        auto dist = it->second.calculate_3D_distance(ball);
                                        if (nearest.second > dist){
                                            nearest.first = sensor.id;
                                            nearest.second = dist;
                                        }
                                    }
                                    auto n = g.sensor_id_to_player_index[nearest.first];
                                    if (n >= 0){
                                        auto pa = possession_attributions.find(n);
                                        if (pa != possession_attributions.end()) {
                                            possession_attributions[n] = pa->second + delta_ts;
                                        } else{
                                            possession_attributions.insert(pair(n,delta_ts));
                                        }
                                    } else {
                                        cout << "Player:None:" << delta_ts << endl;
                                    }
                                    last_local_ball = ball;
                                }
                                #pragma omp critical
                                possession_results.push_back(possession_attributions);
                            }
                            microbatch_players = map<sensor_id, sensor_record>();
                            microbatch_balls = vector<sensor_record>();
                        } else {
                            microbatch_players[sensor.id] = sensor;
                        }
                    } else if (g.balls.find(sensor.id) != g.balls.end()){
                        //is a ball
                        delta_ts = sensor.ts - last_ball.ts;
                        last_ball = sensor;
                        microbatch_balls.push_back(sensor);
                    }
                }
                if (sensor.ts >= nextUpdate) {
                    // cout << sensor.ts << " >= " << nextUpdate << " ? " << (sensor.ts >= nextUpdate)<< endl;

                    #pragma omp taskwait
                    cout << "\n\nfinished tasks for window closed at " << nextUpdate << endl;
                    for (auto mb : possession_results){
                        for (auto npt = mb.begin(); npt != mb.end(); npt++){
                            auto player = g.players[npt->first];
                            auto name = player.name;
                            auto pa = final_possession.find(name);
                            if (pa != final_possession.end()) {
                                final_possession[name] = pa->second + npt->second;
                            } else{
                                final_possession.insert(pair(name,npt->second));
                            }
                            auto team = final_possession_team.find(player.team);
                            if (team != final_possession_team.end()) {
                                final_possession_team[player.team] = team->second + npt->second;
                            } else{
                                final_possession_team.insert(pair(player.team,npt->second));
                            }
                        }
                    }
                    cout << "Window:"<< nextUpdate << endl;
                    for (auto it = final_possession.begin(); it != final_possession.end(); it++){
                        cout << "Player:" << it->first << ":" << it->second << endl;
                    }
                    for (auto fpt = final_possession_team.begin(); fpt != final_possession_team.end(); fpt++){
                        cout << "Team:" << fpt->first << ": " << fpt->second << endl;
                    }
                    possession_results = vector<map<int, game_timestamp>>();
                    nextUpdate += T;
                    // reduce all
                    // print current results
                    // cout << "ok"<< endl;
                }
                if (event.type == referee_event::type::INTERRUPTION_BEGIN && sensor.ts > event.gts){
                    cout << "stopped game at " << event.gts << ", now " << sensor.ts << endl;
                    break;
                }
            }

        }


    }
    file.close();

    /*for (auto window : windows)
        for (auto sensor : window) {
            cout <<

            if (g.activate_offset) {
                timestamp_offset = sensor.ts;
                g.activate_offset = false;
            }
                //check if ts is between starting and ending time, if game is not interrupted at given ts and if is inside game
            if (first_half_starting_time <= sensor.ts <= second_half_ending_time && g.is_interrupted(sensor.ts) && g.is_inside_field(sensor))
                //cout << "ok" << endl;
            //check if the sensor is a player's sensor (or add ! for a ball)/check if the sensor is a player's sensor (
                if (!g.is_player_sensor_id(sensor.id)) {
                    //cout << "palla!" << endl;
                    cout << sensor.ts << " " << sensor.id << " " << sensor.x << " " << sensor.y << " "
                         << sensor.z << endl;

                    //popola lastPos
                    vector<int> temp;
                    temp.push_back(sensor.x);
                    temp.push_back(sensor.y);
                    temp.push_back(sensor.z);
                    g.lastPos[sensor.id] = temp;
                    temp.clear();
                    *//*for (auto elemm : g.lastPos)
                        cout << elemm.first << " " << elemm.second[0] << " " << elemm.second[1]<< " " << elemm.second[2] << endl;*//*
                    if (sensor.ts > timestamp_offset + T) {
                        g.activate_offset = true;
                        cout << "fine periodo" << g.period << endl;
                        g.period++;
                        g.lastPos.clear();
                    }



                }
                    //if it's a ball
                else  {

                    double minimum_distance = MAX_DISTANCE;
                    if (!g.lastPos.empty()){
                        for (auto position : g.lastPos){
                            double temp = sqrt( (sensor.x-position.second[0]) ^ 2 + (sensor.y-position.second[1]) ^ 2 + (sensor.z-position.second[2]) ^ 2);
                            if (temp < minimum_distance) {
                                int delta;
                                minimum_distance = temp;
                                if (g.nearest_sensor == position.first)
                                    int delta = sensor.ts - tempdelta;
                                g.nearest_sensor = position.first;
                                tempdelta = sensor.ts;
                                g.possession_for_sensor[position.first] += delta;
                                //cout << minimum_distance << " " << temp << endl;
                                }
                        }
                    }



                }

        }*/
    
    return 0;
}
