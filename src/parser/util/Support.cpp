#include "Jobs.h"
#include "Cache.h"
#include "Lexer.h"
#include "Support.h"
#include "Grammar.h"
#include "Semantic.h"
#include "Generator.h"
#include "TableHelper.h"

#include <limits>
#include <sstream>
#include <cstddef>
#include <variant>
#include <charconv>
#include <algorithm>


USE_MODULE(Arcb);





//    ██████╗ ██████╗ ██╗██╗   ██╗ █████╗ ████████╗███████╗    ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
//    ██╔══██╗██╔══██╗██║██║   ██║██╔══██╗╚══██╔══╝██╔════╝    ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
//    ██████╔╝██████╔╝██║██║   ██║███████║   ██║   █████╗      █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
//    ██╔═══╝ ██╔══██╗██║╚██╗ ██╔╝██╔══██║   ██║   ██╔══╝      ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
//    ██║     ██║  ██║██║ ╚████╔╝ ██║  ██║   ██║   ███████╗    ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
//    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝  ╚═╝  ╚═╝   ╚═╝   ╚══════╝    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝
//                                                                                                        


/**
 * @brief Arcb CLI banner printed on startup and help/version output.
 */
static const char* ARCB_HEADER = 
#if defined(_WIN32)
R"HEADER(
         Arcb - the modern alternative to make.
)HEADER";
#else
R"HEADER(
         █████╗ ██████╗  ██████╗██████╗ 
        ██╔══██╗██╔══██╗██╔════╝██╔══██╗
        ███████║██████╔╝██║     ██████╔╝
        ██╔══██║██╔══██╗██║     ██╔══██╗
        ██║  ██║██║  ██║╚██████╗██████╔╝
        ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═════╝ 
                                         
      arcb — the modern alternative to make.
)HEADER";
#endif 


/**
 * @brief Print Arcb banner and version.
 * @return Arcb_Result::ARCB_RESULT__OK
 */
static Arcb_Result Version(void)
{
    MSG(ARCB_HEADER);
    MSG("Version: " << __ARCB__VERSION__ << " (" << __ARCB__RELEASE__ << ")");

    return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
}



/**
 * @brief Print the Arcb command-line help message.
 * @return Arcb_Result::ARCB_RESULT__OK
 */
static Arcb_Result Help(void)
{
    static const char* ARCB_HELP = R"HELP(

DESCRIPTION
  Arcb lets you build your project in a simple and modern way.
  By defining tasks, statements, and variables, characterizing them with attributes, 
  you'll be able to define the main building steps of your project yourself, 
  in the cleanest possible form.


USAGE
  arcb [task] [options]
  arcb --help
  arcb --version


OPTIONS
  --help                Show this help message, then exit.
  --version             Print the arcb version, then exit.
  --flush-cache         Flush arcb cache, then exit.
  --silent              Suppress Arcb runtime logs on stdout.
  -p <profile>          Execute the arcfile with a specific profile. 
                        Profiles must be declared in the arcfile, via 'using profiles' statement. 
  -s <arcfile>          Execute the CLI passed arcfile. 
  -t <numofthreads>     Explict pass via CLI the wanted threads. This option will override the
                        'using threads' statement.
  --generate [stream]   Generate an arcfile template. If a stream is passed the template will be
                        saved into it.
                        If the stream is stdout, the template will be printed on it. 
  --value <ITEM>        Show ITEM value. The ITEM can be a task, a variable or a symbol.
  --pubs                Show public tasks.
  --profiles            Show registered profiles. 


LANGUAGE:
  It's a deliberately lightweight grammar, to avoid the complexities of other builders.
  It allows the use of native Arcb statements, variable declarations, and tasks, 
  with the ability to be customized through attributes that define their behavior and execution order.
  In particular, body tasks are grammar-less, meaning no control over their content is performed.
  This is because we wanted to offer users the freedom to use their preferred engine 
  to execute the instructions.
  This means no custom statements like if/for/while, no strange symbols, and no overly complex syntax.
  The only exception is the ability to expand variables declared in Arcb within task statements.

  NATIVE STATEMENTS:
    import <file.arc>                               Imports an arcscript as arcb source file.
    
    using profiles <Profile list>                   Allows the user to define a set of profiles to use 
                                                    in the arcb code.
                                                    Any use of profiles not declared in this way will 
                                                    raise an error.

    using engine <type> [ext] <path>                Allows the user to define the default engine 
                                                    for task bodies. 
                                                    By default, /bin/bash will be used.
                                                    See attribute @engine for more. 
    
    using threads <max threads number>              Allows the user to define the number of threads on 
                                                    which to parallelize the execution of a specific task.
                                                    Omitting this statement will result in the use of all 
                                                    the cores on your machine.

    map <SOURCE> -> <TARGET>                        Same as attribute @map. 

    assert "lvalue" <op> "rvalue" -> "reason"       Executes assert operation with early-exit with 
                                                    reason "reason". 

    assert "lvalue" <op> "rvalue" -> <CB List>      Executes assert operation without early-exit and
                                                    invokes the recovery callbacks list <CB List>. 

  
  BUILTIN SYMBOLS:
    In Arcb there are builtin symbols:

    __main__                        A symbol that identifies the name of the main task.
                                    It represents the entry point of the execution graph.

    __root__                        A symbol that identifies the absolute path of the project root.
                                    The project root is defined as the directory containing the main Arcb file.

    __version__                     A symbol that identifies the current version of Arcb.
                                    It can be used for compatibility checks and diagnostics.

    __release__                     A symbol that carry on the current release name. Just for fun.

    __profile__                     A symbol that identifies the currently selected execution profile.
                                    If no profile is selected, it will have the value 'None'.

    __threads__                     A symbol that identifies the number of threads effectively used
                                    for task execution at runtime.

    __max_threads__                 A symbol that identifies the maximum number of threads allowed
                                    for task execution, as determined by system capabilities and configuration.

    __os__                          A symbol that identifies the target operating system.
                                    The value is determined at compile time and is platform independent.

    __arch__                        A symbol that identifies the target CPU architecture.
                                    The value is determined at compile time and is platform independent.

    __path__                        Operating System path variable content.


  VARIABLES:
    NAME = VALUE                    Simple assignment of VALUE into NAME

    @glob
    GLOB = path/**/*.c              Simple assignment of path/**/*.c into GLOB, but at runtime
                                    the engine will try to expand the glob **/*.c
    @map GLOB
    VAR  = path2/**/*.o             Using the @map X attribute on a glob variable Y will generate 
                                    a mapping of X to Y


  TASKS:
    task Name(INPUT_PARAMS)         
    {
        instructions...
    }

    A task declaration follows the linear semantics of 'task NAME() { OPTIONAL_STATEMENTS }'.
    The inputs are not related to the body of the task itself; they only tell arcb that this task 
    handles these data sets.
    This is to keep track of which statements can be avoided in the cache because they have not changed.
    A task can have 0 inputs and 0 statements.
    If it has 0 statements, it will be optimized by eliminating the task itself, but through the use of 
    attributes like @then, @after, and @pub, it becomes a wrapper that allows the invocation of private tasks.
    As mentioned above, the only task statement management for arcb translates into the expansion 
    of arcb variables.
    Here too, the logic is quite simple.

    VARIABLES EXPANSION:
      There are various types of expansion:

      1) simple expansion

        Follows the grammar arcb::VARNAME and results in a textual substitution
        with the contents of the referenced variable.
  
      2) inline method expansion
  
        Follows the grammar arcb::VARNAME.inline() and evaluates a method on the
        referenced variable, producing an inline textual result inside the
        current statement.
  
      3) list expansion
  
        Follows the grammar arcb::VARNAME.list([l[, r]]).
    
        This expansion transforms the current statement into multiple sibling
        statements, one for each element selected from the variable.
    
        The optional parameters control the selected range:
    
            list()       → expands over the entire variable
            list(r)      → expands from index 0 to r
            list(l, r)   → expands from index l to r
    
        If multiple list expansions are present in the same statement, arcb
        checks whether their effective ranges produce compatible sizes. If so,
        the statement is expanded by performing an inner join across the selected
        elements.
    
        Variables assigned multiple times using the '+=' operator are implicitly
        treated as lists. Scalar variables are automatically coerced into a
        single-element list when used with list().

      Other supported methods include:
    
        size()   → returns the number of elements of the variable as a string
        empty()  → returns 1 if the variable is empty, otherwise 0
    
        Methods do not support chaining.

      For glob expansions, if the passed variable is not a glob, its nominal content will be used.


  ATTRIBUTES:
    Attributes allow you to customize variables and, above all, tasks as much as possible.

    @map                            Valid only for variables. Allows you to map one glob to another.

    @ifos        <os>               Valid only for variables. Enables the annotated variables or 
                                    tasks only when the host OS matches <os>.

    @pub                            Exports task to the caller. By defaults all symbols are private.

    @main                           Marks the task as main task.

    @glob                           Marks the variable as glob.

    @echo                           Prints at runtime on stdout the executed task instructions.

    @then        <task list>        After the execution of the task with the after attribute, 
                                    the specified tasks will be called.

    @death                          Executing a task marked with 'death' aborts the entire execution.

    @requires    <task list>        Before the execution of the task with the after attribute, 
                                    the specified tasks will be called.

    @exclude     <VARNAME>          Used primarily for glob expansions. It allows you to perform 
                                    subtraction between sets by subtracting the value of VARNAME 
                                    from the variable characterized by this attribute.

    @always                         Execute the task regardless of job scheduling.

    @profile     <profile>          Restricts the annotated variables or tasks to the specified 
                                    build profile.

    @cache <command> <var list>     Untrack, Track and Store cache, see more in CACHE.

    @engine <type> [ext] <path>     Execute the task using the specified engine.
                                    type='raw'  → 'ext' not required. The task body is passed directly 
                                                to the executable defined by 'path'.

                                    type='file' → 'ext' required (e.g. .py, .sh). The task body is written 
                                                to a script file with the specified extension and executed 
                                                using 'path'.
                                    Optional executable flags may follow the 'path' parameter.
    
    @threading                      Enable the multithread for the selected task, not guaranteed.

CACHE:
    @cache <command> <var list>

    Manage input file cache.

    Commands:
    track    Track files resolved from the given variables.
    untrack  Remove resolved files from cache.
    store    Store current state of resolved files into cache.

    Variables:
    Variables must use Arcb expansion syntax:

        arcb::VARNAME
            Use variable value as file path (single value or textual list).

        arcb::VARNAME.list([l[, r]])
            Expand variable using LIST algorithm.
            If VARNAME has a glob expansion, all resolved files are used.

    Notes:
    - <var list> accepts one or more variables.

EXAMPLES:
  arcb
  arcb <TASK>
  arcb <TASK> -p Debug
  arcb <TASK> -p Debug -t 1
  arcb --flush-cache
  arcb --pubs
  arcb --value <TASK>
  arcb --generate stdout
)HELP";

    MSG(ARCB_HEADER);
    MSG(ARCB_HELP);

    return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
}



/**
 * @brief Compute the Levenshtein distance between two strings.
 *
 * The Levenshtein distance is the minimum number of single-character edits
 * (insertions, deletions, or substitutions) required to change one string
 * into the other.
 *
 * @param a First string.
 * @param b Second string.
 * @return Edit distance between @p a and @p b.
 */
std::size_t Support::levenshtein_distance(const std::string_view& a,
                                 const std::string_view& b) noexcept
{
    const std::size_t len1 = a.size();
    const std::size_t len2 = b.size();

    if (len1 == 0)
        return len2;
    if (len2 == 0)
        return len1;

    std::vector<std::size_t> prev(len2 + 1);
    std::vector<std::size_t> curr(len2 + 1);

    for (std::size_t j = 0; j <= len2; ++j)
        prev[j] = j;

    for (std::size_t i = 0; i < len1; ++i)
    {
        curr[0] = i + 1;

        for (std::size_t j = 0; j < len2; ++j)
        {
            const std::size_t cost = (a[i] == b[j]) ? 0u : 1u;

            const std::size_t del    = prev[j + 1] + 1;   // cancellazione
            const std::size_t ins    = curr[j] + 1;       // inserzione
            const std::size_t subst  = prev[j] + cost;    // sostituzione

            curr[j + 1] = std::min({ del, ins, subst });
        }

        std::swap(prev, curr);
    }

    return prev[len2];
}





                                                                                             
//    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗    ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
//    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝    ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
//    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗    █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
//    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║    ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
//    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║    ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
//    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝
//                                                                                                              



/**
 * @brief Format and print a syntax error produced by the parsing engine.
 *
 * @param ctx Source context (typically a file path).
 * @param match Grammar match containing error information.
 * @return Arcb_Result::ARCB_RESULT__NOK
 */
Arcb_Result Support::Parser_Error(const std::string& ctx, const Grammar::Match& match, Scan::Lexer& lexer)
{
    const auto& [token, found, semtypes, _] = match.Error;

    std::string       escaping;
    std::string       s(token.start, '~');
    std::string       symbol(token.end, '^');
    std::stringstream ss;

    ss << "[" << ANSI_BRED << "ERROR" << ANSI_RESET << "] " <<  "In file " << ANSI_FG(217, 150, 38) << ANSI_BOLD << ctx << ANSI_RESET 
       << ":" << ANSI_FG(217, 150, 38) << ANSI_BOLD << token.line << ANSI_RESET 
       << " near the statement:\n" 
       << ANSI_FG(252, 240, 190)  << lexer[token] << ANSI_RESET << std::endl;

    // BUILD A VISUAL DIAGNOSTIC LINE WITH A CARET RANGE UNDER THE OFFENDING TOKEN.
    ss << TOKEN_RED(s << symbol) << std::endl << std::endl;

    // NORMALIZE SPECIAL LEXEMES FOR OUTPUT.
    escaping = (token.lexeme == "\n") ? "<New Line>" : token.lexeme;

    if (token.type == Scan::TokenType::UNKNOWN)
    {
        // REPORT UNKNOWN SYMBOLS AS UNDEFINED INPUT.
        ss << "        Found undefined symbol: " << escaping  << std::endl;
    }
    else
    {
        // PRINT FOUND TOKEN AND EXPECTED NON-TERMINALS / RULES.
        ss << "Found:    " << TOKEN_RED(escaping) << " (" << Support::TokenTypeRepr(token.type) << ")" << std::endl;
        ss << "Expected: " << Support::UniqueNonTerminalRepr(found) << " for statement(s): ";
        
        uint32_t i = 0;
        for (const auto& stmt : semtypes)
        {
            if (i) ss << ", ";
            ss << TOKEN_CYAN(Support::RuleRepr(stmt));
            i++;
        }
        ss << std::endl;
    }
    
    // FLUSH DIAGNOSTIC MESSAGE TO STDERR.
    std::cerr << ss.str();

    return Arcb_Result::ARCB_RESULT__NOK;
}




/**
 * @brief Case-insensitive hash for std::string_view based on ASCII lowering.
 *
 * @param s Input string view.
 * @return Hash value.
 */
std::size_t Support::StringViewHash::operator() (std::string_view s) const noexcept
{
    std::size_t h = 0;

    for (char c : s)
    {
        // UPDATE HASH USING LOWERCASED ASCII CHARACTER.
        h = h * 131 + Support::toLowerAscii(c); 
    }

    return h;
}



/**
 * @brief Case-insensitive equality for std::string_view based on ASCII lowering.
 *
 * @param a First string view.
 * @param b Second string view.
 * @return true if equal (case-insensitive ASCII), false otherwise.
 */
bool Support::StringViewEq::operator() (std::string_view a, std::string_view b) const noexcept
{
    if (a.size() != b.size())
    {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i)
    {
        if (Support::toLowerAscii(a[i]) != Support::toLowerAscii(b[i]))
        {
            return false;
        }
    }
    return true;
}







//    ██████╗ ██╗   ██╗██████╗ ██╗     ██╗ ██████╗    ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗██║     ██║██╔════╝    ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
//    ██████╔╝██║   ██║██████╔╝██║     ██║██║         █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗██║     ██║██║         ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
//    ██║     ╚██████╔╝██████╔╝███████╗██║╚██████╗    ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
//    ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝
//  



/**
 * @brief Parse command-line arguments into a Support::Arguments structure.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param args Argument data structure.
 * @return ARCB_RESULT__OK on success, otherwise a failure code.
 */
Arcb_Result Support::ParseArgs(int argc, char** argv, Support::Arguments &args)
{
    std::stringstream  ss;

    int i = 1;
    while (i < argc)
    {
        std::string_view arg = argv[i];

        if (arg == "-s")
        {
            if (i + 1 < argc)
            {
                // READ ARCFILE PATH.
                args.arcfile = argv[i + 1];
                i += 2;
                continue;
            }
            else
            {
                ERR("Missing value for option -s");
                return Arcb_Result::ARCB_RESULT__NOK;
            }
        }
        else if (arg == "-p")
        {
            if (i + 1 < argc)
            {
                // READ PROFILE NAME.
                args.profile.found = true;
                args.profile.value = argv[i + 1];
                i += 2;
                continue;
            }
            else
            {
                ERR("Missing value for option -p");
                return Arcb_Result::ARCB_RESULT__NOK;
            }
        }
        else if (arg == "-t")
        {
            if (i + 1 < argc)
            {
                // PARSE THREAD COUNT AS POSITIVE INTEGER.
                std::string value = std::string(argv[i + 1]);
                const char* begin = value.data();
                const char* end   = begin + value.size(); 

                auto [ptr, ec] = std::from_chars(begin, end, args.threads.ivalue);

                if (ec != std::errc{} || ptr != end || args.threads.ivalue <= 0)
                {
                    ss << "Invalid value for option -t: " << TOKEN_MAGENTA(value) << ". Expected a positive integer.";
                    ERR(ss.str());
                    return Arcb_Result::ARCB_RESULT__NOK;
                }
                args.threads.svalue = value;
                args.threads.found  = true;
                i += 2;
                continue;
            }
            else
            {
                ERR("Missing value for option -t");
                return Arcb_Result::ARCB_RESULT__NOK;
            }
        }
        else if (arg == "--value")
        {
            if (i + 1 < argc)
            {
                // READ PROFILE NAME.
                args.value.found = true;
                args.value.value = argv[i + 1];

                i += 2;
                continue;
            }
            else
            {
                ERR("Missing value for option --value");
                return Arcb_Result::ARCB_RESULT__NOK;
            }
        }
        if (arg == "--generate")
        {
            // ENABLE TEMPLATE GENERATION MODE (OPTIONAL OUTPUT TARGET).
            args.generator.found = true;

            if (i + 1 < argc)
            {
                args.generator.value = argv[i + 1];
                i += 2;
            }
            else
            {
                ++i;
            }

            continue;
        }
        else if (arg == "--flush-cache")
        {
            // REQUEST CACHE FLUSH MODE.
            args.flush_cache = true;
            ++i;
            continue;
        }
        else if (arg == "--version")
        {
            // REQUEST VERSION OUTPUT.
            args.version = true;
            ++i;
            continue;
        }
        else if (arg == "--help")
        {
            // REQUEST HELP OUTPUT.
            args.help = true;
            ++i;
            continue;
        }
        else if (arg == "--pubs")
        {
            // REQUEST PUB TASK OUTPUT.
            args.pubtasks = true;
            ++i;
            continue;
        }
        else if (arg == "--profiles")
        {
            // REQUEST PUB TASK OUTPUT.
            args.profiles = true;
            ++i;
            continue;
        }
        else if (arg == "--verbose")
        {
            // SUPPRESS RUNTIME LOGS ON STDOUT.
            args.verbose = true;
            ++i;
            continue;
        }
        else if (!args.task.found)
        {
            // CAPTURE FIRST POSITIONAL ARGUMENT AS TASK NAME.
            args.task.found = true;
            args.task.value = argv[i];
        }
        else
        {
            // FAIL ON UNRECOGNIZED PARAMETERS.
            ss << "Unknown parameter " << ANSI_BMAGENTA << argv[i] << ANSI_RESET;
            ERR(ss.str());
            return Arcb_Result::ARCB_RESULT__NOK;
        }

        ++i;
    }

    return Arcb_Result::ARCB_RESULT__OK;
}



/**
 * @brief Handle command-line arguments.
 *
 * @param args Argument data structure.
 * @return ARCB_RESULT__OK on success, otherwise a failure code.
 */
Arcb_Result Support::HandleArgsPreParse(const Arguments &args)
{
    if (args.version)
    {
        return Version();
    }

    if (args.help)
    {
        return Help();
    }

    if (args.flush_cache)
    {
        Cache::Manager::Instance().EraseCache();
        return Arcb_Result::ARCB_RESULT__OK;
    }

    if (args.generator.found)
    {
        std::string output = args.generator.value;
        bool res = Generator::Generate_Template(output);

        if (!res)
        {
            ERR("Cannot generate template!");
            return Arcb_Result::ARCB_RESULT__NOK;
        }

        ARC("Generated template in " << output << "!");
        return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
    }

    if (!Support::file_exists(args.arcfile))
    {
        ERR("Script arcfile not found!");
        return Arcb_Result::ARCB_RESULT__NOK;
    }
    else
    {
        fs::path p(args.arcfile);
        fs::path dir = p.parent_path();

        if (!dir.empty())
        {
            std::error_code ec;
            fs::current_path(dir, ec);
            if (ec)
            {
                ERR("chdir failed for " << dir << ": " << ec.message());
                return Arcb_Result::ARCB_RESULT__NOK;
            }
        }
    }

    return Arcb_Result::ARCB_RESULT__OK;
}




Arcb_Result Support::HandleArgsPostParse(const Arguments &args, Arcb::Semantic::Enviroment& env, Arcb::Jobs::List& list)
{
    if (args.pubtasks)
    {
        auto tasks = Table::GetValues(env.ftable, Semantic::Attr::Type::PUBLIC);

        if (tasks)
        {
            for (auto& task : tasks.value())
            {
                MSG(task.get().task_name);
            }
        }
        
        return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
    }
    else if (args.profiles)
    {
        const auto& profsref = env.GetProfile();
        
        for (uint32_t i = 0; i < profsref.profiles.size(); ++i)
        {
            std::stringstream ss;

            if (i == 0)
            {
                ss << profsref.profiles[i] << " [default";

                if (profsref.profiles[i] == profsref.selected)
                {
                    ss << "|selected";
                }

                ss << "]";
            }
            else
            {
                ss << profsref.profiles[i];

                if (profsref.profiles[i] == profsref.selected)
                {
                    ss << " [selected]";
                }
            }

            MSG(ss.str());
        }
        
        return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
    }
    else if (args.value)
    {
        auto print_attributes = [] (const Semantic::Attr::List & attrs) noexcept -> void
        {
            std::stringstream ss;
            uint32_t          i = 0;

            for (const auto& item : attrs)
            {
                ss << "@" << TOKEN_CYAN(item.name);

                if (item.props.size())
                {
                    ss << " " << join(item.props, " ");
                }
                ss << std::endl;

                ++i;
            }

            MSG(ss.str());
        };
        
        auto print_line = [] (const std::string& value) noexcept -> void
        {
            MSG(value);
        };

        auto print_ctx = [] (const std::string& ctx) noexcept -> void
        {
            std::stringstream ss;
            ss << "[" << TOKEN_YELLOW(ANSI_DIM << ctx) << "]";
            MSG(ss.str());
        };
        
        auto print_kv = [&print_ctx, &print_line] (const std::string& ctx, const std::string& value) noexcept -> void
        {
            if (value.empty()) return;

            print_ctx(ctx);
            print_line(value);
            print_line("");
        };

        bool found = false;
        std::stringstream ss;
        const auto& wanted = args.value.value;

        if (const auto& res = env.vtable.find(wanted); res != env.vtable.end())
        {
            found = true;

            print_kv("TYPE", "Variable");
            print_ctx("VALUE");
            for (const auto& value : res->second.var_value)
            {
                print_line(value);
                print_line("");
            }

            if (res->second.attributes.size())
            {
                print_ctx("ATTRIBUTES");
                print_attributes(res->second.attributes);
            }

            if (res->second.glob_expansion.size())
            {
                print_ctx("GLOB EXPANSION");
                for (const auto& exp : res->second.glob_expansion)
                {
                    print_line(exp);
                }
            }

            return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
        }
        
        if (const auto& res = env.ftable.find(wanted); res != env.ftable.end())
        {
            if (found)
            {
                MSG("-------------------------------------------------------------------------------");
            }

            found = true;
            
            print_kv("TYPE", "Task");
            print_kv("ENGINE", res->second.engine.Get_Repr(true));

            if (res->second.attributes.size())
            {
                print_ctx("ATTRIBUTES");
                print_attributes(res->second.attributes);
            }

            if (list.All().size())
            {
                print_kv("SCHEDULATED", "Yes");
            }
            else
            {
                print_kv("SCHEDULATED", "No");
            }
  
            if (res->second.task_instrs.size())
            {
                for (const auto& job : list.All())
                {
                    if (job.name.compare(res->second.task_name) == 0)
                    {
                        print_ctx("JOB INSTRUCTIONS");
                        for (const auto& instr : job.instructions)
                        {
                            print_line(instr);
                        }

                        print_line("");

                        break;
                    }
                }
                
                print_ctx("TASK INSTRUCTIONS");
                for (const auto& instr : res->second.task_instrs)
                {
                    print_line(instr);
                }
            }

            return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
        }

        if (const auto st = Core::is_symbol(wanted); st != Core::SymbolType::UNDEFINED)
        {
            const auto value = Core::symbol(st);

            if (found)
            {
                MSG("-------------------------------------------------------------------------------");
            }

            found = true;
            
            print_kv("TYPE", "Builtin Symbol");
            print_ctx("VALUE");
            for (const auto& s : value)
            {
                print_line(s);
            }

            return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
        }
        
        if (!found)
        {
            ss << "Arcb does not know any task, variable or symbol called " << TOKEN_MAGENTA(wanted);
            ERR(ss.str());
        }

        return Arcb_Result::ARCB_RESULT__OK_AND_EXIT;
    }

    return Arcb_Result::ARCB_RESULT__OK;
}




/**
 * @brief Check whether a file exists and is a regular file.
 *
 * @param filename File path.
 * @return true if the file exists and is a regular file, false otherwise.
 */
bool Support::file_exists(const std::string& filename)
{
    return fs::exists(filename)
        && fs::is_regular_file(filename);
}



/**
 * @brief Trim whitespace from the left side of the input string.
 *
 * @param s Input string.
 * @return Left-trimmed string.
 */
std::string Support::ltrim(const std::string& s)
{
    const std::string ws = " \t\r\n\f\v";

    size_t pos = s.find_first_not_of(ws);
    if (pos == std::string::npos)
    {
        return "";
    }

    return s.substr(pos);
}



/**
 * @brief Trim whitespace from the right side of the input string.
 *
 * @param s Input string.
 * @return Right-trimmed string.
 */
std::string Support::rtrim(const std::string& s)
{
    const std::string ws = " \t\r\n\f\v";

    size_t pos = s.find_last_not_of(ws);
    if (pos == std::string::npos)
    {
        return "";
    }

    return s.substr(0, pos + 1);
}



/**
 * @brief Convert an ASCII character to lowercase.
 *
 * @param c Input character.
 * @return Lowercased ASCII character.
 */
inline char Support::toLowerAscii(char c) noexcept
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}



/**
 * @brief Split a string by a separator character.
 *
 * Empty tokens are skipped.
 *
 * @param s Input string.
 * @param sep Separator character.
 * @return Vector of tokens.
 */
std::vector<std::string> Support::split(const std::string& s, char sep) noexcept
{
    std::vector<std::string> result;
    std::size_t i = 0;
    const std::size_t n = s.size();

    while (i < n)
    {
        // salta spazi
        while (i < n && s[i] == sep)
            ++i;

        if (i >= n)
            break;

        // inizio token
        std::size_t start = i;

        // avanza fino allo spazio
        while (i < n && s[i] != sep)
            ++i;

        result.emplace_back(s.substr(start, i - start));
    }

    return result;
}


std::string Support::join(const std::vector<std::string>& vec, std::string sep) noexcept
{
    uint32_t i = 0;
    std::stringstream ss;

    for (const auto& item : vec)
    {
        if (i) ss << sep;
        ss << item;
        i++;
    }

    return ss.str();
};


void Support::sanificate(std::string& source, char target) noexcept
{
    std::size_t count = 0U;
    for (char c : source)
    {
        if (c == target)
        {
            ++count;
        }
    }

    if (count == 0U)
    {
        return; // niente da fare
    }

    const std::size_t oldSize = source.size();
    const std::size_t newSize = oldSize + count;

    source.resize(newSize);

    // PASSO 2: scorri da destra verso sinistra e sposta/espandi
    std::size_t i = oldSize;
    std::size_t j = newSize;

    while (i > 0U)
    {
        --i;
        char c = source[i];

        if (c == target)
        {
            source[--j] = c;
            source[--j] = '\\';
        }
        else
        {
            source[--j] = c;
        }
    }
}


/**
 * @brief Split a string by a separator, treating single-quoted substrings as atomic tokens.
 *
 * The quoting character is '\'' and the separator is typically space.
 *
 * @param s Input string.
 * @param sep Separator character.
 * @return A Support::SplitResult containing tokens or an error description.
 */
Support::SplitResult Support::split_quoted(const std::string& s, char sep) noexcept
{
    Support::SplitResult res{true, {}, {}};

    std::string current;
    bool in_quote           = false;
    bool just_closed_quote  = false;

    const std::size_t n = s.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        char c = s[i];

        // controllo subito dopo una chiusura di quote
        if (just_closed_quote)
        {
            if (c == sep)
            {
                just_closed_quote = false;
                continue; // consumiamo il separatore
            }
            else
            {
                res.ok    = false;
                res.tokens.clear();
                res.error = "missing space after closing quote";
                return res;
            }
        }

        if (!in_quote)
        {
            if (c == sep)
            {
                if (!current.empty())
                {
                    res.tokens.emplace_back(std::move(current));
                    current.clear();
                }
                continue;
            }

            if (c == '\'')
            {
                // quote deve iniziare un token nuovo
                if (!current.empty())
                {
                    res.ok    = false;
                    res.tokens.clear();
                    res.error = "quote in the middle of a token";
                    return res;
                }

                in_quote = true;
                continue;
            }

            current.push_back(c);
        }
        else // in_quote == true
        {
            if (c == '\'')
            {
                // chiusura della quote: token completo
                res.tokens.emplace_back(std::move(current));
                current.clear();
                in_quote          = false;
                just_closed_quote = true;
            }
            else
            {
                current.push_back(c);
            }
        }
    }

    if (in_quote)
    {
        res.ok    = false;
        res.tokens.clear();
        res.error = "unmatched quote";
        return res;
    }

    if (!current.empty())
        res.tokens.emplace_back(std::move(current));

    return res;
}



/**
 * @brief Convert a decimal numeric string into a long long.
 *
 * @param s Input string.
 * @return Parsed value on success, std::nullopt on failure.
 */
std::optional<long long> Support::to_number(const std::string& s)
{
    if (s.empty()) 
        return std::nullopt;

    long long value = 0;

    for (char c : s)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return std::nullopt;

        int digit = c - '0';

        if (value > (LLONG_MAX - digit) / 10)
            return std::nullopt;

        value = value * 10 + digit;
    }

    return value;
}



/**
 * @brief Generate a mangled symbol name using the provided target and mangling suffix.
 *
 * @param target Base name.
 * @param mangling Mangling suffix.
 * @return Mangled string in the form "target@@mangling".
 */
std::string Support::generate_mangling(const std::string& target, const std::string& mangling)
{
    std::stringstream ss;
    ss << target << "@@" << mangling;
    return ss.str();
}



/**
 * @brief Convert a token type to a human-readable string.
 *
 * @param type Token type.
 * @return String representation of the token type.
 */
std::string Support::TokenTypeRepr(const Scan::TokenType type)
{
    switch (type)
    {
        case Scan::TokenType::IDENTIFIER:  return "identifier";
        case Scan::TokenType::TASK:        return "task";    
        case Scan::TokenType::IMPORT:      return "import";      
        case Scan::TokenType::USING:       return "using";      
        case Scan::TokenType::NUMBER:      return "number";  
        case Scan::TokenType::DQUOTE:      return "Double Quote";  
        case Scan::TokenType::MAPPING:     return "map";      
        case Scan::TokenType::ASSERT:      return "assert";
        case Scan::TokenType::ASSIGN:      return "assignment";  
        case Scan::TokenType::PLUS:        return "plus";
        case Scan::TokenType::MINUS:       return "minus"; 
        case Scan::TokenType::STAR:        return "star";
        case Scan::TokenType::SLASH:       return "slash"; 
        case Scan::TokenType::ROUNDLP:     return "left parenthesis";   
        case Scan::TokenType::ROUNDRP:     return "right parenthesis";   
        case Scan::TokenType::SQUARELP:    return "left bracket";    
        case Scan::TokenType::SQUARERP:    return "right bracket";    
        case Scan::TokenType::CURLYLP:     return "left brace";   
        case Scan::TokenType::CURLYRP:     return "right brace";   
        case Scan::TokenType::ANGULARLP:   return "left angular parenthesis";   
        case Scan::TokenType::ANGULARRP:   return "right angular parenthesis"; 
        case Scan::TokenType::AT:          return "at sign";   
        case Scan::TokenType::EQ:          return "eq";   
        case Scan::TokenType::NE:          return "ne";   
        case Scan::TokenType::IN:          return "in";   
        case Scan::TokenType::SEMICOLON:   return "semicolon";   
        case Scan::TokenType::NEWLINE:     return "<new line>";   
        case Scan::TokenType::ENDOFFILE:   return "EOF";     
        case Scan::TokenType::UNKNOWN:     return "UNKNOWN";   
        case Scan::TokenType::ANY:         return "any";
        case Scan::TokenType::OPT_NEWLINE: return "<new line>";
        default:                           return "<INVALID>";
    }
}



/**
 * @brief Convert a grammar terminal set to a formatted string.
 *
 * @param type Terminal set.
 * @return String representation of the terminal set.
 */
std::string Support::TerminalRepr(const Grammar::Terminal& type)
{   
    std::stringstream ss;

    for (uint32_t i = 0; i < type.size(); ++i)
    {
        if (i > 0) ss << " or ";

        ss << ANSI_GREEN << TokenTypeRepr(type[i]) << ANSI_RESET;
    }

    return ss.str();
}



/**
 * @brief Convert a grammar non-terminal set to a formatted string.
 *
 * @param type Non-terminal set.
 * @return String representation of the non-terminal set.
 */
std::string Support::NonTerminalRepr(const Grammar::NonTerminal& type)
{   
    std::stringstream ss;

    for (uint32_t i = 0; i < type.size(); ++i)
    {
        if (i > 0) ss << " or ";

        ss << TerminalRepr(type[i]);
    }

    return ss.str();
}



/**
 * @brief Convert a unique non-terminal set to a formatted string.
 *
 * @param type Unique non-terminal set.
 * @return String representation of the unique non-terminal set.
 */
std::string Support::UniqueNonTerminalRepr(const Grammar::UniqueNonTerminal& type)
{
    std::stringstream ss;
    uint32_t          i = 0;

    for (const auto& ref : type)
    {
        if (i > 0) ss << " or ";

        ss << TerminalRepr(ref);

        ++i;
    }

    return ss.str();
}



/**
 * @brief Convert a grammar rule to a human-readable string.
 *
 * @param type Grammar rule.
 * @return String representation of the rule.
 */
std::string Support::RuleRepr(const Grammar::Rule type)
{
    switch (type)
    {
        case Grammar::Rule::UNDEFINED:         return "UNDEFINED";
        case Grammar::Rule::VARIABLE_ASSIGN:   return "Assignment";
        case Grammar::Rule::VARIABLE_JOIN:     return "Join";
        case Grammar::Rule::EMPTY_LINE:        return "Empty Line";
        case Grammar::Rule::ATTRIBUTE:         return "Attribute";
        case Grammar::Rule::TASK_DECL:         return "Task Declaration";
        case Grammar::Rule::IMPORT:            return "Import";
        case Grammar::Rule::USING:             return "Using";
        case Grammar::Rule::MAPPING:           return "Mapping";
        case Grammar::Rule::ASSERT_MSG:        return "Assert Message";
        case Grammar::Rule::ASSERT_ACT:        return "Assert Action";
        default:                               return "<INVALID>";
    }
}



std::vector<std::string> Support::GetPathEntries() noexcept
{
    static std::vector<std::string> result;

    if (!result.empty()) return result;

    const char* raw = std::getenv("PATH");
    if (raw == nullptr)
    {
        return result;
    }

    const std::string path_env{raw};

#ifdef _WIN32
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif

    std::size_t start = 0;

    while (start <= path_env.size())
    {
        const std::size_t end = path_env.find(separator, start);

        const std::string token =
            (end == std::string::npos)
            ? path_env.substr(start)
            : path_env.substr(start, end - start);

        if (!token.empty())
        {
            result.emplace_back(token);
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return result;
}


bool Support::Is_In_Path(const std::string& what) noexcept
{
    const auto& paths = Core::symbol(Core::SymbolType::PATH);

    for (const auto& path : paths)
    {
        const fs::path fullpath = path;

        if (file_exists(fullpath / what)) return true;
    }

    return false;
}

