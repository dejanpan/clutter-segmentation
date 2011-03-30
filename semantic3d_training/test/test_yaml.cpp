/*
 * Author: Julius Adorf
 */

#include "test.h"

#include <gtest/gtest.h>
#include <cv.h>
#include <opencv_candidate/PoseRT.h>
#include <boost/filesystem.hpp>

using namespace cv;

TEST(Yaml, ReadYaml) {
    FileStorage fs("./data/config.yaml", FileStorage::READ);
    EXPECT_TRUE(fs.isOpened());
}

/** Read YAML file using OpenCV */
TEST(Yaml, ExtractDetectorType) {
    FileStorage fs("./data/config.yaml", FileStorage::READ);
    EXPECT_EQ("DynamicFAST", string(fs["TODParameters"]["feature_extraction_params"]["detector_type"]));
}

/** Read pose estimation from YAML file */
TEST(Yaml, DeserializePoseFromYAML) {
    using namespace opencv_candidate;
    FileStorage in("./data/pose.yaml", FileStorage::READ);
    PoseRT act_pose;
    act_pose.read(in[PoseRT::YAML_NODE_NAME]);
    EXPECT_EQ(2.0, act_pose.rvec.at<double>(0, 0));
    EXPECT_EQ(4.0, act_pose.rvec.at<double>(1, 0));
    EXPECT_EQ(8.0, act_pose.rvec.at<double>(2, 0));
    EXPECT_EQ(16.0, act_pose.tvec.at<double>(0, 0));
    EXPECT_EQ(32.0, act_pose.tvec.at<double>(1, 0));
    EXPECT_EQ(64.0, act_pose.tvec.at<double>(2, 0));
}

/** Serialize pose estimation to YAML file */
TEST(Yaml, SerializePoseToYAML) {
    using namespace opencv_candidate;
    using namespace boost::filesystem;
    PoseRT pose;
    pose.rvec = Mat::zeros(3, 1, 1); 
    pose.tvec = Mat::ones(3, 1, 1);

    FileStorage out("./data/pose.out.yaml", FileStorage::WRITE);
    out << PoseRT::YAML_NODE_NAME;
    pose.write(out);

    remove("./data/pose.out.yaml");
}
