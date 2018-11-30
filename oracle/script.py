#!/usr/bin/env python
import csv
import json
import sys
import numpy as np

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
    # i=0

    metadata=json.load(m)

    # convert keys to ints because json do not accept numeric keys directly
    metadata["players"] = {int(k):v for (k,v) in metadata["players"].items()}
    metadata["halfs"] = {int(k):v for (k,v) in metadata["halfs"].items()}
    lastPos={}  # used to store last know position of each player
    results={}   # store the results
    lastBall=metadata["game"]["start"]   # used store last ball timestamp, to compute possession attribution
    nextUpdate=metadata["game"]["start"]+T    # compute when next update will be outputted
    players_team = {}

    for v in metadata["players"].values():
        players_team[v[0]] = v[2]
    players_nums= set([el[0] for el in metadata["players"].values()])

    # print("nextUpdate is at {}".format(nextUpdate))
    for event in interruptions:
        eventType, eventTs = event[1:3]
        ms = convert_ts_to_ms(eventTs)
        if ms is 0:
            half+=1
#           print(eventType, half, "time")
        eventTs = metadata["halfs"][half]["start"]+ms
#        print("next Event will be {} at {}".format(eventType, eventTs))
        for row in f:
            sid, ts, x, y = row[:4]
#           print(row)
            if (eventType == "End") and ts > eventTs:
                # print("cond: ", eventType, ts)
                break
            # i= i + 1 if i < 100000 else 0
            # check if record is in game = > between start and end of game, not paused, inside the field
            if ts >= metadata["game"]["start"] and ts <= metadata["game"]["end"] \
                and ((eventType == "Begin" and ts >= eventTs) or (eventType == "End" and ts <= eventTs)) \
                and abs(y) <= metadata["coord"]["y"]["max"] \
                and x >= metadata["coord"]["x"]["min"] \
                and x <=metadata["coord"]["x"]["max"]:
                # print("ok")
                # if it's a record from a player
                coord = np.array([x,y])
                if sid in metadata["players"].keys():
                    # print("player ", sid, ts)
                    pl, part, team = metadata["players"][sid]
                    if pl in lastPos.keys():
                        lastPos[pl][part] = coord
                    else:
                        lastPos[pl] = {part:coord}
                    if "center" in lastPos[pl].keys():
                        lastPos[pl].pop("center")
                    lastPos[pl]["center"] = np.mean(np.array([el[0:2] for el in lastPos[pl].values()]), axis=0) if len(lastPos[pl]) >= 2 else coord
                # if it's a record from a ball
                elif sid in metadata["halfs"][half]["balls"]:
                    # print("ball ", sid, ts)
                    mindist = K
                    nearest_player = -1
#                   print(x, y, lastPos)
                    for player_n, vals in lastPos.items():
                        dist = np.linalg.norm(vals["center"]-coord)
                        if dist <= mindist:
                            mindist = dist
                            nearest_player = player_n
                    if nearest_player in players_nums:
                        delta = ts - lastBall
                        results[nearest_player]= results[nearest_player] + delta if nearest_player in results.keys() else delta
                        team = players_team[nearest_player]
                        # print("nearest_player at ts {} to ball {} is {} of team {} with dist of ".format(ts,sid,nearest_player, team, mindist))
                        results[team] = (results[team] + delta) if team in results.keys() else delta
                    lastBall = ts
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
                print(nextUpdate, results)
                nextUpdate+=T
#               print("nextUpdate is at {}".format(nextUpdate))
            if eventType == "Begin" and ts > eventTs:
                # print("cond: ", eventType, ts)
                break

if __name__ == "__main__":
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
        compute_possession(K,T,m,csv.reader(f,quoting=csv.QUOTE_NONNUMERIC),csv.reader(interruptions))
