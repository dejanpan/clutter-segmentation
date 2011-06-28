/**
 * A ROS node that estimates the pose of one rigid textured object.
 * This ROS node subscribes to 
 *      1) an input point cloud
 *      2) a corresponding camera image
 * and subsequently publishes 
 *      1) the inliers point cloud belonging to the rigid textured object. 
 *      2) the name of the recognized object
 *      3) the estimated pose (6dof) of the recognized object
 *
 * See also:
 *      object_recognition/tod_detecting/apps/recognition_node.cpp
 */

#include "clutseg/clutseg.h"
#include "clutseg/result.h"
#include "clutseg/viz.h"

#include "clutseg/gcc_diagnostic_disable.h"

#include <cv.h>
#include <cv_bridge/cv_bridge.h>
#include <geometry_msgs/Pose.h>
#include <image_transport/image_transport.h>
#include <opencv_candidate/Camera.h>
#include <opencv_candidate/PoseRT.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <pcl/io/pcd_io.h>
#include <ros/ros.h>
#include <sensor_msgs/image_encodings.h>
#include <sensor_msgs/PointCloud2.h>
#include <std_msgs/String.h>

using namespace std;

int main(int argc, char **argv) {
    ros::init(argc, argv, "clutsegmenter");
    ros::NodeHandle n;

    ros::Publisher inliers_publisher = n.advertise<sensor_msgs::PointCloud2>("clutseg_inliers", 1);
    ros::Publisher pose_publisher = n.advertise<geometry_msgs::Pose>("clutseg_pose", 1);
    ros::Publisher object_publisher = n.advertise<std_msgs::String>("clutseg_object", 1);

    // This one is just for debugging purposes. It publishes a kind of heads-up display 
    // showing pose and inliers.
    image_transport::ImageTransport img_transp(n);
    image_transport::Publisher hud_publisher = img_transp.advertise("clutseg_hud", 1);

    // TODO: define subscriptions 
    // ...
    // ...

    ros::Rate loop_rate(0.1);

    // TODO: you need to place <modelbase>.tar somewhere
    // TODO: you need to tell the node where to find <modelbase>.tar 
    clutseg::Clutsegmenter sgm("./data/orb.tar", true);
    cv::namedWindow("hud");

    while (ros::ok()) {
        // 1. Read input from topics 
        // --------------------------------------------------------------------
        clutseg::PointCloudT query_cloud;
        cv::Mat query_image;
        cv::Mat query_image_bgr8;

        // TODO: get query_image from subscribed topic, if color image is not
        // available, grayscale is okay though debugging output will not be as nice.
        query_image_bgr8 = cv::imread("./data/image_00000.png");
        query_image_bgr8.convertTo(query_image, CV_8UC1);
        // TODO: get query_cloud from subscribed topic
        pcl::io::loadPCDFile("./data/cloud_00000.pcd", query_cloud);
        // TODO: get camera from subscribed topic
        opencv_candidate::Camera camera = opencv_candidate::Camera("./data/camera.yml", opencv_candidate::Camera::TOD_YAML);

        // 2. Recognize
        // --------------------------------------------------------------------
        clutseg::ClutsegQuery query(query_image, query_cloud);
        clutseg::Result result;
        sgm.recognize(query, result);

        // 3. Publish to topics
        // --------------------------------------------------------------------
        sensor_msgs::PointCloud2 inliers_msg;
        geometry_msgs::Pose pose_msg;
        std_msgs::String object_msg;

        clutseg::PointCloudT inliers = result.refine_choice.inlierCloud;
        opencv_candidate::PoseRT pose = result.refine_choice.aligned_pose();
        string object = result.refine_choice.getObject()->name;

        inliers_publisher.publish(inliers_msg);
        pose_publisher.publish(pose_msg);
        object_publisher.publish(object_msg);

        {
            // Just for debugging
            cv::Mat hud = query_image_bgr8.clone();
            clutseg::drawGuess(hud, result.refine_choice, camera, opencv_candidate::PoseRT());

            cv_bridge::CvImage cv_hud;
            cv_hud.encoding = sensor_msgs::image_encodings::BGR8;
            cv_hud.image = hud;

            // cv::imshow("hud", cv_hud.image);
            // cv::waitKey(-1);

            hud_publisher.publish(cv_hud.toImageMsg());
        } 

        ros::spinOnce();
        loop_rate.sleep();
    }
}

