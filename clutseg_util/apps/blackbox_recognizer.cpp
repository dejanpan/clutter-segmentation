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
 * This implementation fixes some of the issues. Also, it tries to collect as
 * much information about the performance and output of the classifier on a
 * given testing set.
 */

#include "tod/detecting/Loader.h"
#include "tod/detecting/Recognizer.h"
#include "testdesc.h"
#include "mute.h"
#include "drawpose.h"

#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv_candidate/PoseRT.h>
#include <iostream>
#include <fstream>
#include <set>
#include <string>

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
        std::string statsFilename;
        std::string logFilename;
        std::string rocFilename;
        std::string tableFilename;
        std::string storeDirectory;
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
    desc.add_options()("stats,s",
                       po::value < string > (&opts.statsFilename),
                       "Statistics are stored in this file. INI-style/Python config file syntax.");
    desc.add_options()("roc",
                       po::value < string > (&opts.rocFilename),
                       "Generate points on a ROC graph and save it to a file.");
    desc.add_options()("table",
                       po::value < string > (&opts.tableFilename),
                       "Generate a CSV table containing information about guesses.");
    desc.add_options()("store",
                       po::value < string > (&opts.storeDirectory),
                       "Write all results to this folder in a pre-defined "
                       "manner. If you specify this option then arguments of "
                       "--result, --stats, --roc, --table will be ignored.");
    desc.add_options()("verbose,V",
                       po::value < int >(&opts.verbose)->default_value(1),
                       "Verbosity level");
    desc.add_options()("mode,m",
                       po::value < int >(&opts.mode)->default_value(1),
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

bool readImage(Features2d & test, const string & path) {
    cout << boost::format("Reading <%s>") % path << endl;
    test.image = imread(path, 0);
    if (test.image.empty()) {
        cout << "Cannot read test image" << endl;
        return false;
    }
    return true;
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

    bool write_store = (opts.storeDirectory != "");

    // If --storeDirectory is specified, all and as many results as possible
    // are to be written to that folder.
    if (write_store) {
        opts.resultFilename = opts.storeDirectory + "/results.txt";
        opts.statsFilename = opts.storeDirectory + "/stats.txt";
        opts.rocFilename = opts.storeDirectory + "/roc.gnuplot";
        opts.logFilename = opts.storeDirectory + "/log.txt";
        opts.tableFilename = opts.storeDirectory + "/table.csv";
    }

    bool write_log = (opts.logFilename != "");
    bool write_result = (opts.resultFilename != "");
    bool write_table = (opts.tableFilename != "");
    bool write_stats = (opts.statsFilename != "");
    bool write_roc = (opts.rocFilename != "");

    FileStorage log;
    fstream result;
    fstream roc;
    fstream stats;
    fstream table;


    if (write_log) {
        log.open(opts.logFilename, FileStorage::WRITE);
        log << "trainFolder" << opts.baseDirectory;
        log << "test1" << "{";
        log << "testFolder" << opts.imageDirectory;
        log << "objects" << "{";
    }

    if (write_result) {
        result.open(opts.resultFilename.c_str(), ios_base::out);
        result << "[predictions]" << endl;
    }
 
    if (write_roc) {
        roc.open(opts.rocFilename.c_str(), ios_base::out);
        roc << "set size .75,1" << endl;
        roc << "set size ratio 1" << endl;
        roc << "set xtics .1" << endl;
        roc << "set ytics .1" << endl;
        roc << "set grid" << endl;
        roc << "set xrange [0:1.025]" << endl;
        roc << "set yrange [0:1.025]" << endl;
        roc << "set key right bottom" << endl;
        roc << "set ylabel \"True Positive Rate\"" << endl;
        roc << "set xlabel \"False Positive Rate\" " << endl;
        roc << "set pointsize 2 " << endl;
        roc << "plot x with lines, '-' with points" << endl;
    }

    if (write_stats) {
        stats.open(opts.statsFilename.c_str(), ios_base::out);
        stats << "[statistics]" << endl;
    }

    if (write_table) {
        table.open(opts.tableFilename.c_str(), ios_base::out);
        table << boost::format("%-15s %-25s %-5s %-3s %-7s "
                           "%-10s %-10s %-10s %-10s %-10s %-10s "
                           "%-10s %-10s %-10s %-10s %-10s %-10s "
                           "%-10s %-10s") 
                            % "image" % "object" % "guess" % "hit" % "inliers"
                            % "guess_tx" % "guess_ty" % "guess_tz" % "guess_rx" % "guess_ry" % "guess_rz"
                            % "est_tx" % "est_ty" % "est_tz" % "est_rx" % "est_ry" % "est_rz" 
                            % "max_rerr_t" % "max_rerr_r" << endl;
    }
  
    int objectIndex = 1;

    // count hits, misses, error 1, error 2.  see Receiver operating
    // characteristics.  true/false positives/negatives.  A confusion matrix
    // has 4 dof and it's easiest to keep track of tp, fp and the column sums
    // to compute tn, fn later.

    int tp_acc = 0;
    int fp_acc = 0;
    int tn_acc = 0;
    int fn_acc = 0;
    int p_acc = 0;
    int n_acc = 0;

    for (TestDesc::iterator it = testdesc.begin(); it != testdesc.end(); it++) {
        set<string> expected = it->second; 
        // true positives
        int tp = 0;
        // false positives
        int fp = 0;
        // total positives
        int p = expected.size();

        string img_name = it->first;
        Features2d test;
        
        string path = opts.imageDirectory + "/" + img_name;
        if (!readImage(test, path)) {
            cerr << "Error: cannot read image" << endl;
            return -1;
        }

        extractor->detectAndExtract(test);

        vector<Guess> guesses;
        recognizer->match(test, guesses);

        // The set of detected objects on the query image.
        set<string> found;
        // The number of guesses per detected object.
        map<string, int> guess_count;
        foreach(const Guess & guess, guesses) {
            string name = guess.getObject()->name;
            Pose guess_pose = guess.aligned_pose();
            
            found.insert(name);
            guess_count[name] += 1;

            if (write_log) {
                stringstream nodeIndex;
                nodeIndex << objectIndex;
                string nodeName = "object" + nodeIndex.str();
                log << nodeName << "{";
                log << "id" << guess.getObject()->id;
                log << "name" << name;
                log << "imageName" << img_name;
                log << Pose::YAML_NODE_NAME;
                guess_pose.write(log);
                log << "}";
            }
           
            if (write_store) {
                // Write guessed pose to file
                FileStorage p_out;
                p_out.open(str(boost::format("%s/%s.%s.%d.guessed.pose.yaml") % opts.storeDirectory % img_name % name % guess_count[name]), FileStorage::WRITE);
                p_out << Pose::YAML_NODE_NAME;
                guess_pose.write(p_out);
                p_out.release();
            }

            if (write_table) {
                PoseRT guess_posert;
                poseToPoseRT(guess_pose, guess_posert);
                float guess_tx = guess_posert.tvec.at<float>(0, 0);
                float guess_ty = guess_posert.tvec.at<float>(1, 0);
                float guess_tz = guess_posert.tvec.at<float>(2, 0);
                float guess_rx = guess_posert.rvec.at<float>(0, 0);
                float guess_ry = guess_posert.rvec.at<float>(1, 0);
                float guess_rz = guess_posert.rvec.at<float>(2, 0);
                table << boost::format("%-15s %-25s %5d %3d %7d "
                           "%10.7f %10.7f %10.7f %10.7f %10.7f %10.7f "
                           "%-10s %-10s %-10s %-10s %-10s %-10s "
                           "%-10s %-10s") 
                            % img_name % name % guess_count[name]  % (expected.find(name) == expected.end() ? 0 : 1) % guess.inliers.size()
                            % guess_tx % guess_ty % guess_tz % guess_rx % guess_ry % guess_rz
                            % "-" % "-" % "-" % "-" % "-" % "-" 
                            % "-" % "-" << endl;
            }
            objectIndex++;
        }

        if (opts.verbose >= 2) {
            foreach(const Guess & guess, guesses) {
                Mat canvas = test.image.clone();
                drawPose(guess.aligned_pose(), test.image, objects[0]->observations[0].camera(), canvas);
                imshow("Guess", canvas);
                waitKey(0);
            }
        }

        foreach(string name, found) {
            // Check for true or false positive.
            if (it->second.find(name) == it->second.end()) {
                fp += 1;
            } else {
                tp += 1;
            }
        }

        int n = objects.size() - p;
        int fn = p - tp;
        int tn = n - fp;
        if (write_stats) {
            stats << endl;
            stats << "# -- " << img_name << " -- " << endl;
            stats << "# actual objects: ";
            foreach (string x, it->second) {
                stats << x << ", ";
            }
            stats << endl;
            stats << "# predicted objects: ";
            foreach (string x, found) {
                stats << x << ", ";
            }
            stats << endl;
            stats << "# tp = " << tp << endl;
            stats << "# fp = " << fp << endl;
            stats << "# tn = " << tn << endl;
            stats << "# fn = " << fn << endl;
            stats << "# p = " << p << endl;
            stats << "# n = " << n << endl;
        }

        n_acc += n;
        p_acc += p;
        tp_acc += tp;
        fp_acc += fp;
        fn_acc += fn;
        tn_acc += tn;

        if (write_result) {
            result << img_name << " = ";
            foreach(string obj, found) {
                result << obj << " ";
            }
            result << endl;
        }
        foreach (string obj, found) {
            cout << (boost::format("Detected %15s (%d different guesses)") % obj % guess_count[obj]) << endl;
        }
    }

    if (write_log) {
        log << "objectsCount" << objectIndex - 1;
        log << "}" << "}";
        log.release();
    }

    if (write_result) {
        result.close();
    }

    if (write_roc) {
        // gnuplot does not like NaN
        if (n_acc > 0 && p_acc > 0) {
            roc << fp_acc / (double) n_acc << " ";
            roc << tp_acc / (double) p_acc << endl;
        }
        roc << "e" << endl;
        roc << "## confusion matrix" << endl;
        roc << "##    accumulated over each test image";
        roc << "# tp = " << tp_acc << endl;
        roc << "# fp = " << fp_acc << endl;
        roc << "# tn = " << tn_acc << endl;
        roc << "# fn = " << fn_acc << endl;
        roc << "## n = tn + fp" << endl;
        roc << "# n = " << n_acc << endl;
        roc << "## p = tp + fn" << endl;
        roc << "# p = " << p_acc << endl; 
        roc << "## tp_rate = tp / p = sensitivity = hit rate = recall" << endl;
        roc << "# tp_rate = " << tp_acc / (double) p_acc << endl;
        roc << "## fp_rate = fall-out" << endl;
        roc << "# fp_rate = " << fp_acc / (double) n_acc << endl;
        roc.close();
    }

    if (write_stats) {
        // write down confusion matrix
        // plus some more calculation on top of it
        stats << endl;
        stats << endl;
        stats << "# confusion matrix" << endl;
        stats << "#    accumulated over each test image" << endl;
        stats << "tp = " << tp_acc << endl;
        stats << "fp = " << fp_acc << endl;
        stats << "tn = " << tn_acc << endl;
        stats << "fn = " << fn_acc << endl;
        stats << "# n = tn + fp" << endl;
        stats << "n = " << n_acc << endl;
        stats << "# p = tp + fn" << endl;
        stats << "p = " << p_acc << endl; 
        stats << "# tp_rate = tp / p = sensitivity = hit rate = recall" << endl;
        stats << "tp_rate = " << tp_acc / (double) p_acc << endl;
        stats << "# fp_rate = fall-out" << endl;
        stats << "fp_rate = " << fp_acc / (double) n_acc << endl;
        stats.close();
    }

    if (write_table) {
        table.close();
    }

    return 0;
}
