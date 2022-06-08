#!/bin/bash

x86_64-w64-mingw32-g++.exe -s -O2 -std=c++17 -o Program -Wall -Wextra -Wpedantic main.cpp -luser32 -lgdi32 -lgdiplus -lopengl32 -lSHlwapi -ldwmapi -lstdc++fs -lwinmm -Wl,-subsystem,windows
