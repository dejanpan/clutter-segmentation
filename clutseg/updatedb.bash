#!/usr/bin/env bash
cd $CLUTSEG_PATH
source setup.bash
roscd clutseg
rm data/test.sqlite3 
cat schema/*.sql | sqlite3 data/test.sqlite3
cat data/test.sql | sqlite3 data/test.sqlite3 