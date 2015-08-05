#!/usr/bin/env sh

URL=http://downloads.zdaemon.org/wads/doom1.zip

mkdir dooms
cd dooms

wget ${URL}
unzip doom1.zip

../mapgrab.py doom1.wad E1M1
../mapgrab.py doom1.wad E1M2
../mapgrab.py doom1.wad E1M3
../mapgrab.py doom1.wad E1M4
../mapgrab.py doom1.wad E1M5
../mapgrab.py doom1.wad E1M6
../mapgrab.py doom1.wad E1M7
../mapgrab.py doom1.wad E1M8
../mapgrab.py doom1.wad E1M9
