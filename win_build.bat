@echo off
SET BASEDIR="%cd%"
rem rmdir /q /s build
mkdir build
cd utils/win32
call docker-build.bat
cd src
docker run --rm -v %BASEDIR%/build:/home/rcas/build rcas-ui:latest
cd ..