#!/bin/bash

# Remove all Docker containers
docker ps -aq | xargs docker rm

# Remove all Docker images
docker images -q | xargs docker rmi