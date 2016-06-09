/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2016 Joshua C. Klontz                                           *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef LIKELY_TIMER_HPP
#define LIKELY_TIMER_HPP

#include <algorithm>
#include <ctime>
#include <functional>
#include <limits>

/*!
 * \defgroup timer Timer
 * \brief Measure function execution speed (\c likely/timer.hpp).
 *
 * Unlike the primary Likely API, this is a \c C++ API.
 * As a result, they follow a \c camelCase naming convention and you must explicitly <tt>\#include \<likely/timer.hpp\></tt>.
 *
 * Measure function speed using \ref LikelyTimer::measure, compare function speeds using \ref LikelyTimer::speedup.
 * @{
 */

/*!
 * \brief Function execution speed statistics.
 */
struct LikelyTimer
{
    size_t numIterations; /*!< \brief Number of iterations the function was called. */
    clock_t duration; /*!< \brief Total elapsed processor time during which the function was called. */
    clock_t fastestIteration; /*!< \brief Fastest recorded elapsed processor time of the function. */
    float Hz; /*!< \brief Function executions per second. */

    /*!
     * \brief Measure a function's execution speed.
     * \param function Function to measure the execution speed of.
     * \param duration Length of time to perform the test, default is one second.
     * \return Execution speed statistics.
     */
    static LikelyTimer measure(const std::function<void()> &function, clock_t duration = CLOCKS_PER_SEC)
    {
        LikelyTimer t;
        t.numIterations = 0;
        t.fastestIteration = std::numeric_limits<clock_t>::max();

        clock_t startTime, endTime;
        startTime = endTime = clock();
        while (endTime-startTime < duration) {
            function();
            const clock_t current = clock();
            t.numIterations++;
            t.fastestIteration = std::min(t.fastestIteration, current - endTime);
            endTime = current;
        }

        t.duration = endTime - startTime;
        t.Hz = float(t.numIterations) * CLOCKS_PER_SEC / t.duration;
        return t;
    }

    /*!
     * \brief Compare two benchmarks to measure the speedup factor.
     * \param before Baseline speed statistics.
     * \param after New speed statistics.
     * \return The speedup factor.
     */
    static float speedup(const LikelyTimer &before, const LikelyTimer &after)
    {
        if (std::min(before.fastestIteration, after.fastestIteration) < 10)
            return after.Hz / before.Hz; // Fallback to average speedup for very fast functions
        return float(before.fastestIteration) / float(after.fastestIteration);
    }
};

/** @} */ // end of timer

#endif // LIKELY_TIMER_HPP
