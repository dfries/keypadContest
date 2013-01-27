#!/bin/ash

s0="http://ice.somafm.com/groovesalad"
s1="http://ice.somafm.com/lush"
s2="http://ice.somafm.com/poptron"
s3="http://ice.somafm.com/illstreet"
s4="http://ice.somafm.com/covers"
s5="http://ice.somafm.com/u80s"
s6="http://ice.somafm.com/secretagent"
s7="http://ice.somafm.com/illstreet"
s8="http://192.168.0.101:8000/"

stty -F /dev/ttyUSB0 38400

while /bin/true;
do
        L=`head -n 1 /dev/ttyUSB0`
        echo "Received: ${L}"
        s=""
        if [ "$L" == "P0" ]; then
                s="${s0}";
        elif [ "$L" == "P1" ]; then
                s="${s1}";
        elif [ "$L" == "P2" ]; then
                s="${s2}";
        elif [ "$L" == "P3" ]; then
                s="${s3}";
        elif [ "$L" == "P4" ]; then
                s="${s4}";
        elif [ "$L" == "P5" ]; then
                s="${s5}";
        elif [ "$L" == "P6" ]; then
                s="${s6}";
        elif [ "$L" == "P7" ]; then
                s="${s7}";
        elif [ "$L" == "P8" ]; then
                s="${s8}";
        elif [ "$L" == "P9" ]; then
                mpc stop;
        fi
        if [ "$s" != "" ]; then
                echo "command: ${s}"
                mpc stop;
                mpc clear;
                mpc add $s;
                mpc play;
        fi
done

