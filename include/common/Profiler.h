#ifndef ARCB_PROFILER_H
#define ARCB_PROFILER_H

#include <cstdint>
#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdio>

namespace Arcb
{
namespace Profile
{
struct Stats
{
    uint64_t count    = 0;
    uint64_t total_ns = 0;
    uint64_t min_ns   = 0;
    uint64_t max_ns   = 0;
};

#if defined(ARC_PROFILE)

inline std::mutex& Mutex()
{
    static std::mutex m;
    return m;
}

inline std::unordered_map<std::string, Stats>& Table()
{
    static std::unordered_map<std::string, Stats> t;
    return t;
}

inline void AddSample(const char* name, uint64_t ns)
{
    std::lock_guard<std::mutex> lock(Mutex());
    Stats& s = Table()[name];

    s.count++;
    s.total_ns += ns;

    if (s.min_ns == 0 || ns < s.min_ns) { s.min_ns = ns; }
    if (ns > s.max_ns)                  { s.max_ns = ns; }
}

class ScopeTimer
{
public:
    explicit ScopeTimer(const char* name) noexcept
        : m_name(name)
        , m_start(std::chrono::steady_clock::now())
    {}

    ~ScopeTimer() noexcept
    {
        const auto end = std::chrono::steady_clock::now();
        const auto ns  = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start).count();
        AddSample(m_name, ns);
    }

private:
    const char* m_name;
    std::chrono::steady_clock::time_point m_start;
};

inline void Dump(FILE* out = stderr)
{
    std::lock_guard<std::mutex> lock(Mutex());

    auto& table = Table();

    if (table.size() == 0) return;

    std::fprintf(out, "\n=== ARCB PROFILE DUMP ===\n");
    for (const auto& kv : table)
    {
        const char* name = kv.first.c_str();
        const Stats& s   = kv.second;

        const double total_ms = (double)s.total_ns / 1e6;
        const double avg_us   = (s.count > 0) ? ((double)s.total_ns / (double)s.count / 1e3) : 0.0;
        const double min_us   = (double)s.min_ns / 1e3;
        const double max_us   = (double)s.max_ns / 1e3;

        std::fprintf(out,
                     "%-40s  calls=%llu  total=%.3f ms  avg=%.3f us  min=%.3f us  max=%.3f us\n",
                     name,
                     (unsigned long long)s.count,
                     total_ms,
                     avg_us,
                     min_us,
                     max_us);
    }
    std::fprintf(out, "==========================\n");
}

#else

class ScopeTimer
{
public:
    explicit ScopeTimer(const char*) noexcept {}
};

inline void Dump(FILE* = stderr) {}

#endif

} // namespace Profile
} // namespace Arcb

#if defined(ARC_PROFILE)
    #define ARC_PROFILE_SCOPE(id,name) ::Arcb::Profile::ScopeTimer arc_profiler__scope__##id(name)
#else
    #define ARC_PROFILE_SCOPE(id,name) do { } while (0)
#endif

#endif
