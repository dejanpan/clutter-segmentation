/**
 * Author: Julius Adorf
 */

#include "clutseg/experiment.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <ctype.h>
#include <cv.h>
#include <fstream>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include <string>

using namespace boost;
using namespace std;
using namespace tod;

namespace bfs = boost::filesystem;

namespace clutseg {

    // TODO: create method that lists all training subjects

    void TrainFeatures::generate() {
        bfs::path p(getenv("CLUTSEG_PATH"));
        bfs::path train_dir = p / train_set;
        bfs::path gen_bash = train_dir / "generate.bash";
        bfs::directory_iterator dir_it(train_dir);
        bfs::directory_iterator dir_end;
        
        // As tod_training/apps/detector.cpp and
        // tod_training/apps/f3d_creator.cpp are not included in the linked
        // libraries, we cannot trigger these processes directly from C++ but
        // have to spawn a subprocess by generating a bash-script on-the-fly.
        ofstream cmd;
        cmd.open(gen_bash.string().c_str());
        cmd << "#!/usr/bin/env bash" << endl;
        cmd << "# This file has been automatically generated. " << endl;
        cmd << "# Better do not modify it because it might be " << endl;
        cmd << "# overwritten without notification. " << endl;
        cmd << "cd $1" << endl << endl;

        while (dir_it != dir_end) {
            if (bfs::is_directory(*dir_it)) {
                string subj = dir_it->filename();
                cmd << "echo 'extracting features for template " << subj << "'" << endl;
                cmd << "rosrun tod_training detector -d " << subj << " -j8" << endl;
                cmd << "echo '2d-3d-mapping for template " << subj << "'" << endl;
                cmd << "rosrun tod_training f3d_creator -d " << subj << " -j8" << endl;
                cmd << endl;
            }
            dir_it++;
        }

        cmd << endl;
        cmd.close();
             
        // start a subprocess
        FILE *in;
        in = popen(("bash " + gen_bash.string() + " " + train_dir.string()).c_str(), "r");
        /*int c;
        while ((c = fgetc(in)) != EOF) {
            cout << (unsigned char) c;
        }*/
        ssize_t len;
        do {
            char *line = NULL;
            size_t n = 0;
            len = getline(&line, &n, in);
            stringstream s;
            for (ssize_t i = 0; i < len; i++) {
                s << line[i];
            }
            cout << s.str();
        } while (len != -1);
        pclose(in);
    }

    #ifdef TEST
        TrainFeaturesCache::TrainFeaturesCache() {}
    #endif

    TrainFeaturesCache::TrainFeaturesCache(const bfs::path & cache_dir) : cache_dir_(cache_dir) {}

    bfs::path TrainFeaturesCache::trainFeaturesDir(const TrainFeatures & tr_feat) {
        return cache_dir_ / tr_feat.train_set / sha1(tr_feat.fe_params);
    }

    bool TrainFeaturesCache::trainFeaturesExist(const TrainFeatures & tr_feat) {
        return bfs::exists(trainFeaturesDir(tr_feat));
    }

    void TrainFeaturesCache::addTrainFeatures(const TrainFeatures & tr_feat, bool consistency_check) {
        if (trainFeaturesExist(tr_feat)) {
            throw runtime_error("train features already exist");
        } else {
            bfs::path p(getenv("CLUTSEG_PATH"));
            bfs::path train_dir = p / tr_feat.train_set;

            if (consistency_check) {
                // Check whether the features.config.yaml in the training data directory matches
                // the supplied feature configuration.
                FeatureExtractionParams stored_fe_params; 
                readFeParams(train_dir / "features.config.yaml", stored_fe_params);
                if (sha1(stored_fe_params) != sha1(tr_feat.fe_params)) {
                    throw runtime_error( str(format(
                        "Cannot add train features, feature extraction parameter mismatch detected.\n"
                        "Please make sure the features.config.yaml in the training base directory\n"
                        "matches the supplied feature configuration! This is a consistency check.\n"
                        "Checksums %s (stored) and %s (supplied)") % sha1(stored_fe_params) % sha1(tr_feat.fe_params)));
                }
            }


            bfs::path tfd = trainFeaturesDir(tr_feat);
            bfs::create_directories(tfd);

            // Need to generate the config.txt as well.
            ofstream cfg_out;
            cfg_out.open((tfd / "config.txt").string().c_str());

            bfs::directory_iterator dir_it(train_dir);
            bfs::directory_iterator dir_end;
            while (dir_it != dir_end) {
                if (bfs::is_directory(*dir_it)) {
                    string subj = dir_it->filename();
                    cfg_out << subj << endl;
                    bfs::directory_iterator subj_it(*dir_it);
                    bfs::directory_iterator subj_end;
                    bfs::create_directory(tfd / subj);
                    while (subj_it != subj_end) {
                        if (algorithm::ends_with(subj_it->filename(), ".f3d.yaml.gz")) {
                            bfs::copy_file( *subj_it, 
                                tfd / subj / subj_it->filename());
                        }
                        subj_it++;
                    }
                }
                dir_it++; 
            }
            cfg_out.close();
            
            writeFeParams(tfd / "features.config.yaml", tr_feat.fe_params);
        }
    }

    string sha1(const string & file) {
        // http://www.gnu.org/s/hello/manual/libc/Pipe-to-a-Subprocess.html
        // http://www.gnu.org/s/hello/manual/libc/Line-Input.html#Line-Input
        FILE *in;
        in = popen(str(format("sha1sum %s") % file).c_str(), "r");
        char *line = NULL;
        size_t n = 0;
        ssize_t len = getline(&line, &n, in);
        pclose(in);
        stringstream s;
        for (ssize_t i = 0; i < len && !isspace(line[i]); i++) {
            s << line[i];
        }
        return s.str();
    }

    string sha1(const FeatureExtractionParams & feParams) {
        char buffer[L_tmpnam];
        char * c = tmpnam(buffer);
        c = NULL;
        stringstream fn;
        fn << buffer;
        writeFeParams(fn.str(), feParams);
        string s = sha1(fn.str()); 
        bfs::remove(fn.str());
        return s;
    }

    void readFeParams(const bfs::path & p, FeatureExtractionParams & feParams) {
        cv::FileStorage in(p.string(), cv::FileStorage::READ);
        feParams.read(in[FeatureExtractionParams::YAML_NODE_NAME]);
        in.release();
    }

    void writeFeParams(const bfs::path & p, const tod::FeatureExtractionParams & feParams) {
        cv::FileStorage out(p.string(), cv::FileStorage::WRITE);
        out << FeatureExtractionParams::YAML_NODE_NAME;
        feParams.write(out);
        out.release();
    }

}

