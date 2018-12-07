#!/usr/bin/env bash
mkdir -p results
M=200
for T in `seq 10 10 60`;
do
  for K in `seq 1 5`;
  do
    echo serial,${K}m,${T}s: >> results/times.txt
    (time ./serial $K $T data $M) > results/result_${K}m_${T}s_${M}_serial.txt 2>> results/times.txt;
    echo parallel,${K}m,${T}s: >> results/times.txt
    (time ./parallel $K $T data $M) > results/result_${K}m_${T}_${M}_parallel.txt 2>> results/times.txt;
  done;
done
