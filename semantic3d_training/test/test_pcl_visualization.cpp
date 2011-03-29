/* 
 * Author: Julius Adorf
 */

#include "test.h"

#include <gtest/gtest.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl_visualization/pcl_visualizer.h>
#include <cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv_candidate/PoseRT.h>
#include <boost/format.hpp>
#include <tod/detecting/Tools.h>

// TODO: move to own namespace
#include "rotation.h"
#include "pcl_visualization_addons.h"

using namespace pcl;
using namespace pcl_visualization;
using namespace cv;
using namespace opencv_candidate;

TEST(PclVisualization, ShowPointCloud) {
    // TODO: Getting X error when not calling visualizer.spin()
    if (enableUI) {
        PointCloud<PointXYZ> cloud;
        PointCloud<PointXYZ> cloud2;
        io::loadPCDFile("./data/sample.delimited.pcd", cloud);
        io::loadPCDFile("./data/cloud_00000.pcd", cloud2);
        PCLVisualizer visualizer;
        visualizer.addPointCloud(cloud, "cloud1");
        visualizer.addPointCloud(cloud2, "cloud2");
        visualizer.addCoordinateSystem(1, 0, 0, 0);
        visualizer.spin();
    }
}

TEST(PclVisualization, ShowCircle) {
    // TODO: Getting X error when not calling visualizer.spin()
    if (enableUI) {
        PCLVisualizer visualizer;
        PointXYZ origin;
        origin.x = 0;
        origin.y = 0;
        origin.z = 0;
        visualizer.addSphere(origin, 1, 0, 0, 1, "sphere1");
        visualizer.addSphere(origin, 2, 0, 1, 0, "sphere2");
        visualizer.addSphere(origin, 3, 1, 0, 0, "sphere3");
        visualizer.addCoordinateSystem(1, 0, 0, 0);
        visualizer.spin();
    }
}

TEST(PclVisualization, ShowPoseEstimate) {
    // TODO: Getting X error requires call to visualizer.spin()
    if (enableUI) {
        PCLVisualizer visualizer;
        PointXYZ tvec;
        tvec.x = 1;
        tvec.y = -0.25;
        tvec.z = -0.1;
        PointXYZ rvec;
        rvec.x = 1;
        rvec.y = -0.25;
        rvec.z = -0.1;
        PointXYZ origin;
        origin.x = 0;
        origin.y = 0;
        origin.z = 0;
        visualizer.addCoordinateSystem(1, 0, 0, 0);
        visualizer.addCoordinateSystem(0.2, tvec.x, tvec.y, tvec.z);
        visualizer.addLine(origin, tvec, 0.5, 0.5, 1);
        PointCloud<PointXYZ> cloud;
        io::loadPCDFile("./data/sample.delimited.pcd", cloud);
        visualizer.addPointCloud(cloud);
        visualizer.spin();
    }
}

// ||rvec|| is the angle, and rvec is the axis of rotation
TEST(PclVisualization, ShowFiducialPoseEstimate) {
    // TODO: Getting X error when not calling visualizer.spin()
    if (enableUI) {
        // Load pose estimation from yaml file
        FileStorage fs("./data/fat_free_milk_image_00000.png.pose.yaml", FileStorage::READ);
        PoseRT pose;
        pose.read(fs[PoseRT::YAML_NODE_NAME]);
        // Load point cloud
        PointCloud<PointXYZ> cloud;
        io::loadPCDFile("./data/fat_free_milk_cloud_00000.pcd", cloud);
        // Create visualizer
        PCLVisualizer visualizer;
        // Add coordinate system
        visualizer.addCoordinateSystem(0.5, 0, 0, 0);
        // Add point cloud
        visualizer.addPointCloud(cloud);
        // Draw pose
        addPose(visualizer, pose);
        visualizer.spin();
    }
}

TEST(PclVisualization, ShowInvertedPose) {
    // TODO: Getting X error when not calling visualizer.spin()
    if (enableUI) {
        // Load pose estimation from yaml file
        FileStorage fs("./data/fat_free_milk_image_00000.png.pose.yaml", FileStorage::READ);
        PoseRT pose;
        pose.read(fs[PoseRT::YAML_NODE_NAME]);
        PoseRT invPose = tod::Tools::invert(pose);
        // Load point cloud
        PointCloud<PointXYZ> cloud;
        io::loadPCDFile("./data/fat_free_milk_cloud_00000.pcd", cloud);
        // Create visualizer
        PCLVisualizer visualizer;
        // Add coordinate system
        visualizer.addCoordinateSystem(0.5, 0, 0, 0);
        // Add point cloud
        visualizer.addPointCloud(cloud);
        // Draw pose
        addPose(visualizer, pose, "standard");
        // Draw pose
        addPose(visualizer, invPose, "inverted");
        visualizer.spin();
    }
}


TEST(PclVisualization, ShowAllPoses) {
    // TODO: Getting X error when not calling visualizer.spin()
    if (enableUI) {
        PCLVisualizer visualizer;
        visualizer.addCoordinateSystem(0.5, 0, 0, 0);
        PoseRT pose;
        for (int i = 0; i < 76; i++) {
            FileStorage fs(str(boost::format("./data/pose/image_%05i.png.pose.yaml") % i), FileStorage::READ);
            pose.read(fs[PoseRT::YAML_NODE_NAME]);
            addPose(visualizer, pose, str(boost::format("pose-%05i-") % i));
        }    
        visualizer.spin();
    }
}

