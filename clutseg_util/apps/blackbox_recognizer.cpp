/*
 * blackbox_recognizer.cpp
 */

/* This recognizer takes a test configuration file that lists a number of test
 * images and the objects that can be found on each test image. It prints out
 * the image name, the actual objects on this image and those object that have
 * been predicted to be on the image. The advantage of such batch-processing is
 * speed because the training base has not to be reloaded each time.
 *
 * Originally derived from folder_recognizer.cpp in tod_detecting/apps. The
 * folder_recognizer.cpp in tod_detecting rev. 50320 is broken for several
 * reasons.
 * - it does only handle png files
 * - it interprets files that contain .png in their filename as images
 * - it inscrupulously tries to recognize objects on each image it can find
 *   within a directory, but does not attempt to recurse.
 * - it produces OpenCV errors
 * - it's command-line description is out of sync
 * - it generates spam sample.config.yaml files
 * - it is spamming standard output
 * - ...
 * 
 * This implementation fixes some of the issues.
 */

#include "tod/detecting/Loader.h"
#include "tod/detecting/Recognizer.h"
#include "testdesc.h"
#include "mute.h"

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>

#define foreach BOOST_FOREACH

using namespace cv;
using namespace tod;
using namespace std;
namespace po = boost::program_options;

namespace {
    struct Options
    {
        std::string imageDirectory;
        std::string baseDirectory;
        std::string config;
        std::string testdescFilename;
        std::string resultFilename;
        std::string logFilename;
        TODParameters params;
        int verbose;
        int mode;
    };
}

int options(int ac, char **av, Options & opts)
{

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()("help", "Produce help message.");
    desc.add_options()("image,I", po::value < string > (&opts.imageDirectory),
                       "Test image base directory.");
    desc.add_options()("testdesc",
                       po::value < string > (&opts.testdescFilename),
                       "Test description file");
    desc.add_options()("base,B",
                       po::value < string >
                       (&opts.baseDirectory)->default_value("./"),
                       "The directory that the training base is in.");
    desc.add_options()("tod_config,f", po::value < string > (&opts.config),
                       "The name of the configuration file");
    desc.add_options()("log,l", po::value < string > (&opts.logFilename),
                       "The name "
                       "of the log file where results are written to in YAML format. Cannot be written "
                       "to standard output because standard output seems to serve debugging "
                       "purposes...");
    desc.add_options()("result,r",
                       po::value < string > (&opts.resultFilename),
                       "Result file using INI-style/Python config file syntax. This one is optional.");
    desc.add_options()("verbose,V",
                       po::value < int >(&opts.verbose)->default_value(1),
                       "Verbosity level");
    desc.add_options()("mode,m",
                       po::value < int >(&opts.mode)->default_value(0),
                       "Mode");

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    FileStorage fs;
    if (opts.config.empty()
        || !(fs = FileStorage(opts.config, FileStorage::READ)).isOpened()) {
        cout << "Must supply configuration." << "\n";
        cout << desc << endl;
        return 1;
    } else
        opts.params.read(fs[TODParameters::YAML_NODE_NAME]);

    if (!vm.count("image")) {
        cout << "Must supply an image directory." << "\n";
        cout << desc << endl;
        return 1;
    }

    if (!vm.count("testdesc")) {
        cout << "Must supply a test description file." << "\n";
        cout << desc << endl;
        return 1;
    }

    if (!vm.count("base")) {
        cout << "Must supply training base directory." << "\n";
        cout << desc << endl;
        return 1;
    }

    return 0;

}

int main(int argc, char *argv[])
{
    Options opts;
    if (options(argc, argv, opts))
        return 1;

    tod::Loader loader(opts.baseDirectory);
    vector < cv::Ptr < TexturedObject > >objects;
    loader.readTexturedObjects(objects);

    if (!objects.size()) {
        cout << "Empty base\n" << endl;
    }

    TrainingBase base(objects);
    Ptr < FeatureExtractor > extractor =
        FeatureExtractor::create(opts.params.feParams);

    cv::Ptr < Matcher > rtMatcher =
        Matcher::create(opts.params.matcherParams);
    rtMatcher->add(base);

    cv::Ptr < Recognizer > recognizer;
    if (opts.mode == TOD) {
        recognizer =
            new TODRecognizer(&base, rtMatcher, &opts.params.guessParams,
                              opts.verbose, opts.baseDirectory,
                              opts.params.clusterParams.maxDistance);
    } else if (opts.mode == KINECT) {
        recognizer =
            new KinectRecognizer(&base, rtMatcher, &opts.params.guessParams,
                                 opts.verbose, opts.baseDirectory);
    } else {
        cout << "Invalid mode option!" << endl;
        return 1;
    }

    TestDesc testdesc = loadTestDesc(opts.testdescFilename);

    FileStorage fs;
    fs.open(opts.logFilename, FileStorage::WRITE);
    fs << "trainFolder" << opts.baseDirectory;
    fs << "test1" << "{";
    fs << "testFolder" << opts.imageDirectory;
    fs << "objects" << "{";

    bool writeR = (opts.resultFilename != "");
    fstream r;
    if (writeR) {
        r.open(opts.resultFilename.c_str(), ios_base::out);
        r << "[predictions]" << endl;
    }

    int objectIndex = 1;

    for (TestDesc::iterator it = testdesc.begin(), end = testdesc.end();
         it != end; it++) {
        std::string image_name = it->first;
        string path = opts.imageDirectory + "/" + image_name;
        cout << "< Reading the image... " << path;

        Features2d test;
        test.image = imread(path, 0);
        cout << ">" << endl;
        if (test.image.empty()) {
            cout << "Cannot read test image" << endl;
            break;
        }
        extractor->detectAndExtract(test);

        Mat drawImage;
        if (test.image.channels() > 1)
            test.image.copyTo(drawImage);
        else
            cvtColor(test.image, drawImage, CV_GRAY2BGR);

        vector < tod::Guess > foundObjects;
        recognizer->match(test, foundObjects);

        if (writeR) {
            r << image_name << " = ";
        }

        set < string > found;
        foreach(const Guess & guess, foundObjects)
        {
            stringstream nodeIndex;
            nodeIndex << objectIndex++;
            string nodeName = "object" + nodeIndex.str();
            fs << nodeName << "{";
            fs << "id" << guess.getObject()->id;
            fs << "name" << guess.getObject()->name;
            fs << "rvec" << guess.aligned_pose().rvec;
            fs << "tvec" << guess.aligned_pose().tvec;
#if 0
            int index = atoi((image_name.substr(0, position)).c_str());
            fs << "imageIndex" << index;
#endif
            fs << "imageName" << image_name;
            // damn nasty bug
            //fs << "idx" <<  guess.image_indices_;
            fs << "}";
            found.insert(guess.getObject()->name);
        }
        if (writeR) {
            foreach(string obj, found) {
                r << obj << " ";
            }
        }
        if (writeR) {
            r << endl;
        }
    }
    fs << "objectsCount" << objectIndex - 1;
    fs << "}" << "}";
    fs.release();
    if (writeR) {
        r.close();
    }
    return 0;
}