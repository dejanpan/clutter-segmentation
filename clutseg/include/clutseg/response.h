/*
 * Author: Julius Adorf
 */

#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "clutseg/ground.h"
#include "clutseg/paramsel.h"
#include "clutseg/result.h"

#include "clutseg/gcc_diagnostic_disable.h"
    #include <map>
    #include <set>
    #include <string>
    #include <tod/detecting/GuessGenerator.h>
#include "clutseg/gcc_diagnostic_enable.h"

namespace clutseg {

    /**
     * \brief Computes the response of the system on a given test set.
     *
     * As such the result has to be compared with ground truth. The smaller the
     * response, the better the guess compared to ground truth. The response
     * function is deliberately defined over the whole test set rather than for
     * a single query to allow to incorporate (and collect) global performance
     * statistics. */
    class ResponseFunction {

        protected:

            ResponseFunction(float max_trans_error = 0.03, float max_angle_error = M_PI / 9) : max_trans_error_(max_trans_error), max_angle_error_(max_angle_error) {}

            /** Basic response function always sets response.value to zero, and
             * populates the other response statistics, such as response.success_rate etc.
             * This method must be called from derived classes. */
            virtual void operator()(const SetResult & result,
                                    const GroundTruth  & ground,
                                    const std::set<std::string> & templateNames,
                                    Response & response);

            float max_trans_error_;
            float max_angle_error_;

    };

    /** \brief Computes the response using the error measured by the sum of squares cut off at a certain threshold. */
    class CutSseResponseFunction : public ResponseFunction {

        public:

            CutSseResponseFunction(float max_trans_error = 0.03, float max_angle_error = M_PI / 9) : ResponseFunction(max_trans_error, max_angle_error) {}

            virtual void operator()(const SetResult & result,
                                    const GroundTruth  & ground,
                                    const std::set<std::string> & templateNames,
                                    Response & response);

    };
}

#endif
