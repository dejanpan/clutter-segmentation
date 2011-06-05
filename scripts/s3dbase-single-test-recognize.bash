#!/usr/bin/env bash

if [ "$1" = "--help" ] ; then
    cat <<HELP
Usage: s3dbase-single-test-recognize.bash

Runs the recognizer on an image that shows the single object the classifier
knows from the training process.
HELP
    exit
fi

source ~/.env
if [ ! "$CLUTSEG_PATH" ] ; then
    echo "ERROR: Environment variable CLUTSEG_PATH is not defined."
    exit
fi

pushd $CLUTSEG_PATH/s3dbase-single > /dev/null
    rosrun tod_detecting recognizer --base=. --tod_config=./config.yaml --image=./icetea2/image_00001.png --verbose=2 --mode=1
popd > /dev/null

