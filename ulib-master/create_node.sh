#!/bin/bash
mode="666"
module="uframe"


vid=$1
pid=$2
interface=$3

type=$4
direction=$5
minor=$6

dir="/home/samir/Documents/dev/uframe/"$vid"/"$pid"/"$interface"/"
mkdir -p $dir

major=`awk "\\$2==\"$module\" {print \\$1}" /proc/devices`

echo "major : "$major
echo "minor : "$minor

group="staff"
grep -q '^staff:' /etc/group || group="wheel"

mkdir -p "$dir""$type"
sudo mknod "$dir""$type""/""$direction" c $major $minor
sudo chgrp $group "$dir""$type""/""$direction"
sudo chmod $mode "$dir""$type""/""$direction"
