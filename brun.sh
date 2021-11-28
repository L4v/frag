# NOTE(Jovan): A simple building script
#! /bin/bash

# NOTE(Jovan): Create a build folder if one doesn't already exist
mkdir -p build

pushd build

IM_CXXFLAGS=$(Magick++-config --cxxflags)
IM_LDFLAGS=$(Magick++-config --ldflags)
g++ -g -o frag ../code/linux_frag.cpp ../code/libs/glad.c -I../code/include `pkg-config --libs glfw3` -ldl\
    `pkg-config --libs assimp` $IM_CXXFLAGS $IM_LDFLAGS\
    && ./frag
popd
