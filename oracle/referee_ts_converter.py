#!/usr/bin/env python
import sys
def conv(time):
    t = time.strip().split(":")
    if len(t)>1:
        h, m, s = [float(el) for el in t]
        return int((h*3600 + m*60 + s)*10**12)
    else:
        return 0

offset = int(sys.argv[1])
for line in sys.stdin:
    l = line.strip().split(",")
    l[2] = str(conv(l[2])+offset)
    print(",".join(l))
    

