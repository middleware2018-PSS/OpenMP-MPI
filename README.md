# OpenMP/MPI Project

Project by Lorenzo Petrangeli, Tommaso Sardelli and Philippe Scorsolini for the course of "Middleware Technologies for distributed systems" 2018 at Politecnico di Milano held by Professors Sam Guinea and Luca Mottola.

# Dataset

## General Info:

- Players: 1 sensor per leg (2 total, 200Hz)
- Goal keeper: 1 sensor per leg or arm (4 total, 200Hz)
- Ball: 
  - 1 sensor (200KHz)
  - 3 balls in the first half
  - 4 balls in the second half
  - only one used while playing
- Game start: `10753295594424116`
- Game end: `14879639146403495`
- Half field used
- Played 2 halves of 30 min each
- All timestamped inputs are ingested in increasing order w.r.t. timestamps

## Sensors' Stream (full-game, 4.3 GB):

CSV formatted: `sid, ts, x, y, z, |v|, |a|, vx, vy, vz, ax, ay, az`
Where:
- `sid`: sensor id
- `ts`: timestamp
- `x`, `y`, `z`: 3D coordinates w.r.t. the origin in the center of the field [`mm`]
- other can be ignored

## Referee Events:
- Game Interruption (7.9KB):
  - timestamp pause and resume of game
- Ball Possession (93.3KB): Approximate ball possession statistics, created manually and can serve as an aid in validating the results
- Shot on Goal (2.6KB): Ignored

## Metadata (metadata.txt 920 B):
- Team composition
- Sensors' assignment to players and referee
- Balls used for each half of the game

# Task
Using MPI and/or OpenMP you are to create a software that computes the real-time statistics of ball possession during the game.
## Assumptions
- A player is considered in possession of the ball when:
  - He is the player closest to the ball
  - He is not farther than K meters from the ball
- Ball possession is undefined whenever the game was paused
- T from 1 to 60
- K from 1 to 50
- The statistics need to be output for every T time units as the game unfolds
- The statistics accumulate every T time units

## Software specifications:
- Input:
  - K from 1 to 5
  - T from 1 to 60
- Output:
  - a string every T units of play, arbitrarily formatted, with ball possession statistics:
    - for each player
    - for the whole team
  - to stdout or file

## To be turned in:
- complete codebase
- a max 3 page document illustrating design choices

# Refs
- Data:
  - https://aerofs.neslab.it/l/0b15ed33250349c199c37c85a7408dc9
  - https://aerofs.neslab.it/l/a47fd7abc87b4b73b2c409d8fa18d5ad
  - https://aerofs.neslab.it/l/ec01789e77bc4f97a63e3f77009a30fb
  - https://aerofs.neslab.it/l/f7b889e2a2b74925a8e5a515b06b21a8
  - https://aerofs.neslab.it/l/54d2d70b25474e82845598e47f3ba7a8

