/*
 * Author: Julius Adorf
 */

#include "clutseg/ground.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <cv.h>
#include <fstream>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

using namespace cv;
using namespace std;

namespace bfs = boost::filesystem;

namespace clutseg {

    bool GroundTruth::onScene(const string & name) const {
        // slow 
        BOOST_FOREACH(const NamedPose & np, labels) {
            if (np.name == name) {
                return true;
            }
        }
        return false;
    }

    void GroundTruth::read(const bfs::path & filename) {
        labels.clear();
        FileStorage fs = FileStorage(filename.string(), FileStorage::READ);
        // iterate over objects
        for (FileNodeIterator n_it = fs.root().begin(); n_it != fs.root().end(); n_it++) {
            NamedPose np((*n_it).name());
            np.pose.read(*n_it);
            np.pose.estimated = true;
            labels.push_back(np);  
        }
    }

    TestSetGroundTruth loadTestSetGroundTruth(const bfs::path & filename) {
        TestSetGroundTruth m = loadTestSetGroundTruthWithoutPoses(filename);
        for (TestSetGroundTruth::iterator it = m.begin(); it != m.end(); it++) {
            string img_name = it->first;   
            GroundTruth g = it->second;
            string ground_name = img_name + ".ground.yaml";
            g.read(filename.parent_path() / ground_name);
        }
        return m;
    }

    // TODO: does not really parse a python configuration file
    TestSetGroundTruth loadTestSetGroundTruthWithoutPoses(const bfs::path & filename) {
        TestSetGroundTruth m;
        ifstream f;
        f.open(filename.string().c_str()); 
        if (!f.is_open()) {
            throw ios_base::failure(
                str(boost::format(
                "Cannot open testdesc file '%s', does it exist?") % filename));
        }
        size_t s = 1024;
        char cline[1024];
        while (!f.eof()) {
            if (f.fail()) {
                throw ios_base::failure("Cannot read line from testdesc file, failbit set!");                
            } 
            // getline stops on eof, no character will be
            // read twice.
            f.getline(cline, s);
            string line(cline);
            size_t offs = line.find_first_of('='); 
            if (offs != string::npos) {
                string key = line.substr(0, offs);
                string val = line.substr(offs+1, line.length() - offs);
                boost::trim(key);
                vector<string> v;
                boost::split(v, val, boost::is_any_of(" "), boost::token_compress_on);
                GroundTruth groundTruth;
                for (size_t i = 0; i < v.size(); i++) {
                    boost::trim(v[i]);
                    if (v[i].length() > 0) {
                        NamedPose np(v[i]);
                        groundTruth.labels.push_back(np);
                    }
                } 
                m[key] = groundTruth;
            }
        }
        f.close();
        return m; 
    }

}

