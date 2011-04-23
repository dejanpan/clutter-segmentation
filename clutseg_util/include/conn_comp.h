/*
 * Author: Julius Adorf
 */

#include <cv.h>

using namespace cv;

/** Finds the largest connected components and fills all other connected
 * components with background color. Background is black and foreground is
 * white. See method cv::findContours for what type of images are accepted. */
void largestConnectedComponent(Mat& img);

