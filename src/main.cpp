#include <iostream>
#include "parsers.cpp"
#include <omp.h>

using namespace std;

int main (int argc, char *argv[]) {
    int K = 0;
    long int T = 3000000000000; //30 seconds
    string path = "../";
    game_timestamp lastBall = first_half_starting_time;
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

    #pragma omp parallel
    #pragma omp single
    {
        vector<sensor_record> microbatch;
        sensor_record parsed_sensor;
        while (!file.eof()) {
            file >> line;
            parsed_sensor = sensor_record_parser(line);
            //check kind of sensor data
            if (microbatch.size() >= 100) {
                #pragma omp task firstprivate(microbatch)
                {
                    cout << omp_get_thread_num() << " " << microbatch[0].ts << " -> "<< microbatch.back().ts << endl;
            }
            microbatch = vector<sensor_record>();
            } else {
                microbatch.push_back(parsed_sensor);
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
            //check if the sensor is a player's sensor (or add ! for a ball)
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
