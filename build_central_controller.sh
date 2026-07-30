#!/usr/bin/env bash
set -e

cmake -DCMAKE_BUILD_TYPE=Release -DZT1_CENTRAL_CONTROLLER=1 -DCMAKE_INSTALL_PREFIX=$PWD -S . -B build/ -DCMAKE_INSTALL_PREFIX=$(shell pwd)/build-out
cmake --build build/ --target all -j4 --verbose

ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    ARCH="amd64"
elif [ "$ARCH" = "aarch64" ]; then
    ARCH="arm64"
fi

if [ -z "$TARGET_DOCKER_REPO" ]; then
    TARGET_DOCKER_REPO="us-central1-docker.pkg.dev/zerotier-421eb9/docker-images"
fi

docker build -platform linux/$ARCH -t $TARGET_DOCKER_REPO/ztcentral-controller:$(git rev-parse --short HEAD)-$ARCH -f ext/central-controller/docker/Dockerfile.central-controller . --load --push