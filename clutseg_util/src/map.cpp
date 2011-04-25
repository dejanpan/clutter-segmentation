/*
 * Author: Julius Adorf
 */

#include "map.h"

#include <cv.h>
#include <boost/foreach.hpp>

namespace clutseg {

    void mapToCloud(PointCloudT & keypoints3d, const vector<Point> keypoints2d,
                    const Mat & scene2d, const PointCloudT & scene3d) {
        // Find inlier points in query cloud. Note that we do not use
        // camera information here. Instead we assume that indices in query
        // cloud correspond to the 2d indices. This makes sense especially
        // when both 2d and 3d data has been taken from a Kinect camera.
        // Nevertheless, this assumption has to be verified. Also, it seems
        // that in tod_training, tod::PCLToPoints is solving the same task.
        // thing. Yet I think either my code has a bug or there code which
        // maps 2d to 3d points. This has to be investigated. See line 117
        // of clouds.h in tod_training. Why is the x-scale factor used for
        // y-scaling?
        // TODO: is this code snippet correct?
        float scaleW = float(scene3d.width) / scene2d.cols;
        float scaleH = float(scene3d.height) / scene2d.rows;
        BOOST_FOREACH(Point p, keypoints2d) {
            size_t u = size_t(p.x * scaleW);
            size_t v = size_t(p.y * scaleH);
            if (u < scene3d.width && v < scene3d.height) {
                keypoints3d.push_back(scene3d(u, v));
            } else {
                cerr << "WARNING: cannot find 3d point for inlier, outside of point cloud" << endl; 
            }
        }
    }

    void mapInliersToCloud(PointCloudT & keypoints3d, const Guess & guess,
                    const Mat & scene2d, const PointCloudT & scene3d) {
        vector<Point> inliers(guess.inliers.size());
        BOOST_FOREACH(int idx, guess.inliers) {
            inliers.push_back(guess.image_points_[idx]);
        }
        mapToCloud(keypoints3d, inliers, scene2d, scene3d);
    }   
}

