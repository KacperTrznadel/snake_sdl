#!/bin/bash

g++ -O2 -w -I./sdl/include -L. -o main main.cpp -lm -lSDL2 -lpthread -ldl -lrt
./main