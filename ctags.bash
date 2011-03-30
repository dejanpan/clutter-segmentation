#!/usr/bin/env bash
# Creates tags for diamondback sources, clutter-segmentation, stack
# object_recognition, and dependencies of tod_training and tod_detecting.
if [ ! "$CLUTSEG_PATH" ] ; then
    echo "ERROR: Environment variable CLUTSEG_PATH is not defined."
    exit
fi
paths="/opt/ros/diamondback $CLUTSEG_PATH"
echo "Generating tags in paths $paths ..."
ctags --recurse $paths
echo "Finished. Add the following line to your .vimrc file to always include"
echo "the generated tag file when opening vim:"
echo ""
echo "    set tags+=\$CLUTSEG_PATH/clutter-segmentation/tags    "
echo ""
