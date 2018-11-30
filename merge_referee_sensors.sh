#!/usr/bin/env zsh

sort -t , -k2,2 -n \
  <(cat src/data/referee-events/game-interruption/1st-half.csv | python3 -u oracle/referee_ts_converter.py 10753295594424116 | awk -F, '{print "R,"$3","$2}') \
  <(cat src/data/referee-events/game-interruption/2nd-half.csv | python3 -u oracle/referee_ts_converter.py 13086639146403495 | awk -F, '{print "R,"$3","$2}') \
  <(cat data/full-game.csv | awk -F, '{print "S,"$2","$1","$3","$4","$5}')
