#!/usr/bin/env bash

# 2011,Game Interruption End,00:00:03.092,1,empty
first_half=original-data/1st-half.csv
second_half=original-data/2nd-half.csv
# 98,10629342490369879,26406,-6869,1070,1166235,7161971,-5706,3485,-7435,-2410,-8454,-4765 
full_game=original-data/full-game.csv

python3 -u oracle/referee_ts_converter.py 10753295594424116 < $first_half | sed -e 's/Game Interruption \(E\|B\)/\L\1/g' | awk -F, '{print "R,"$3","$2","$1","$4","$5}' > game.tmp
python3 -u oracle/referee_ts_converter.py 13086639146403495 < $second_half | sed -e 's/Game Interruption \(E\|B\)/\L\1/g' | awk -F, '{print "R,"$3","$2","$1","$4}' >> game.tmp
awk -F, '{printf("%s","S,"$2","$1);for(i=3; i<=NF; i++){ printf(",%s", $i) } printf("\n")}' $full_game >> game.tmp
sort -t , -k2,2 -n < game.tmp > $1
rm -f game.tmp
