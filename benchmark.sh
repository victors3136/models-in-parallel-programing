#!/bin/bash
if [ $# -ne 1 ]; then
    echo "usage: benchmark <N>";
    exit 1;
fi;

dir="data";
N=$1;
fmt="bin"
./sequential $N "$dir/$N.A.$fmt" "$dir/$N.B.$fmt" "$dir/$N.out.$fmt" > /dev/null
for run in {1..10}; do
    ./sequential $N "$dir/$N.A.$fmt" "$dir/$N.B.$fmt" "$dir/$N.out.$fmt";
done;
