#!/usr/bin/env python
import json
from math import sqrt
from functools import reduce
import sys
def convert_ts_to_ms(time):
    t = time.strip().split(":")
    if len(t)>1:
        h, m, s = [float(el) for el in t]
        return int((h*3600 + m*60 + s)*10**12)
    else:
        return 0

def mean(numbers):
    return float(sum(numbers)) / max(len(numbers), 1)

def compute_possession(K, T, m, f, interruptions):
    K=K*10**3
    T=T*10**12
    half=0
    i=0
    metadata=json.load(m)
    # convert keys to ints because json do not accept numeric keys directly
    metadata["players"] = {int(k):v for (k,v) in metadata["players"].items()}
    metadata["halfs"] = {int(k):v for (k,v) in metadata["halfs"].items()}
    lastPos={}  # used to store last know position of each player
    results={}   # store the results
    lastBall=metadata["game"]["start"]   # used store last ball timestamp, to compute possession attribution
    nextUpdate=metadata["game"]["start"]+T    # compute when next update will be outputted
#    print("nextUpdate is at {}".format(nextUpdate))
    for event in interruptions:
        eventType, eventTs = event.strip().split(",")[1:3]
        ms = convert_ts_to_ms(eventTs)
        if ms is 0:
            half+=1
#           print(eventType, half, "time")
        eventTs = metadata["halfs"][half]["start"]+ms
#        print("next Event will be {} at {}".format(eventType, eventTs))
        for row in f:
#           print(row)
            sid, ts, x, y = [int(x) for x in row.split(",")[:4]]
#           i= i + 1 if i < 100000 else 0
            # check if record is in game = > between start and end of game, not paused, inside the field
            if ts >= metadata["game"]["start"] and ts <= metadata["game"]["end"] \
                and ((eventType == "Begin" and ts >= eventTs) or (eventType == "End" and ts <= eventTs)) \
                and abs(y) <= metadata["coord"]["y"]["max"] \
                and x >= metadata["coord"]["x"]["min"] \
                and x <=metadata["coord"]["x"]["max"]:
#               print("ok")
                # if it's a record from a player
                if sid in metadata["players"].keys():
                    # print("player ", sid, ts)
                    pl, part, team = metadata["players"][sid]
                    if pl in lastPos.keys():
                        lastPos[pl][part] = (x,y)
                    else:
                        lastPos[pl] = {part:(x,y)}
                    xp = mean([el[0] for el in lastPos[pl].values()])
                    yp = mean([el[1] for el in lastPos[pl].values()]) 
                    lastPos[pl]["center"] = (xp,yp)
                # if it's a record from a ball
                elif sid in metadata["halfs"][half]["balls"]:
#                   print("ball ", sid, ts)
                    mindist = K
                    nearest_player = 0
#                   print(x, y, lastPos)
                    for player_n, vals in lastPos.items():
                        dist = sqrt((vals["center"][0]-x)**2 + (vals["center"][1]-x)**2) 
                        if dist <= mindist:
                            mindist = dist
                            nearest_player = player_n
		    if nearest_player in metadata["players"].keys():
			delta = ts - lastBall
			results[nearest_player]= results[nearest_player] + delta if nearest_player in results.keys() else delta
			team = metadata["players"][nearest_player][2]
#                       print("nearest_player at ts {} to ball {} is {} of team {} with dist of ".format(ts,sid,nearest_player, team, mindist))
			results[team] = results[team] + delta if team in results.keys() else delta
# 	             else:
#       	           print("no nearest_player {} with K = {} at ts {} to ball {}".format(nearest_player, K*10**-12, ts, sid))
#             elif i is 0: 
#                 a1=ts >= metadata["game"]["start"]
#                 b1=ts <= metadata["game"]["end"]
#                 c1=eventType == "Begin"
#                 c2= ts >= eventTs
#                 c3=eventType == "End"
#                 c4=ts <= eventTs
#                 d1=abs(y) <= metadata["coord"]["y"]["max"]
#                 e1=x >= metadata["coord"]["x"]["min"]
#                 f1=x <=metadata["coord"]["x"]["max"]
#                 print(ts,x,y,eventType, a1 and b1 and ((c1 and c2) or (c3 and c4)) and d1 and e1 and f1)
	    if ts >= nextUpdate:
		print(ts,nextUpdate, results)
		nextUpdate+=T
#    		print("nextUpdate is at {}".format(nextUpdate))
            if (eventType == "Begin" or eventType == "End") and ts > eventTs:
#               print("cond: ", eventType, ts)
                break

if len(sys.argv) <6:
    print("help:\t./script.py K T metadata.json full-game.csv full-game.csv interruptions_stream.csv")
    print("\tK must be in meters and T in seconds")
else:
    K = int(sys.argv[1])
    T = int(sys.argv[2])
    metadata_file = sys.argv[3]
    full_game_file = sys.argv[4]
    interruptions_file = sys.argv[5]
    with open(metadata_file) as m, open(full_game_file) as f, open(interruptions_file) as interruptions:
        compute_possession(K,T,m,f,interruptions)
