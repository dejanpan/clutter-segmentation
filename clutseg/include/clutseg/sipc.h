/**
 * Author: Julius Adorf
 *
 * This module helps computing the score as defined by Solutions in Perception
 * Challenge 2011 (from now on referred to as SIPC) at ICRA in Shanghai. There
 * is a notable difference between the SIPC score and this score since we try
 * only to recognize one object. The score calculation remains the same. It is
 * true that the scores cannot be directly compared to the contestants' scores.
 * Yet, the recognizer that tries only to locate one object can be regarded as
 * a classifier/estimator with a priori information about all but one object in
 * the scene.
 *
 * See "How To Read a Detailed Score Report (ICRA2011 Solutions in Perception
 * Challenge)", in the following referred to as SIPC11 for a description.
 *
 * Note that the SIPC scoring system does not give any scores for empty scenes,
 * and I extended the scoring system to cover it.
 *
 * The following cases can happen in a test scene:
 *
 *  scene type          choice                      n.score     ROC terminology 
 * ----------------------------------------------------------------------------
 *   empty              none                        1.0         true negative
 *   empty              some object not on scene    0.0         false positive    
 *   not empty          none                        0.0         false negative
 *   not empty          some object on scene        0.5 + x     true positive
 *   not empty          some object not on scene    0.0         false positive
 */

#ifndef _SIPC_H_
#define _SIPC_H_

#include <vector>

namespace clutseg {

    struct sipc_t {
        sipc_t() : final_score(0), frames(0), rscore(0), tscore(0), cscore(0),
                    max_cscore(0), max_rscore(0), max_tscore(0) {}
        float final_score;
        int frames;
        float rscore;
        float tscore;
        float cscore;
        float max_cscore;
        float max_rscore;
        float max_tscore;
        void compute_final_score();
        void print();
    };

    float compute_rscore(float angle_err);
    float compute_tscore(float trans_err);

}

#endif