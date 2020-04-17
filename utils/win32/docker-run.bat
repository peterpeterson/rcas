@echo off
cd ../..
docker run -p 8000:8000 -d mcsc-ui:latest
docker cp mcsc-ui:/build.zip .
