/**
 * Author: Julius Adorf
 */

#include "clutseg/response.h"

#include "clutseg/pose.h"

#include <boost/foreach.hpp>
#include <iostream>

using namespace std;
using namespace tod;

namespace clutseg {


    void ResponseFunction::operator()(const TestSetResult & result, const TestSetGroundTruth & ground, Response & response) {
        response.value = 0.0;
    }

    void CutSseResponseFunction::operator()(const TestSetResult & result, const TestSetGroundTruth & ground, Response & response) {
        ResponseFunction::operator()(result, ground, response);

        double r_acc = 0;
        for (TestSetGroundTruth::const_iterator it = ground.begin(); it != ground.end(); it++) {
            const string & img_name = it->first;
            const GroundTruth & groundTruth = it->second;
            if (!result.guessMade(img_name)) {
                if (!groundTruth.empty()) {
                    r_acc += 1.0;
                }
            } else {
                Guess guess = result.get(img_name);
                PoseRT est_pose = poseToPoseRT(guess.aligned_pose());
                double r = 1.0;
                BOOST_FOREACH(NamedPose np, groundTruth) {
                    cout << np.name << endl;
                    cout << guess.getObject()->name << endl;
                    if (np.name == guess.getObject()->name) {
                        double dt = distBetweenLocations(est_pose, np.pose); 
                        double da = angleBetweenOrientations(est_pose, np.pose); 
                        double r2 = (dt * dt) / (max_trans_error_ * max_trans_error_) + (da * da) / (max_angle_error_ * max_angle_error_);
                        r = r2 < r ? r2 : r;
                    }
                }
                r_acc += r;
            }
        }
        response.value = r_acc / ground.size();
    }

}
