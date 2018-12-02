#!/usr/bin/env bash
mkdir -p results
for T in `seq 1 60`;
do
  for K in `seq 1 5`;
  do
    sudo docker run -v $PWD/data:/data openmp $K $T . | tee results/result_${K}m_${T}s.txt;
  done;
done
