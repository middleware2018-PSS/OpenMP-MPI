#!/usr/bin/env bash

first_half=data/referee-events/game-interruption/1st-half.csv
second_half=data/referee-events/game-interruption/2nd-half.csv
full_game=data/full-game.csv

python3 -u oracle/referee_ts_converter.py 10753295594424116 <  $first_half | awk -F, '{print "R,"$3","$2}' | tee game.tmp
python3 -u oracle/referee_ts_converter.py 13086639146403495 < $second_half | awk -F, '{print "R,"$3","$2}' | tee -a game.tmp
awk -F, '{print "S,"$2","$1","$3","$4","$5}' $full_game | tee -a game.tmp
sort -t , -k2,2 -n < game.tmp
