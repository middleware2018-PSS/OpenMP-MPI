
build:
	sudo docker build -f src/Dockerfile -t openmp src
make run:
	sudo docker run -v $$PWD/data:/data openmp ./openmpi 3 30 .
