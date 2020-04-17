@echo off
SET BASEDIR="%cd%"
rmdir /q /s build
mkdir build
cd utils/win32
call docker-build.bat
cd src
docker run --rm -v %BASEDIR%/build:/home/mcsc/build mcsc-ui:latest
cd ..