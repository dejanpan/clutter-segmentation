/*
 * Author: Julius Adorf
 */

#include "clutseg/clutseg.h"

#include "clutseg/common.h"
#include "clutseg/map.h"

#include <tod/detecting/Loader.h>
#include <boost/foreach.hpp>
#include <limits>

using namespace std;
using namespace cv;
using namespace pcl;
using namespace tod;

namespace clutseg {

    #ifdef TEST
        // see header
        ClutSegmenter::ClutSegmenter() {}
    #endif
        
    ClutSegmenter::ClutSegmenter(const string & baseDirectory,
                                    const string & detect_config,
                                    const string & locate_config,
                                    Ptr<GuessRanking> ranking,
                                    float accept_threshold) :
                                baseDirectory_(baseDirectory),
                                ranking_(ranking),
                                accept_threshold_(accept_threshold) {
        loadParams(detect_config, detect_params_);
        loadParams(locate_config, locate_params_);
        loadBase();
    }

    ClutSegmenter::ClutSegmenter(const string & baseDirectory,
                                    const TODParameters & detect_params,
                                    const TODParameters & locate_params,
                                    Ptr<GuessRanking> ranking,
                                    float accept_threshold) :
                                baseDirectory_(baseDirectory),
                                detect_params_(detect_params),
                                locate_params_(locate_params),
                                ranking_(ranking),
                                accept_threshold_(accept_threshold) {
        loadBase();
    }

    void ClutSegmenter::loadParams(const string & config, TODParameters & params) {
        FileStorage fs(config, FileStorage::READ);
        if (!fs.isOpened()) {
            throw ios_base::failure("Cannot read configuration file '" + config + "'");
        }
        params.read(fs[TODParameters::YAML_NODE_NAME]);
        fs.release();
    }

    void ClutSegmenter::loadBase() {
        Loader loader(baseDirectory_);
        loader.readTexturedObjects(objects_);
        base_ = TrainingBase(objects_);
    }

    TODParameters & ClutSegmenter::getDetectParams() {
        return detect_params_;
    }

    TODParameters & ClutSegmenter::getLocateParams() {
        return locate_params_;
    }

    int ClutSegmenter::getAcceptThreshold() const {
        return accept_threshold_;
    }

    void ClutSegmenter::setAcceptThreshold(int accept_threshold) {
        accept_threshold_ = accept_threshold;
    }

    Ptr<GuessRanking> ClutSegmenter::getRanking() const {
        return ranking_;
    }

    void ClutSegmenter::setRanking(const Ptr<GuessRanking> & ranking) {
        ranking_ = ranking;
    }

    set<string> ClutSegmenter::getTemplateNames() const {
        set<string> s;
        BOOST_FOREACH(const Ptr<TexturedObject> & t, objects_) {
            s.insert(t->name);
        }
        return s;
    }

    void initRecognizer(Ptr<Recognizer> & recognizer, TrainingBase & base, TODParameters params, const string & baseDirectory) {
        Ptr<Matcher> rtMatcher = Matcher::create(params.matcherParams);
        rtMatcher->add(base);
        recognizer = new KinectRecognizer(&base, rtMatcher,
                            &params.guessParams, 0 /* verbose */,
                             baseDirectory);
    }

    bool ClutSegmenter::recognize(const Mat & queryImage, const PointCloudT & queryCloud, Guess & resultingGuess, PointCloudT & inliersCloud) {
        Features2d query;
        query.image = queryImage;

        // Generate a couple of guesses. Ideally, each object on the scene is
        // detected and there are no misclassifications.
        vector<Guess> guesses;
        detect(query, guesses);

        if (guesses.empty()) {
            // In case there are any objects in the scene, this is kind of the
            // worst-case. None of the objects has been detected.
            return false;
        } else {
            BOOST_FOREACH(Guess & guess, guesses) {
                mapInliersToCloud(guess.inlierCloud, guess, query.image, queryCloud);
            }
            // Sort the guesses according to the ranking function.
            sort(guesses.begin(), guesses.end(), GuessComparator(ranking_));
            bool pos = false;
            // Iterate over every guess, beginning with the highest ranked
            // guess. If the guess resulting of locating the object got a score
            // larger than the acceptance threshold, this is our best guess.
            for (size_t i = 0; i < guesses.size(); i++) {
                resultingGuess = guesses[0]; 

                cout << "inliers before: " << resultingGuess.inliers.size() << endl;
                locate(query, queryCloud, resultingGuess);
                cout << "inliers after:  " << resultingGuess.inliers.size() << endl;

                cout << "ranking: " << (*ranking_)(resultingGuess) << endl;
                cout << "accept_threshold: " << accept_threshold_ << endl;

                if ((*ranking_)(resultingGuess) >= accept_threshold_) {
                    pos = true;
                    break;
                }
            }

            inliersCloud = PointCloudT(); 
            if (pos) {
                mapInliersToCloud(inliersCloud, resultingGuess, queryImage, queryCloud);
            }

            return pos;
        }
    }

    bool ClutSegmenter::detect(Features2d & query, vector<Guess> & guesses) {
        Ptr<FeatureExtractor> extractor = FeatureExtractor::create(detect_params_.feParams);
        Ptr<Recognizer> recognizer;
        initRecognizer(recognizer, base_, detect_params_, baseDirectory_);

        extractor->detectAndExtract(query);
        recognizer->match(query, guesses);
        return guesses.empty();
    }

    bool ClutSegmenter::locate(const Features2d & query, const PointCloudT & queryCloud, Guess & resultingGuess) {
        if (locate_params_.matcherParams.doRatioTest) {
            cerr << "[WARNING] RatioTest enabled for locating object" << endl;
        }
        vector<Ptr<TexturedObject> > so;
        BOOST_FOREACH(const Ptr<TexturedObject> & obj, objects_) {
            if (obj->name == resultingGuess.getObject()->name) {
                // Create a new textured object instance --- those thingies
                // cannot exist in multiple training bases, this breaks indices
                // that are tightly coupled between TrainingBase and
                // TexturedObject instances.
                Ptr<TexturedObject> newObj = new TexturedObject();
                newObj->id = 0;
                newObj->name = obj->name;
                newObj->directory_ = obj->directory_;
                newObj->stddev = obj->stddev;
                newObj->observations = obj->observations;
                so.push_back(newObj);
                break;
            }
        }
        TrainingBase single(so);

        Ptr<Recognizer> recognizer;
        initRecognizer(recognizer, single, locate_params_, baseDirectory_);

        vector<Guess> guesses;
        recognizer->match(query, guesses); 

        if (guesses.empty()) {
            // This almost certainly should not happen. If the object has been
            // detected despite of possible confusion, it will be detected
            // again if the source of confusion is removed.
            cerr << "[WARNING] No guess made in refinement!" << endl;
            return false;
        } else {
            BOOST_FOREACH(Guess & guess, guesses) {
                mapInliersToCloud(guess.inlierCloud, guess, query.image, queryCloud);
            }
            sort(guesses.begin(), guesses.end(), GuessComparator(ranking_));
            resultingGuess = guesses[0]; 
            return true;
        }
    }

}

