# NOTE(Jovan): A simple building script
#! /bin/bash

# NOTE(Jovan): Create a build folder if one doesn't already exist
mkdir -p build

pushd build
g++ -o frag ../code/*.cpp ../code/libs/glad.c -I../code/include `pkg-config --libs glfw3` -ldl\
    && ./frag
popd
