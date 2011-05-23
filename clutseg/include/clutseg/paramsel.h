/**
 * Author: Julius Adorf
 */

#ifndef _PARAMSEL_H_
#define _PARAMSEL_H_

#include "clutseg/sipc.h"

#include <boost/format.hpp>
#include <map>
#include <sqlite3.h>
#include <tod/training/feature_extraction.h>
#include <tod/detecting/GuessGenerator.h>
#include <tod/detecting/Parameters.h>
#include <vector>

namespace clutseg {


    /** A serializable object. It corresponds to a row in a relational database
     * with rowid id. If id <= 0, then the object is new and has not yet been
     * written to or read from the database. An object must manage its own
     * serialization and if it contains members that are not of type Serializable,
     * it must also manage foreign key relationships. */
    struct Serializable {

        /** Corresponds to Sqlite3 rowid. */
        int64_t id;

        Serializable() : id(-1) {}

        /** Inserts or updates corresponding rows in the database. */
        virtual void serialize(sqlite3* db);

        /** Reads member values from database. */
        virtual void deserialize(sqlite3* db);

    };

    struct ClutsegParams : public Serializable {

        float accept_threshold;
        std::string ranking;

        virtual void serialize(sqlite3* db);
        virtual void deserialize(sqlite3* db);

    };

    struct Paramset : public Serializable {

        Paramset() : train_pms_fe_id(-1), recog_pms_fe_id(-1),
                    detect_pms_match_id(-1), detect_pms_guess_id(-1),
                    locate_pms_match_id(-1), locate_pms_guess_id(-1),
                    max_trans_error(0.03), max_angle_error(M_PI / 9) {}
    
        tod::FeatureExtractionParams train_pms_fe;
        tod::FeatureExtractionParams recog_pms_fe;
        tod::MatcherParameters detect_pms_match;
        tod::GuessGeneratorParameters detect_pms_guess;
        tod::MatcherParameters locate_pms_match;
        tod::GuessGeneratorParameters locate_pms_guess;

        // The row identifiers are always stored explicitly in each
        // serializable structure. Since the tod::* structures are not
        // serializable, we manage ids for them externally.
        int64_t train_pms_fe_id;
        int64_t recog_pms_fe_id;
        int64_t detect_pms_match_id;
        int64_t detect_pms_guess_id;
        int64_t locate_pms_match_id;
        int64_t locate_pms_guess_id;
 
        ClutsegParams pms_clutseg;

        /** Maximum translational error that allows a guess to be marked a
         * success. */
        float max_trans_error;
        /** Maximum orientational error that allows a guess to be marked a
         * success. */
        float max_angle_error;

        virtual void serialize(sqlite3* db);
        virtual void deserialize(sqlite3* db);

    };

    /** Stores statistics about an experiment run on a test set. These values
     * guide the selection of better configurations. Note that we distinguish
     * between failures and successes that are defined by the joint angular and
     * translational error exceeding a predefined threshold. */
    struct Response : public Serializable {

        /** A maintainer's nightmare */
       Response() : value(0.0), succ_rate(0), avg_angle_err(0),
            avg_succ_angle_err(0), avg_trans_err(0), avg_succ_trans_err(0),
            avg_angle_sq_err(0), avg_succ_angle_sq_err(0), avg_trans_sq_err(0),
            avg_succ_trans_sq_err(0), mislabel_rate(0), none_rate(0), avg_keypoints(0),
            avg_detect_matches(0), avg_detect_inliers(0), avg_detect_choice_matches(0),
            detect_tp(0), detect_fp(0), detect_fn(0), detect_tn(0), avg_locate_matches(0),
            avg_locate_inliers(0), avg_locate_choice_matches(0), avg_locate_choice_inliers(0)
            { sipc_score = sipc_t(); }

        /** Average of the values returned by the response function */
        float value;
        /** SIPC score, see sipc.h */
        sipc_t sipc_score;
        /** Average of query images where locating (up to error margins)
         * succeeded. Depends on max_angle_error and max_trans_error. */
        float succ_rate;
        /** Average orientational error (all queries). */
        float avg_angle_err;
        /** Average orientational error (success only). This statistic depends
         * on max_angle_error. */
        float avg_succ_angle_err;
        /** Average translational error (all queries). */
        float avg_trans_err;
        /** Average translational error (success only). This statistic depends
         * on max_trans_error. */
        float avg_succ_trans_err;
        /** Average orientational squared error (all queries) */
        float avg_angle_sq_err;
        /** Average orientational squared error (success only). This statistic
         * depends on max_angle_error. */
        float avg_succ_angle_sq_err;
        /** Average translational squared error (all queries) */
        float avg_trans_sq_err;
        /** Average translational squared error (success only). This statistic
         * depends on max_trans_error. */
        float avg_succ_trans_sq_err;
        /** Average number of scenes where label was not correct. */
        float mislabel_rate;
        /** Average number of scenes where no choice was made, though scene was
         * not empty. */
        float none_rate;

        /** Average number of extracted keypoints per image */
        float avg_keypoints;
        /** Average number of matches in detection stage */
        float avg_detect_matches;
        /** Average number of inliers of all guesses in detection stage */
        float avg_detect_inliers;
        /** Average number of matches for the best guess object in detection stage */
        float avg_detect_choice_matches;
        /** Average number of inliers for the best guess object in detection stage */
        float avg_detect_choice_inliers;

        /** ROC true positives for detection stage */
        int detect_tp;
        /** ROC false positives for detection stage */
        int detect_fp;
        /** ROC false negatives for detection stage */
        int detect_fn;
        /** ROC true negatives for detection stage */
        int detect_tn;

        /** Average number of matches in locating stage */
        float avg_locate_matches;
        /** Average number of inliers of all guesses in locating stage */
        float avg_locate_inliers;
        /** Average number of matches for the best guess object in locating stage */
        float avg_locate_choice_matches;
        /** Average number of inliers for the best guess object in locating stage */
        float avg_locate_choice_inliers;

        inline float fail_rate() const {
            return 1 - succ_rate;
        }

        inline float avg_fail_angle_err() const { 
            return (avg_angle_err - succ_rate * avg_succ_angle_err) / fail_rate();
        }

        inline float avg_fail_trans_err() const { 
            return (avg_trans_err - succ_rate * avg_succ_trans_err) / fail_rate();
        }

        inline float avg_fail_angle_sq_err() const { 
            return (avg_angle_sq_err - succ_rate * avg_succ_angle_sq_err) / fail_rate();
        }

        inline float avg_fail_trans_sq_err() const { 
            return (avg_trans_sq_err - succ_rate * avg_succ_trans_sq_err) / fail_rate();
        }

        inline float detect_tp_rate() const {
            return detect_tp / float(detect_tp + detect_fn);
        }

        inline float detect_fp_rate() const {
            return detect_fp / float(detect_fp + detect_tn);
        }

        virtual void serialize(sqlite3* db);
        virtual void deserialize(sqlite3* db);

    };

    struct Experiment : public Serializable {
      
        Experiment() : skip(false), has_run(false) {
            // What's the standard
            paramset = Paramset(); 
            response = Response(); 
        }
 
        Paramset paramset; 
        Response response;
        std::string train_set;
        std::string test_set;
        std::string time;
        std::string vcs_commit;

        /** Specifies whether to skip this experiment when carrying out
         * experiments that have not yet been run. This allows for temporarily
         * disabling experiments. */
        bool skip;
        /** Specifies whether this experiment has already been carried out. In case
         * it has been carried out, and the experiment is serialized to the database
         * column response_id will be a valid reference into table response. If not
         * run yet, column response_id will be set to NULL when serializing. */
        bool has_run;

        void record_time();
        void record_commit();

        virtual void serialize(sqlite3* db);
        virtual void deserialize(sqlite3* db);

    };

    void deserialize_pms_fe(sqlite3* db, tod::FeatureExtractionParams & pms_fe, int64_t & id);
    void serialize_pms_fe(sqlite3* db, const tod::FeatureExtractionParams & pms_fe, int64_t & id);

    void deserialize_pms_match(sqlite3* db, tod::MatcherParameters & pms_match, int64_t & id);
    void serialize_pms_match(sqlite3* db, const tod::MatcherParameters & pms_match, int64_t & id);

    void deserialize_pms_guess(sqlite3* db, tod::GuessGeneratorParameters & pms_guess, int64_t & id);
    void serialize_pms_guess(sqlite3* db, const tod::GuessGeneratorParameters & pms_guess, int64_t & id);

    void selectExperimentsNotRun(sqlite3* & db, std::vector<Experiment> & exps);

    void sortExperimentsByTrainFeatures(std::vector<Experiment> & exps);
    
    typedef std::map<std::string, std::string> MemberMap;
   
    // TODO: use template?
    void setMemberField(MemberMap & m, const std::string & field, float val);
    void setMemberField(MemberMap & m, const std::string & field, double val);
    void setMemberField(MemberMap & m, const std::string & field, int val);
    void setMemberField(MemberMap & m, const std::string & field, int64_t val);
    void setMemberField(MemberMap & m, const std::string & field, bool val);
    void setMemberField(MemberMap & m, const std::string & field, const std::string & val);

    /** No-frills, low-level insert-or-update function that generates a SQL statement. Be careful,
     * not much efforts have been spent into making it safe in any way (you can easily delete the whole
     * table by a crafted SQL-query. Also, be careful with any string data because it is not correctly
     * quoted. */
    // TODO: fix insertOrUpdate and use sqlite3_prepare_v2
    void insertOrUpdate(sqlite3* & db, const std::string & table, const MemberMap & m, int64_t & id);

}

#endif
