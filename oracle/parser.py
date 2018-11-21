#!/usr/bin/env python
import csv
import json

with open("metadata.txt", "r") as infile:
    player_file = open('players.csv', 'w')
    fieldnames = ("Name","Role","X","Y")
    reader = csv.DictReader( infile, fieldnames)
    out = json.dumps( [ row for row in reader ], indent=0)
    jsonfile.write(out)