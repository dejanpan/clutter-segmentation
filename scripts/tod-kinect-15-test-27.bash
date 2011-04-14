#!/usr/bin/env bash

if [ "$1" = "--help" ] ; then
    cat <<HELP
Usage: tod-kinect-15-test-27

Evaluates classifier trained on 15 subjects from tod kinect database using
tod_kinect_test_27 as testing set.
HELP
    exit
fi

if [ ! "$CLUTSEG_PATH" ] ; then
    echo "ERROR: Environment variable CLUTSEG_PATH is not defined."
    exit
fi

pushd $CLUTSEG_PATH > /dev/null
    rm tod_kinect_test_27/result/*
    blackbox_recognizer \
        -B tod_kinect_train_15 \
        -I tod_kinect_test_27 \
        --store=tod_kinect_test_27/result \
        --testdesc=tod_kinect_test_27/testdesc-15.txt \
        --mode=1 \
        --verbose=0 \
        -f tod_kinect_train/config.yaml
popd > /dev/null
