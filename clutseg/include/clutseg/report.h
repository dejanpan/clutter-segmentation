/**
 * Author: Julius Adorf
 */

#ifndef _REPORT_H_
#define _REPORT_H_

#include "clutseg/ground.h"
#include "clutseg/paramsel.h"
#include "clutseg/result.h"

#include "clutseg/gcc_diagnostic_disable.h"
    #include <boost/filesystem.hpp>
    #include <cv.h>
    #include <opencv_candidate/Camera.h>
#include "clutseg/gcc_diagnostic_enable.h"

namespace clutseg {

    /** The test report bundles all information about a single test scene
     * evaluated during the run of an experiment. It has knowledge about the
     * input and output, plus basic statistics local to the single test scene.
     * Given such test reports, global statistics can easily be generated by
     * aggregation. It's equally easy to process a test report locally, e.g.
     * for storing results to the filesystem. */
    struct TestReport {

        TestReport() {}

        TestReport(const Experiment & experiment,
                    const ClutsegQuery & query,
                    const Result & result,
                    const LabelSet & ground,
                    const std::string & img_name,
                    const boost::filesystem::path & test_dir,
                    const opencv_candidate::Camera camera) :
                        experiment(experiment),
                        query(query),
                        result(result),
                        ground(ground),
                        img_name(img_name),
                        test_dir(test_dir),
                        camera(camera) {}
    
        Experiment experiment;
        ClutsegQuery query;
        Result result;
        LabelSet ground;
       
        std::string img_name;
        boost::filesystem::path test_dir; 
        opencv_candidate::Camera camera;

        float angle_error() const;
        float trans_error() const;
        bool success() const;

    };

}

#endif
