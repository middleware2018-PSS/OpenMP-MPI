#!/usr/bin/env bash
mkdir -p results
for T in `seq 1 60`;
do
  for K in `seq 1 5`;
  do
    echo ${K}m ${T}s : >> results/times.txt
    sudo time docker run -v $PWD/data:/data openmp $K $T . > results/result_${K}m_${T}s.txt 2>> results/times.txt;
  done;
done
