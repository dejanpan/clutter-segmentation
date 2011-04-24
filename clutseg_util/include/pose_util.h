/*
 * Author: Julius Adorf
 */

#include <cv.h>
#include <opencv_candidate/PoseRT.h>
#include <opencv_candidate/Camera.h>

using namespace cv;
using namespace opencv_candidate;

namespace clutseg {

    void poseToPoseRT(const Pose & src, PoseRT & dst);

    void poseRtToPose(const PoseRT & src, Pose & dst);

    // TODO: use template or remove one of these methods
    void writePose(const string & filename, const PoseRT & pose);

    void readPose(const string & filename, PoseRT & dst);

    void writePose(const string & filename, const Pose & pose);

    void readPose(const string & filename, Pose & dst);

}

