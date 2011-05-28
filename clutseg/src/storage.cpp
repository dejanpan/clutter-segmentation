/**
 * Author: Julius Adorf
 */

#include "clutseg/storage.h"

#include "clutseg/pose.h"
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

    void ResultStorage::store(const TestReport & report) {
        bfs::path erd = result_dir_ / (str(boost::format("%05d") % report.experiment.id));
        if (!bfs::exists(erd)) {
            bfs::create_directory(erd);
        }

        cout << boost::format("[STORE] Saving result on '%s' for experiment '%d'") % report.img_name % report.experiment.id << endl;

        // TODO: extract method
        size_t offs = report.img_name.rfind(".");
        string img_basename = report.img_name;
        if (offs == string::npos) {
        } else {
            img_basename = img_basename.substr(0, offs);
        }

        // Draw locate choice image
        Mat lci = report.query.img.clone();
        drawGroundTruth(lci, report.ground, report.camera);
        if (report.result.guess_made) { 
            drawGuess(lci, report.result.locate_choice, report.camera, PoseRT());
            vector<string> err_text;
            err_text.push_back(str(boost::format("angle_error: %4.2f deg") % (report.angle_error() * 360 / (2 * M_PI))));
            err_text.push_back(str(boost::format("trans_error: %4.2f cm") % (report.trans_error() * 100)));
            drawText(lci, err_text, Point(20, 60), CV_FONT_HERSHEY_SIMPLEX, 0.7, 1, Scalar(255, 255, 255));
        }
        if (report.success()) {
            vector<string> succ_text(1, "SUCCESS");
            drawText(lci, succ_text, Point(10, 10), CV_FONT_HERSHEY_SIMPLEX, 1.2, 3, Scalar(0, 204, 0));
        } else {
            vector<string> fail_text(1, "FAILURE");
            drawText(lci, fail_text, Point(10, 10), CV_FONT_HERSHEY_SIMPLEX, 1.2, 3, Scalar(0, 0, 255));
        }

        bfs::path lci_path = erd / (img_basename + ".locate_choice.png");
        bfs::create_directories(lci_path.parent_path());
        imwrite(lci_path.string(), lci);

        // Draw detect choices image
        Mat dci = report.query.img.clone();
        drawGroundTruth(dci, report.ground, report.camera);
        vector<PoseRT> dummy;
        drawGuesses(dci, report.result.detect_choices, report.camera, dummy); // TODO: create delegate method or use default parameter
        bfs::path dci_path = erd / (img_basename + ".detect_choices.png");
        bfs::create_directories(dci_path.parent_path());
        imwrite(dci_path.string(), dci);

        /*
        Mat kptsi = img.clone();
        clutseg::drawKeypoints(kptsi, result.features.keypoints);
        bfs::path kptsi_path = erd / (img_name + ".keypoints.png");
        bfs::create_directories(kptsi_path.parent_path());
        imwrite(kptsi_path.string(), kptsi);*/

        // Save keypoints
        bfs::path feat_path = erd / (img_basename + ".features.yaml.gz");
        bfs::create_directories(feat_path.parent_path());
        FileStorage feat_fs(feat_path.string(), FileStorage::WRITE);
        feat_fs << Features2d::YAML_NODE_NAME;
        report.result.features.write(feat_fs);
        feat_fs.release();

        // Save locate choice
        bfs::path lc_path = erd / (img_basename + ".locate_choice.yaml.gz");
        bfs::create_directories(lc_path.parent_path());
        FileStorage lc_fs(lc_path.string(), FileStorage::WRITE);
        if (report.result.guess_made) {
            lc_fs << report.result.locate_choice.getObject()->name;
            report.result.locate_choice.aligned_pose().write(lc_fs);
        }
        lc_fs.release();

        // Save detect choices 
        bfs::path dc_path = erd / (img_basename + ".detect_choices.yaml.gz");
        bfs::create_directories(dc_path.parent_path());
        FileStorage dc_fs(dc_path.string(), FileStorage::WRITE);
        BOOST_FOREACH(const Guess & c, report.result.detect_choices) {
            dc_fs << c.getObject()->name;
            c.aligned_pose().write(dc_fs);
        }
        dc_fs.release();


        // Store detect.config.yaml
        TODParameters dp;
        // TODO: create function in Paramset that allows to convert
        // to TODParameters, see also src/clutseg.cpp, the following
        // lines show duplication
        dp.feParams = report.experiment.paramset.recog_pms_fe;
        dp.matcherParams = report.experiment.paramset.detect_pms_match;
        dp.guessParams = report.experiment.paramset.detect_pms_guess;
        bfs::path dp_path = erd / "detect.config.yaml";
        bfs::create_directories(dp_path.parent_path());
        FileStorage dp_fs(dp_path.string(), FileStorage::WRITE);
        dp_fs << TODParameters::YAML_NODE_NAME;
        dp.write(dp_fs);
        dp_fs.release();

        // Store locate.config.yaml
        TODParameters lp;
        lp.feParams = report.experiment.paramset.recog_pms_fe;
        lp.matcherParams = report.experiment.paramset.locate_pms_match;
        lp.guessParams = report.experiment.paramset.locate_pms_guess;
        bfs::path lp_path = erd / "locate.config.yaml";
        bfs::create_directories(lp_path.parent_path());
        FileStorage lp_fs(lp_path.string(), FileStorage::WRITE);
        lp_fs << TODParameters::YAML_NODE_NAME;
        lp.write(lp_fs);
        lp_fs.release();
    }

}
