/* 
 * Author: Julius Adorf
 */

#include <gtest/gtest.h>
#include <limits>
#include "pcl/io/pcd_io.h"
#include "pcl/point_types.h"
#include "cv.h"
#include <opencv2/highgui/highgui.hpp>

#include "fiducial/fiducial.h"
#include "tod/core/Features3d.h"
#include "tod/training/masking.h"
#include "tod/training/clouds.h"
#include "tod/training/Opts.h"
#include "opencv_candidate/Camera.h"


using namespace std;
using namespace pcl;

/** Tests whether a point cloud file from kinect training data set can be
 * read using routines from 'pcl'. See http://vault.willowgarage.com/wgdata1/vol1/tod_kinect_bags/training */
TEST(PCL, ReadKinectPointCloud) {
    PointCloud<PointXYZRGB> cloud;
    io::loadPCDFile("./data/cloud_00000.pcd", cloud);
}

/** Tests whether a point cloud file from TUM/IAS semantic 3d data set can be
 * read using routines from 'pcl'. Also have a look at the data itself. */
TEST(PCL, ReadSemantic3dPointCloud) {
    PointCloud<PointXYZ> cloud;
    io::loadPCDFile("./data/sample.delimited.pcd", cloud);
}

/** Test how points can be projected onto two out of three coordinates. Use the
 * characteristically shaped icetea2 object and project the y and z coordinates
 * onto a picture. */
TEST(PCL, CoordinateProjection) {
    using namespace cv;
    // TODO: remove platform dependency
    typedef unsigned char uint8;
    PointCloud<PointXYZ> cloud;
    io::loadPCDFile("./data/sample.delimited.pcd", cloud);

    // find y and z range
    float ymin = numeric_limits<float>::max();
    float ymax = numeric_limits<float>::min();
    float zmin = numeric_limits<float>::max();
    float zmax = numeric_limits<float>::min();
    PointCloud<PointXYZ>::iterator it = cloud.begin();
    PointCloud<PointXYZ>::iterator end = cloud.end();
    while (it != end) {
        ymin = min(ymin, it->y);
        ymax = max(ymax, it->y);
        zmin = min(zmin, it->z);
        zmax = max(zmax, it->z);
        it++;
    }
    // create image and scale of 2d coordinate axes
    float scale = 2000;
    int r = int(scale * (ymax - ymin)) + 1;
    int c = int(scale * (zmax - zmin)) + 1;
    Mat img = Mat::zeros(r, c, CV_8U);
    
    // project points onto image
    it = cloud.begin();
    while (it != end) {
        img.at<uint8>(r - int(scale * (it->y - ymin)) - 1, int(scale * (it->z - zmin))) = 255;
        it++;
    }

    // show image
    namedWindow("TEST(PCL, CoordinateProjection)"); 
    imshow("TEST(PCL, CoordinateProjection)", img);
    waitKey(5000);
}

/** Test how to create a mask by doing a perspective projection */
TEST(PCL, PerspectiveProjection) {
    fiducial::KnownPoseEstimator pose_est("./data/fat_free_milk_image_00000.png.pose.yaml");
    cv::Mat colorimg = cv::imread("./data/fat_free_milk_image_00000.png", CV_LOAD_IMAGE_COLOR);
    tod::Camera camera = tod::Camera("./data/fat_free_milk_camera.yml", opencv_candidate::Camera::TOD_YAML);
    tod::Features2d f2d(camera, colorimg);
    f2d.camera.pose = pose_est.estimatePose(cv::Mat());
    PointCloud<PointXYZRGB> cloud;
    io::loadPCDFile("./data/fat_free_milk_cloud_00000.pcd", cloud);
    f2d.mask = tod::cloudMask(cloud, f2d.camera.pose, camera);
    cv::Mat colorMask;
    cv::cvtColor(f2d.mask, colorMask, CV_GRAY2BGR);
    cv::imshow("mask", f2d.image & colorMask);
    cv::waitKey(5000);
}

