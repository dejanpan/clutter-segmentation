/*
 * Author: Julius Adorf
 */

#include "viz.h"

#include <boost/foreach.hpp>

using namespace cv;
using namespace opencv_candidate;

namespace clutseg {

    void drawKeypoints(Mat & canvas, const vector<KeyPoint> & keypoints,
                        const Scalar & color) { 
        // Use OpenCV's drawKeypoints method DrawMatchesFlags::DRAW_OVER_OUTIMG
        // specifies that keypoints are simply drawn onto existing content of
        // the output canvas. The first image parameter of drawKeypoints is
        // then not used by drawKeypoints. See also
        // https://code.ros.org/svn/opencv/trunk/opencv/modules/features2d/src/draw.cpp
        cv::drawKeypoints(canvas, keypoints, canvas, color,
                            DrawMatchesFlags::DRAW_OVER_OUTIMG);
    }

    void drawInliers(Mat & canvas, const Guess & guess, const Scalar & color) {
        vector<KeyPoint> kpts;
        for (size_t i = 0; i < guess.inliers.size(); i++) {
            Point2f kp = guess.image_points_[guess.inliers[i]];
            kpts.push_back(KeyPoint(kp, 0));
        }
        clutseg::drawKeypoints(canvas, kpts, color);
    }

    void drawPose(Mat & canvas, const PoseRT & pose, const Camera & camera,
                const Scalar & colorX, const Scalar & colorY, const Scalar & colorZ,
                const string & labelX, const string & labelY, const string & labelZ) {
        // see fiducial::PoseDrawer(canvas, camera.K, pose);
        // most of the code has been copied from that function
        Point3f z(0, 0, 0.25);
        Point3f x(0.25, 0, 0);
        Point3f y(0, 0.25, 0);
        Point3f o(0, 0, 0);
        vector<Point3f> op(4);
        op[1] = x, op[2] = y, op[3] = z, op[0] = o;
        vector<Point2f> ip;
        projectPoints(Mat(op), pose.rvec, pose.tvec, camera.K, Mat(), ip);

        vector<Scalar> c(4); //colors
        c[0] = Scalar(255, 255, 255);
        c[1] = colorX; //Scalar(255, 0, 0);//x
        c[2] = colorY; //Scalar(0, 255, 0);//y
        c[3] = colorZ; //Scalar(0, 0, 255);//z
        line(canvas, ip[0], ip[1], c[1]);
        line(canvas, ip[0], ip[2], c[2]);
        line(canvas, ip[0], ip[3], c[3]);
        /* see question on answers.ros.org
        string scaleText = "scale 0.25 meters";
        int baseline = 0;
        Size sz = getTextSize(scaleText, CV_FONT_HERSHEY_SIMPLEX, 1, 1, &baseline);
        rectangle(canvas, Point(10, 30 + 5), Point(10, 30) + Point(sz.width, -sz.height - 5), Scalar::all(0), -1);
        putText(canvas, scaleText, Point(10, 30), CV_FONT_HERSHEY_SIMPLEX, 1.0, c[0], 1, CV_AA, false);
        */
        putText(canvas, labelZ, ip[3], CV_FONT_HERSHEY_SIMPLEX, 0.5, c[3], 1, CV_AA, false);
        putText(canvas, labelY, ip[2], CV_FONT_HERSHEY_SIMPLEX, 0.5, c[2], 1, CV_AA, false);
        putText(canvas, labelX, ip[1], CV_FONT_HERSHEY_SIMPLEX, 0.5, c[1], 1, CV_AA, false);
    }

    void drawPose(Mat & canvas, const Pose & pose, const Camera & camera,
                const Scalar & colorX, const Scalar & colorY, const Scalar & colorZ,
                const string & labelX, const string & labelY, const string & labelZ) {
        PoseRT posert;
        Mat R;
        eigen2cv(pose.t(), posert.tvec);
        eigen2cv(pose.r(), R);
        Rodrigues(R, posert.rvec);
        drawPose(canvas, posert, camera, colorX, colorY, colorZ, labelX, labelY, labelZ);
    }

    Rect drawText(Mat & outImg, const vector<string> & lines,
                        const Point & topleft, int fontFace, double fontScale,
                        const Scalar & color) {
        int baseline = 0;
        // bottom right corner, is "pushed" down and right as appropriate
        Point br = topleft;
        // bottom left corner, is "pushed" down as appropriate
        Point bl;
        bl.x = topleft.x;
        for (size_t i = 0; i < lines.size(); i++) {
            Size sz = getTextSize(lines[i], fontFace, fontScale, 1, &baseline);
            // calculate in absolute frame, avoid to cumulate errors
            bl.y = topleft.y + 1.7*(i+1)*sz.height;
            putText(outImg, lines[i], bl, fontFace, fontScale, color, 1, CV_AA, false);
            br.y = bl.y;
            if (bl.x + sz.width > br.x) {
                br.x = bl.x + sz.width;
            }
        }
        return Rect(topleft, br);
    }

    void drawAllMatches(Mat & canvas, const TrainingBase & base,
                            const Ptr<Matcher> matcher, const Mat& testImage,
                            const KeypointVector & testKeypoints, const string & baseDirectory) {
      namedWindow("matches", CV_WINDOW_KEEPRATIO);
      vector<Mat> match_images;
      int scaled_width = 1000;
      int scaled_height = 0;

      // Build the individual matches
      for (size_t objectInd = 0; objectInd < base.size(); objectInd++)
      {
        for (size_t imageInd = 0; imageInd < base.getObject(objectInd)->observations.size(); imageInd++)
        {
          tod::Matches imageMatches;
          matcher->getImageMatches(objectInd, imageInd, imageMatches);
          if (imageMatches.size() > 7)
          {
            Features3d f3d = base.getObject(objectInd)->observations[imageInd];
            Features2d f2d = f3d.features();
            if (f2d.image.empty())
            {
              string filename = baseDirectory + "/" + base.getObject(objectInd)->name + "/" + f2d.image_name;
              f2d.image = imread(filename, 0);
            }
            Mat matchesView;
            drawMatches(testImage, testKeypoints, f2d.image,
                            base.getObject(objectInd)->observations[imageInd].features().keypoints, imageMatches,
                            matchesView, Scalar(255, 0, 0), Scalar(0, 0, 255));
            // Resize the individual image
            Size smaller_size(scaled_width, (matchesView.rows * scaled_width) / matchesView.cols);
            Mat resize_match;
            resize(matchesView, resize_match, smaller_size);
            // Keep track of the max height of each image
            scaled_height = std::max(scaled_height, smaller_size.height);

            // Add the image to the big_image
            match_images.push_back(resize_match);
          }
        }
      }
      // Display a big image with all the correspondences
      unsigned int big_x = 0, big_y = 0;
      unsigned int n_match_image = 0;
      while (n_match_image < match_images.size())
      {
        if (1.5 * big_x < big_y)
        {
          big_x += scaled_width;
          n_match_image += big_y / scaled_height;
        }
        else
        {
          big_y += scaled_height;
          n_match_image += big_x / scaled_width;
        }
      }
      //Mat_<Vec3b> big_image = Mat_<Vec3b>::zeros(big_y, big_x);
      canvas.create(big_y, big_x, CV_8UC3);
      int y = 0, x = 0;
      BOOST_FOREACH(const Mat & match_image, match_images)
            {
              Mat_<Vec3b> sub_image = canvas(Range(y, y + scaled_height), Range(x, x + scaled_width));
              match_image.copyTo(sub_image);
              x += scaled_width;
              if (x >= canvas.cols)
              {
                x = 0;
                y += scaled_height;
              }
            }
      if (canvas.empty()) {
            cout << "[WARNING] Big image is empty!" << endl;
      }
    }

}

