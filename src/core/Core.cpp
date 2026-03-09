#include "Core.h"
#include "Cache.h"
#include "Common.h"
#include "Semantic.h"

#include <mutex>

USE_MODULE(Arcb);

using SymbolMap = Support::AbstractKeywordMap<std::vector<std::string>>;

/**
 * @brief Built-in symbol table used for `arcb::__...__` expansions.
 *
 * Values are stored as strings and can be updated at runtime (e.g. profile selection, threads override).
 */
static SymbolMap builtin_symbols =
{
    { "__main__"       , {                                                     } },
    { "__root__"       , { std::filesystem::current_path().generic_string()    } },
    { "__path__"       , Support::GetPathEntries()                               },
    { "__version__"    , { __ARCB__VERSION__                                 } },
    { "__release__"    , { __ARCB__RELEASE__                                 } },
    { "__profile__"    , {                                                     } },
    { "__threads__"    , {                                                     } },
    { "__max_threads__", { std::to_string(std::thread::hardware_concurrency()) } },


#if defined(_WIN32)
    { "__os__"         , { "windows"                                           } },
#elif defined(__APPLE__) && defined(__MACH__)
    { "__os__"         , { "macos"                                             } },
#elif defined(__linux__)
    { "__os__"         , { "linux"                                             } },
#elif defined(__FreeBSD__)
    { "__os__"         , { "freeBSD"                                           } },
#elif defined(__unix__)
    { "__os__"         , { "unix"                                              } },
#else
    { "__os__"         , { "unknown"                                           } },
#endif

#if defined(__x86_64__) || defined(_M_X64)
    { "__arch__"       , { "x86_64"                                            } },
#elif defined(__i386__) || defined(_M_IX86)
    { "__arch__"       , { "x86"                                               } },
#elif defined(__aarch64__) || defined(_M_ARM64)
    { "__arch__"       , { "aarch64"                                           } },
#elif defined(__arm__) || defined(_M_ARM)
    { "__arch__"       , { "arm"                                               } },
#elif defined(__riscv) || defined(__riscv__)
    { "__arch__"       , { "riscv"                                             } },
#elif defined(__powerpc64__) || defined(__ppc64__)
    { "__arch__"       , { "ppc64"                                             } },
#elif defined(__powerpc__) || defined(__ppc__)
    { "__arch__"       , { "ppc"                                               } },
#else
    { "__arch__"       , { "unknown"                                           } },
#endif
};



/**
 * @brief Map from SymbolType to its canonical token string (e.g. MAIN -> "__main__").
 */
static std::map<Core::SymbolType, std::string> Known_Symbols_By_Token =
{
    { Core::SymbolType::MAIN        , "__main__"        },
    { Core::SymbolType::ROOT        , "__root__"        },
    { Core::SymbolType::VERSION     , "__version__"     },
    { Core::SymbolType::RELEASE     , "__release__"     },
    { Core::SymbolType::PROFILE     , "__profile__"     },
    { Core::SymbolType::THREADS     , "__threads__"     },
    { Core::SymbolType::MAX_THREADS , "__max_threads__" },
    { Core::SymbolType::OS          , "__os__"          },
    { Core::SymbolType::ARCH        , "__arch__"        },
    { Core::SymbolType::PATH        , "__path__"        },
};



/**
 * @brief Map from canonical token string to SymbolType (e.g. "__main__" -> MAIN).
 */
static std::map<std::string, Core::SymbolType> Known_Symbols_By_String =
{
    { "__main__"        , Core::SymbolType::MAIN        },
    { "__root__"        , Core::SymbolType::ROOT        },
    { "__version__"     , Core::SymbolType::VERSION     },
    { "__release__"     , Core::SymbolType::RELEASE     },
    { "__profile__"     , Core::SymbolType::PROFILE     },
    { "__threads__"     , Core::SymbolType::THREADS     },
    { "__max_threads__" , Core::SymbolType::MAX_THREADS },
    { "__os__"          , Core::SymbolType::OS          },
    { "__arch__"        , Core::SymbolType::ARCH        },
    { "__path__"        , Core::SymbolType::PATH        },
};



/**
 * @brief Supported OS names for `@ifos` and internal checks.
 */
static std::vector<std::string> Known_OSs =
{
    "windows",
    "macos",
    "linux",
    "freeBSD",
    "unix",
};



/**
 * @brief Supported architecture names for internal checks.
 */
static std::vector<std::string> Known_ARCHs =
{
    "x86_64",
    "x86",
    "aarch64",
    "arm",
    "riscv",
    "ppc64",
    "ppc",
};



// ============================================================================
// PRIVATE EXECUTION HELPERS
// ============================================================================

/**
 * @brief Execute a single instruction by generating a script and running it via the chosen engine.
 *
 * On Windows, if the engine is `cmd.exe`, a `.bat` script is generated and executed with `/d /s /c`.
 * On other interpreters/OSes, a plain script is generated and passed as an argument.
 *
 * @param jobname     Task/job name (used for cache script naming).
 * @param idx         Instruction index within the job.
 * @param engine Executor executable path.
 * @param command     Command text to persist into the script.
 * @param echo        If true, print the command before executing.
 * @return InstructionResult containing command and exit code.
 */
static Core::InstructionResult run_instruction(const std::string& jobname,
                                               const std::size_t  idx,
                                               Semantic::Executor& engine,
                                               const std::string& command,
                                               const bool         echo) noexcept  
{
    std::string             full_cmd;
    std::filesystem::path   script;
    Core::InstructionResult res { command, 0 };
    Cache::Manager&         cache = Cache::Manager::Instance();

    // OPTIONALLY ECHO COMMAND
    if (echo)
    {
        MSG(command);
    }

    if (engine.type == Semantic::Executor::Type::FILE)
    {
        engine.argument = cache.WriteScript(jobname, idx, command, engine.ext);
    }
    else
    {
        engine.argument = command;
    }

    const auto instruction = engine.Get_Repr();

    // DBG(instruction);

    // EXECUTE COMMAND
    int ret = std::system(instruction.data());

    // NORMALIZE EXIT CODE
    if (ret == -1)
    {
        res.exit_code = 127;
    }
    else
    {
#if defined(_WIN32)
    res.exit_code = ret;
#else
    if (WIFEXITED(ret))
    {
        res.exit_code = WEXITSTATUS(ret);
    }
    else if (WIFSIGNALED(ret))
    {
        res.exit_code = 128 + WTERMSIG(ret);
    }
    else
    {
        res.exit_code = 127;
    }
#endif
    }

    return res;
}



/**
 * @brief Execute a job either linearly or by spawning a thread per instruction.
 *
 * The decision is driven by `job.parallelizable`. In MT mode, results are stored by index.
 *
 * @param job Job to run.
 * @param opt Runtime options (stop-on-error, max parallelism, etc.).
 * @return Core::Result containing per-instruction results and summary status.
 */
static Core::Result run_job(Jobs::Job& job, const Core::RunOptions& opt) noexcept
{
    Core::Result result {};

    // INIT RESULT HEADER
    result.name        = job.name;
    result.ok          = true;
    result.first_error = 0;

    // LINEAR EXECUTION
    if (!job.parallelizable)
    {
        std::size_t idx = 0;

        for (const auto& cmd : job.instructions)
        {
            // RUN INSTRUCTION AND STORE RESULT
            auto r = run_instruction(job.name, idx, job.engine, cmd, job.echo);
            result.results.push_back(r);

            // HANDLE ERROR
            if (r.exit_code != 0)
            {
                result.ok          = false;
                result.first_error = r.exit_code;

                if (opt.stop_on_error)
                {
                    break;
                }
            }

            ++idx;
        }
    }
    else
    {
        // PARALLEL EXECUTION (THREAD PER INSTRUCTION, WITH BATCH JOIN)
        std::vector<std::thread> threads;
        std::mutex               mutex;

        result.results.resize(job.instructions.size());

        // WORKER: RUN ONE INSTRUCTION AND WRITE RESULT SAFELY
        auto worker = [&] (std::size_t idx)
        {
            const auto& cmd = job.instructions[idx];
            auto        r   = run_instruction(job.name, idx, job.engine, cmd, job.echo);

            std::lock_guard<std::mutex> lock(mutex);
            result.results[idx] = r;
        };

        // SPAWN THREADS UP TO MAX PARALLELISM
        for (std::size_t i = 0; i < job.instructions.size(); ++i)
        {
            threads.emplace_back(worker, i);

            if (threads.size() >= opt.max_parallelism)
            {
                // JOIN CURRENT BATCH
                for (auto& t : threads)
                {
                    t.join();
                }

                threads.clear();
            }
        }

        // JOIN REMAINING THREADS
        for (auto& t : threads)
        {
            t.join();
        }

        // AGGREGATE ERRORS
        for (const auto& r : result.results)
        {
            if (r.exit_code != 0)
            {
                result.ok = false;

                if (result.first_error == 0)
                {
                    result.first_error = r.exit_code;
                }
            }
        }
    }

    return result;
}

 

// ============================================================================
// CORE API
// ============================================================================

/**
 * @brief Execute the job list in order and return the collected results.
 *
 * Prints per-task progress and summary unless `opt.silent` is enabled.
 *
 * @param jobs Job list to execute.
 * @param opt Execution options.
 * @return ARCB_RESULT__OK on success, otherwise a failure code.
 */
Arcb_Result Core::run_jobs(Jobs::List& jobs, const Core::RunOptions& opt) noexcept
{
    Arcb_Result result = Arcb_Result::ARCB_RESULT__OK;
    Stopwatch sw;

    // START TIMER
    sw.start();

    // RUN EACH JOB
    for (auto& job : jobs.All())
    {
        if (opt.verbose)
        {
            ARC("Running task: " << ANSI_BGREEN <<  job.name << ANSI_RESET);
        }

        // RUN JOB AND COLLECT RESULT 
        auto r = run_job(job, opt);

        // STOP EARLY ON ERROR IF REQUESTED
        if (!r.ok && opt.stop_on_error)
        {
            result = Arcb_Result::ARCB_RESULT__NOK;
            ERR("Task failed: " << ANSI_BRED <<  job.name << ANSI_RESET);
            break;
        }

        if (job.cache.enabled && job.cache.type == Semantic::InstructionTask::Cache::Type::STORE)
        {
            for (auto& file : job.cache.data)
            {
                Arcb::Cache::Manager::Instance().Store(file);
            }
        }

        if (job.death)
        {
            result = Arcb_Result::ARCB_RESULT__NOK;
            ERR("Task failed: " << ANSI_BRED <<  jobs.main_job << ANSI_RESET );
            break;
        }
    }

    // STOP TIMER
    sw.stop();
    auto ms = sw.elapsed<>();

    // PRINT SUMMARY
    if (result == Arcb_Result::ARCB_RESULT__OK)
    {
        ARC("Action '" << jobs.main_job << "' done in " << Stopwatch::format(ms));
    }

    return result;
}


 
/**
 * @brief Get the mutable value associated to a built-in symbol type.
 * @param type Symbol type.
 * @return Reference to the stored string.
 */
std::vector<std::string> Core::symbol(Core::SymbolType type) noexcept
{
    // MAP TYPE TO TOKEN STRING
    const std::string symbol = Known_Symbols_By_Token[type];

    // RETURN STORED VALUE
    return builtin_symbols[symbol];
}



std::string Core::first_symbol(Core::SymbolType type) noexcept
{
    // MAP TYPE TO TOKEN STRING
    const std::string symbol = Known_Symbols_By_Token[type];

    // RETURN STORED VALUE
    return builtin_symbols[symbol].empty() ? "" : builtin_symbols[symbol][0];
}



/**
 * @brief Check whether a string is a known built-in symbol token and return its type.
 * @param symbol Token string (e.g. "__main__").
 * @return SymbolType or UNDEFINED.
 */
Core::SymbolType Core::is_symbol(const std::string& symbol) noexcept
{
    Core::SymbolType type = Core::SymbolType::UNDEFINED;

    // LOOKUP TOKEN STRING
    if (auto it = Known_Symbols_By_String.find(symbol); it != Known_Symbols_By_String.end())
    {
        type = it->second;
    }

    return type;
}



/**
 * @brief Update a built-in symbol value.
 * @param type Symbol type.
 * @param val  New value.
 */
void Core::update_symbol(Core::SymbolType type, const std::string& val) noexcept
{
    // MAP TYPE TO TOKEN STRING
    const std::string symbol = Known_Symbols_By_Token[type];

    // UPDATE STORED VALUE
    if (type == Core::SymbolType::PATH)
    {
        builtin_symbols[symbol].push_back(val);
    }
    else
    {
        if (builtin_symbols[symbol].empty())
        {
            builtin_symbols[symbol].push_back(val);
        } 
        else
        {
            builtin_symbols[symbol][0] = val;
        }
    }

    return;
}



/**
 * @brief Check if a sybol is set.
 * @param type Symbol type.
 * @return True if supported.
 */
bool Core::is_symbol_set(Core::SymbolType type) noexcept
{
    // MAP TYPE TO TOKEN STRING
    const std::string symbol = Known_Symbols_By_Token[type];

    return !builtin_symbols[symbol].empty();
}



/**
 * @brief Check if a string matches a known OS name.
 * @param param OS name.
 * @return True if supported.
 */
bool Core::is_os(const std::string& param) noexcept
{
    // LINEAR SEARCH IN KNOWN OS LIST
    return (std::find(Known_OSs.begin(), Known_OSs.end(), param) != Known_OSs.end());
}



/**
 * @brief Check if a string matches a known architecture name.
 * @param param Arch name.
 * @return True if supported.
 */
bool Core::is_arch(const std::string& param) noexcept
{
    // LINEAR SEARCH IN KNOWN ARCH LIST
    return (std::find(Known_ARCHs.begin(), Known_ARCHs.end(), param) != Known_ARCHs.end());
}
