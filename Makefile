build:
	sudo docker build -f src/Dockerfile -t openmp src

run: build
	sudo docker run -v $$PWD/data:/data openmp ./openmpi 3 30 .

generate_data:
	mv data/full-game.csv data/full-game.csv.bkup
	./merge_referee_sensors.sh > data/full-game.csv

