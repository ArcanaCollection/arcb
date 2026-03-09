#include "Glob.h"
#include "Jobs.h"
#include "Core.h"
#include "Cache.h"
#include "Parser.h"
#include "Support.h"
#include "Defines.h"
#include "Semantic.h"
#include "Profiler.h"
#include "TableHelper.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <filesystem>


USE_MODULE(Arcb);





//     █████╗ ██████╗  ██████╗██████╗ 
//    ██╔══██╗██╔══██╗██╔════╝██╔══██╗
//    ███████║██████╔╝██║     ██████╔╝
//    ██╔══██║██╔══██╗██║     ██╔══██╗
//    ██║  ██║██║  ██║╚██████╗██████╔╝
//    ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═════╝ 
//                                    



#define CHECK_RESULT(op)                    if (auto res = op; res != Arcb_Result::ARCB_RESULT__OK) { return (res == Arcb_Result::ARCB_RESULT__NOK) ? Arcb_Result::ARCB_RESULT__NOK : Arcb_Result::ARCB_RESULT__OK; }
#define CHECK_STR_RESULT(op)                if (auto res = op; res.has_value())                       { ERR(res.value()); return Arcb_Result::ARCB_RESULT__NOK; }


static Semantic::Enviroment env;




/**
 * @brief Parse and process the Arcb source file.
 *
 * This function performs lexical analysis, parsing, semantic validation,
 * environment alignment, variable expansion, and assertion execution.
 *
 * @param args Parsed command-line arguments.
 * @return Arcb_Result::ARCB_RESULT__OK on success, NOK on failure.
 */
static Arcb_Result Parse(const Support::Arguments& args)
{
    // INITIALIZE LEXER, GRAMMAR ENGINE, AND PARSER.
    Scan::Lexer          lexer(args.arcfile);
    Grammar::Engine      engine;
    Parsing::Parser      parser(lexer, engine);
    
    // PARSE INPUT FILE AND BUILD ENVIRONMENT STATE.
    CHECK_RESULT(parser.Parse(env));

    // VALIDATE CLI ARGUMENTS AGAINST ENVIRONMENT (PROFILES, THREADS, MAIN TASK, ETC).
    CHECK_RESULT(env.CheckArgs(args));

    // ALIGN ENVIRONMENT TABLES AND DEFAULTS.
    CHECK_RESULT(env.AlignEnviroment());

    // EXPAND VARIABLES, GLOBS, AND ATTRIBUTE-DRIVEN TRANSFORMS.
    CHECK_RESULT(env.Expand());

    // CHECK FOR PUBLIC TASKS PRESENCE.
    if (!Table::GetValues(env.ftable, Semantic::Attr::Type::PUBLIC))
    {
        std::stringstream ss;
        ss << "Arcfile " << TOKEN_MAGENTA(args.arcfile) << " has no public tasks"; 
        ERR(ss.str());
        return Arcb_Result::ARCB_RESULT__NOK;
    }

    return Arcb_Result::ARCB_RESULT__OK;
}



/**
 * @brief Build the job list from the environment and execute it.
 *
 * @param args Parsed command-line arguments.
 * @return Arcb_Result::ARCB_RESULT__OK on success, NOK on failure.
 */
static Arcb_Result Execute(const Support::Arguments& args)
{
    Jobs::List               joblist;
    Core::RunOptions         runopt;
    std::vector<std::string> recovery;
    Arcb_Result            result;

    if (args.verbose) ARC(ANSI_GRAY << "Executing Environment" << ANSI_RESET);

    // EXECUTE ASSERT_MSG STATEMENTS.
    if (env.ExecuteAsserts(recovery) != Arcb_Result::ARCB_RESULT__OK)
    {
        if (recovery.size() == 0)
        {
            return Arcb_Result::ARCB_RESULT__NOK;
        }
    } 

    // BUILD JOBLIST FROM CURRENT ENVIRONMENT.
    CHECK_RESULT(Jobs::List::FromEnv(env, joblist, recovery));

    // HANDLE POST PARSE EARLY-EXIT OPTIONS.
    CHECK_RESULT(Support::HandleArgsPostParse(args, env, joblist));

    // CONFIGURE RUNTIME EXECUTION OPTIONS.
    runopt.verbose         = args.verbose;
    runopt.max_parallelism = env.GetThreads();

    // EXECUTE JOBS
    result = Core::run_jobs(joblist, runopt);

    if (result == Arcb_Result::ARCB_RESULT__OK)
    {
        Cache::Manager::Instance().Freeze();
    }

    return result;
}
                                                                                                   
                                                                                                                                                                                      
                                                                    
/**
 * @brief Arcb program entry point.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char** argv)  
{   
    Support::Arguments args;

    // PARSE COMMAND-LINE ARGUMENTS INTO `args`.
    CHECK_RESULT(Support::ParseArgs(argc, argv, args));

    // HANDLE PRE PARSE EARLY-EXIT OPTIONS AND VALIDATE INPUTS.
    CHECK_RESULT(Support::HandleArgsPreParse(args));

    if (args.verbose) ARC(ANSI_GRAY << "Building Environment" << ANSI_RESET);

    // PARSE ARCFILE AND PREPARE THE SEMANTIC ENVIRONMENT.
    CHECK_RESULT(Parse(args));

    // LOAD CACHE.
    Cache::Manager::Instance().LoadCache(env.GetProfile().selected);

    // GENERATE JOBLIST AND EXECUTE.
    return Execute(args);
}