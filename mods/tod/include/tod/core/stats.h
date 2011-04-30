#include <time.h>
#include <ostream>

/** Collection of statistics for pose_estimator.cpp. */
struct posest_stats {

    posest_stats();

    /** time (in seconds) necessary for estimating pose on all given images */
    float time;

    /** the number of images pose should be estimated on */
    int img_cnt;

    /** the number of times the pose was estimated successfully */
    int success_cnt;

    /** the number of times the pose could not be estimated */
    int failure_cnt;

    /** the number of times the pose could not be estimated on the
      * original image. */
    int orig_success_cnt;

    /** the number of times the pose could not be estimated on the
      * original image. */
    int orig_failure_cnt;

    /** the number of times the pose was successfully estimated on the
      * fallback image. */
    int fallback_success_cnt;

    /** the number of times the pose could not be estimated on the
      * fallback image. */
    int fallback_failure_cnt;

    float avg_time() const { return time / img_cnt; }
    float success_rate() const { return float(success_cnt) / img_cnt; }
    float failure_rate() const { return float(failure_cnt) / img_cnt; }
    float orig_success_rate() const { return float(orig_success_cnt) / img_cnt; }
    float orig_failure_rate() const { return float(orig_failure_cnt) / img_cnt; }
    float fallback_success_rate() const { return float(fallback_success_cnt) / (fallback_success_cnt + fallback_failure_cnt); }
    float fallback_failure_rate() const { return 1.0 - fallback_success_rate(); }

    void print(std::ostream & out) const;

};

std::ostream & operator<<(std::ostream & out, const posest_stats & s) {
    s.print(out);
    return out;
}

/** Statistics for tod_training/apps/masker.cpp */
struct masker_stats {

    masker_stats();

    /** time (in seconds) necessary for masking all given images */
    float time;

    /** the number of images that should be masked */
    int img_cnt;

    /** the number of times an image has been masked successfully */
    int success_cnt;

    /** the number of times an image masking has failed. */
    int failure_cnt;

    float avg_time() const { return time / img_cnt; }
    float success_rate() const { return float(success_cnt) / img_cnt; }
    float failure_rate() const { return float(failure_cnt) / img_cnt; }

    void print(std::ostream & out) const;

};

std::ostream & operator<<(std::ostream & out, const masker_stats & s) {
    s.print(out);
    return out;
}

/** Statistics for tod_training/apps/detector.cpp */
struct detector_stats {

    detector_stats();

    /** time (in seconds) necessary for extracting features from all given images */
    float time;

    /** the number of images for feature extraction */
    int img_cnt;

    /** the number of times features have been successfully extracted. */
    int success_cnt;

    /** the number of times feature extraction has failed. */
    int failure_cnt;

    // TODO: extract super-class
    float avg_time() const { return time / img_cnt; }
    float success_rate() const { return float(success_cnt) / img_cnt; }
    float failure_rate() const { return float(failure_cnt) / img_cnt; }

    void print(std::ostream & out) const;

};

std::ostream & operator<<(std::ostream & out, const detector_stats & s) {
    s.print(out);
    return out;
}


