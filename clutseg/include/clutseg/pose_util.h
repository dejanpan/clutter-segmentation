/*
 * Author: Julius Adorf
 */

#ifndef _POSE_UTIL_H_
#define _POSE_UTIL_H_

#include <cv.h>
#include <opencv_candidate/PoseRT.h>
#include <opencv_candidate/Camera.h>

namespace clutseg {

    cv::Point projectOrigin(const opencv_candidate::PoseRT & pose, const opencv_candidate::Camera & camera);

    void randomizePose(opencv_candidate::PoseRT & pose, double stddev_t, double stddev_r);

    void poseToPoseRT(const opencv_candidate::Pose & src, opencv_candidate::PoseRT & dst);

    void poseRtToPose(const opencv_candidate::PoseRT & src, opencv_candidate::Pose & dst);

    // TODO: use template or remove one of these methods
    void writePose(const std::string & filename, const opencv_candidate::PoseRT & pose);

    void readPose(const std::string & filename, opencv_candidate::PoseRT & dst);

    void writePose(const std::string & filename, const opencv_candidate::Pose & pose);

    void readPose(const std::string & filename, opencv_candidate::Pose & dst);

    void modelToView(const cv::Mat & mvtrans, const cv::Mat & mvrot, const cv::Mat & mpt, cv::Mat & vpt);

    void modelToView(const opencv_candidate::PoseRT & pose, const cv::Point3d & mpt, cv::Point3d & vpt);

    void translatePose(const opencv_candidate::PoseRT & src, const cv::Mat & model_tvec, opencv_candidate::PoseRT & dst);

}

#endif