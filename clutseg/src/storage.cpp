/**
 * Author: Julius Adorf
 */

#include "clutseg/storage.h"

#include "clutseg/viz.h"

#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <tod/core/Features2d.h>

using namespace cv;
using namespace opencv_candidate;
using namespace std;
using namespace tod;
namespace bfs = boost::filesystem;

namespace clutseg {

    /** Stores results for one test scene. */
    void ResultStorage::store(int experiment_id,
                const bfs::path & test_dir,
                const string & img_name,
                const Mat & img,
                const Camera & camera,
                const GroundTruth & ground,
                const Result & result) {
        bfs::path erd = result_dir_ / (str(boost::format("%05d") % experiment_id));
        if (!bfs::exists(erd)) {
            bfs::create_directory(erd);
            //throw runtime_error(str(boost::format(
            //    "ERROR: Result directory for experiment %d already exists.") % experiment_id));
        }

        /* // Save image
        bfs::path img_path = erd / img_name; 
        bfs::create_directories(img_path.parent_path());
        imwrite(img_path.string(), img); */

        /*// Draw locate choice image
        Mat lci = img.clone();
        drawGroundTruth(lci, ground, camera);
        if (result.guess_made) { // TODO: make locate_choice a method, then throw exception if !guess_made
            drawGuess(lci, result.locate_choice, camera, PoseRT());
        }
        bfs::path lci_path = erd / (img_name + ".locate_choice.png");
        bfs::create_directories(lci_path.parent_path());
        imwrite(lci_path.string(), lci);

        // Draw detect choices image
        Mat dci = img.clone();
        drawGroundTruth(dci, ground, camera);
        vector<PoseRT> dummy;
        drawGuesses(dci, result.detect_choices, camera, dummy); // TODO: create delegate method or use default parameter
        bfs::path dci_path = erd / (img_name + ".detect_choices.png");
        bfs::create_directories(dci_path.parent_path());
        imwrite(dci_path.string(), dci);*/

        /*
        Mat kptsi = img.clone();
        clutseg::drawKeypoints(kptsi, result.features.keypoints);
        bfs::path kptsi_path = erd / (img_name + ".keypoints.png");
        bfs::create_directories(kptsi_path.parent_path());
        imwrite(kptsi_path.string(), kptsi);*/

        // Save keypoints
        bfs::path feat_path = erd / (img_name + ".features.yaml.gz");
        bfs::create_directories(feat_path.parent_path());
        FileStorage feat_fs(feat_path.string(), FileStorage::WRITE);
        feat_fs << Features2d::YAML_NODE_NAME;
        result.features.write(feat_fs);
        feat_fs.release();
    }

}
