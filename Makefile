all:
	g++ main.cpp `sdl2-config --cflags --libs` -lX11 -lXi -O3 -o main

