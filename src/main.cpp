#include <iostream>
#include "game.h"

using namespace std;

int main() {
    int K = 0;
    long int T = 3000000000000; //30 seconds
    string path = "../";
    game_timestamp lastBall = first_half_starting_time;
    game_timestamp nextUpdate = first_half_starting_time + T;
    //cout << lastPos << endl;
    //cout << nextUpdate << endl;
    game_timestamp tempdelta = 0;
    game g;
    g.load(path);                                               //loads csvs into structures


    /*for (int i = 0; i < g.players.size(); i++) {
        cout << g.players[i].name << g.players[i].role << g.players[i].team << endl;
        for (int j = 0; j<g.players[i].sensors.size(); j++)
            cout << g.players[i].sensors[j] << endl;
    }
    */

    /*for (auto elem : g.sensor_id_to_player_index)
        cout << elem.first << elem.second << endl;
    cout << g.referee_index;
    */

    /*for (auto elem : g.events)
        cout << elem.id << " " << elem.type << " " << elem.gts << " " << elem.counter << endl;
    */
    game_timestamp timestamp_offset;


    for (auto record : g.records)
        for (auto sensor : record) {


            if (g.activate_offset)
                timestamp_offset = sensor.ts;
                g.activate_offset = false;

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
                    /*for (auto elemm : g.lastPos)
                        cout << elemm.first << " " << elemm.second[0] << " " << elemm.second[1]<< " " << elemm.second[2] << endl;*/
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

        }
    
    return 0;
}
