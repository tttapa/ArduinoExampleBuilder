#!/usr/bin/env bash

cd "$(dirname "$0")"

docker build . -t arduino-example-builder

container=$(docker create arduino-example-builder)
docker cp $container:/tmp/build/release/bin/arduino-example-builder build/release/bin
docker rm $container
