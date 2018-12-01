FROM ubuntu:bionic AS build
RUN apt update && apt upgrade -y && apt install -y build-essential cmake g++
COPY . /src
WORKDIR /src
RUN cmake . && make all

FROM scratch
WORKDIR /
ENV OMP_WAIT_POLICY active
ENV OMP_DYNAMIC false
ENV OMP_PROC_BIND true
COPY --from=build /src/openmpi ./
ENTRYPOINT ["/openmpi"]
