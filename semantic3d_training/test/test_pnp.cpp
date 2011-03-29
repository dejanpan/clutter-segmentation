/**
 * Author: Julius Adorf
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <posest/pnp_ransac.h>

TEST(PNP, CallSolvePnPRansac) {
    solvePnPRansac(
        objectPoints,
        imagePoints,
        f3d.camera().K,
        f3d.camera().D,
        rvec,
        tvec,
        false,
        params.ransacIterationsCount,
        params.maxProjectionError,
        -1,
        &inliers);
}

