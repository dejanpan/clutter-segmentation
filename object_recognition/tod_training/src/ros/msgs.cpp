#include <tod/training/ros/msgs.h>
#include <opencv2/core/core.hpp>
using namespace cv;
namespace tod
{
Camera fromRosMsg(sensor_msgs::CameraInfoConstPtr camera_info)
{
  tod::Camera camera;
  camera.D = Mat(camera_info->D).clone();
  camera.K = Mat(3, 3, CV_64F, (void*)camera_info->K.elems).clone();
  camera.image_size.width = camera_info->width;
  camera.image_size.height = camera_info->height;
  camera.Kinv = camera.K.inv();
  return camera;
}
}
