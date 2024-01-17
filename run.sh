#!/bin/bash

#docker run -it --ipc=host ipc_test:latest
docker run -it -v /dev/shm:/dev/shm ipc_test:latest
