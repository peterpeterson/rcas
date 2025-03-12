#!/bin/bash

BASEDIR=$(pwd)
rm -rf build
mkdir -p build
cd utils/mac
./docker-build.sh
cd ../../src
docker run --rm -v "$BASEDIR/build:/home/rcas/build" rcas-ui:latest
cd ..