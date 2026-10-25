#!/bin/bash

cd ..

cmake --preset linux-release -DCMAKE_BUILD_TYPE=Debug -G "CodeLite - Ninja"
