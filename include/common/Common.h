#ifndef __ARCB_COMMON_H__
#define __ARCB_COMMON_H__


#include <string>
#include <chrono>
#include <sstream>


/**
 * @brief Utility class for measuring elapsed time.
 *
 * Stopwatch measures the time interval between a call to start() and stop().
 * If stop() is not called, elapsed() returns the time elapsed up to the current instant.
 *
 * Internally uses std::chrono::steady_clock to guarantee monotonic behavior.
 */
class Stopwatch
{
public:
    using clock = std::chrono::steady_clock;

    /**
     * @brief Starts the stopwatch.
     *
     * Resets the start time and marks the stopwatch as running.
     */
    void start()
    {
        running_ = true;
        t0_      = clock::now();
    }

    /**
     * @brief Stops the stopwatch.
     *
     * If the stopwatch is running, records the stop time and
     * freezes the elapsed duration.
     */
    void stop()
    {
        if (running_)
        {
            t1_      = clock::now();
            running_ = false;
        }
    }

    /**
     * @brief Returns the elapsed time.
     *
     * If the stopwatch is still running, the elapsed time is computed
     * up to the current instant. Otherwise, it is computed up to the
     * last call to stop().
     *
     * @tparam Dur Duration type used for the result
     *             (defaults to std::chrono::milliseconds).
     * @return Elapsed time expressed in the requested duration unit.
     */
    template <typename Dur = std::chrono::milliseconds>
    long long elapsed() const
    {
        auto end = running_ ? clock::now() : t1_;
        return std::chrono::duration_cast<Dur>(end - t0_).count();
    }


    /**
     * @brief Formats a time duration into a human-readable string.
     *
     * Durations below one second are formatted in milliseconds,
     * otherwise they are formatted in seconds.
     *
     * @param time Time duration in milliseconds.
     * @return Human-readable representation of the duration.
     */
    static std::string format(long long time)
    {
        std::stringstream ss;
        if (time < 1000)
        {   
            ss << time << " milliseconds";
            return ss.str();
        }
        else
        {
            double formatted = (double) time / 1000;
            ss << formatted << " seconds";
            return ss.str();
        }
    }

private:
    clock::time_point t0_{};                ///< Start time.
    clock::time_point t1_{};                ///< Stop time.
    bool              running_{false};      ///< Indicates whether the stopwatch is running.
};







#endif /* __ARCB_COMMON_H__ */