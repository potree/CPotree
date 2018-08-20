#!/bin/bash -e

sudo apt-get update
sudo apt-get install -y build-essential libbsd0 libbsd-dev

# Clean
rm -f *.o && rm -rf build
mkdir -p build

./scripts/make-package.sh
