#!/bin/bash

set -e

echo "Building aligner..."
cd src/aligner
make
echo "Done."

echo "Building fastUtils..."
cd ../preprocessing/utils
make
echo "Done."

cd ../..

echo "Installation complete. You may now use LRCstats."
