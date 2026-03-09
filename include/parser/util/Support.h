#ifndef __ARCANA_UTIL_SUPPORT__H__
#define __ARCANA_UTIL_SUPPORT__H__


#include <map>
#include <set>
#include <cctype>
#include <string>
#include <vector>
#include <limits>
#include <climits> 
#include <optional>
#include <string_view>
#include <unordered_map>
#include <filesystem>


#include "Defines.h"



//    ███████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ██████╗ ███████╗
//    ██╔════╝██╔═══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔══██╗██╔════╝
//    █████╗  ██║   ██║██████╔╝██║ █╗ ██║███████║██████╔╝██║  ██║███████╗
//    ██╔══╝  ██║   ██║██╔══██╗██║███╗██║██╔══██║██╔══██╗██║  ██║╚════██║
//    ██║     ╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║██████╔╝███████║
//    ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝
//                                                                                                                                                                            

/**
 * @file Support.h
 * @brief Utility helpers used across Arcana.
 *
 * This header provides common utilities used across the scanner, grammar,
 * semantic and runtime layers, including:
 * - command line argument parsing
 * - filesystem helpers
 * - string helpers (trim/split/quoted split)
 * - numeric conversion helpers
 * - mangling helpers for profile/OS specialized keys
 * - string representations for grammar/scanner entities
 * - fuzzy matching helpers (closest string)
 * - transparent hashing/equality for std::string_view keyed maps
 */

/**
 * @defgroup Support Support Utilities
 * @brief Shared helper utilities for Arcana subsystems.
 */

/**
 * @addtogroup Support
 * @{
 */

BEGIN_MODULE(Scan)

class Lexer;

enum class TokenType : uint32_t;

END_MODULE(Scan)


BEGIN_MODULE(Grammar)

class Index;

struct Match;

enum class Rule : uint32_t;

END_MODULE(Grammar)


BEGIN_MODULE(Semantic)

struct Enviroment;

END_MODULE(Semantic)



BEGIN_MODULE(Jobs)

class List;

END_MODULE(Jobs)



BEGIN_MODULE(Support)

struct SemanticOutput;

END_MODULE(Support)





//    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗
//    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝
//    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗
//    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║
//    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║
//    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
//           


BEGIN_MODULE(Support)

/// @brief structure used to hold command line arguments
struct Arguments
{
    std::string arcfile;

    struct
    {
        std::string value;
        bool        found;

        operator bool () const noexcept { return found; } 
    } 
    task,
    value,
    profile,
    generator;
    
    struct
    {
        std::string svalue;
        uint32_t    ivalue;
        bool        found;

        operator bool () const noexcept { return found; } 
    } 
    threads;

    bool debug;
    bool flush_cache;
    bool version;
    bool help;
    bool verbose;
    bool pubtasks;
    bool profiles;

    Arguments() 
        : 
        arcfile("arcfile"),
        task{"", false},
        value{"", false},
        profile{"", false},
        generator{"", false},
        threads{"", 0, false},
        debug(false),
        flush_cache(false),
        version(false),
        help(false),
        verbose(false),
        pubtasks(false),
        profiles(false)
    {}
};


END_MODULE(Support)





//    ██╗   ██╗███████╗██╗███╗   ██╗ ██████╗ ███████╗
//    ██║   ██║██╔════╝██║████╗  ██║██╔════╝ ██╔════╝
//    ██║   ██║███████╗██║██╔██╗ ██║██║  ███╗███████╗
//    ██║   ██║╚════██║██║██║╚██╗██║██║   ██║╚════██║
//    ╚██████╔╝███████║██║██║ ╚████║╚██████╔╝███████║
//     ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝
//                                                   

BEGIN_MODULE(Grammar)

using Terminal          = std::vector<Scan::TokenType>;
using NonTerminal       = std::vector<Terminal>;
using Production        = std::map<Grammar::Rule, NonTerminal>;

using UniqueRule        = std::set<Grammar::Rule>;
using UniqueNonTerminal = std::set<Terminal>;


END_MODULE(Grammar)

using Point             = const Arcana::Grammar::Index*;
using Input             = std::string&;
using Lexeme            = std::string;
using Statement         = std::vector<std::string>;











//    ███╗   ███╗ ██████╗ ██████╗ ██╗   ██╗██╗     ███████╗    ███████╗██╗   ██╗██████╗ ██████╗  ██████╗ ██████╗ ████████╗
//    ████╗ ████║██╔═══██╗██╔══██╗██║   ██║██║     ██╔════╝    ██╔════╝██║   ██║██╔══██╗██╔══██╗██╔═══██╗██╔══██╗╚══██╔══╝
//    ██╔████╔██║██║   ██║██║  ██║██║   ██║██║     █████╗      ███████╗██║   ██║██████╔╝██████╔╝██║   ██║██████╔╝   ██║   
//    ██║╚██╔╝██║██║   ██║██║  ██║██║   ██║██║     ██╔══╝      ╚════██║██║   ██║██╔═══╝ ██╔═══╝ ██║   ██║██╔══██╗   ██║   
//    ██║ ╚═╝ ██║╚██████╔╝██████╔╝╚██████╔╝███████╗███████╗    ███████║╚██████╔╝██║     ██║     ╚██████╔╝██║  ██║   ██║   
//    ╚═╝     ╚═╝ ╚═════╝ ╚═════╝  ╚═════╝ ╚══════╝╚══════╝    ╚══════╝ ╚═════╝ ╚═╝     ╚═╝      ╚═════╝ ╚═╝  ╚═╝   ╚═╝   
//                                                                                                                        





BEGIN_MODULE(Support)



namespace fs = std::filesystem;



//    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗
//    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝
//    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗
//    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║
//    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║
//    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
//                                                                                                               

/**
 * @brief Transparent hash for std::string_view.
 *
 * Enables heterogeneous lookup in unordered containers keyed by std::string_view
 * (e.g. find with std::string, const char*, std::string_view).
 */
struct StringViewHash
{
    using is_transparent = void;

    /**
     * @brief Hashes a string_view.
     */
    std::size_t operator()(std::string_view s) const noexcept;
};


/**
 * @brief Transparent equality for std::string_view.
 *
 * Enables heterogeneous comparison in unordered containers keyed by std::string_view.
 */
struct StringViewEq
{
    using is_transparent = void;

    /**
     * @brief Compares two string_view values for equality.
     */
    bool operator() (std::string_view a, std::string_view b) const noexcept;
};





/**
 * @brief Tokenization result for split_quoted().
 */
struct SplitResult
{
    bool                     ok;     ///< True on success.
    std::vector<std::string> tokens; ///< Token list.
    std::string              error;  ///< Error message on failure.
};







//    ██████╗ ██╗   ██╗██████╗ ██╗     ██╗ ██████╗    ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗██║     ██║██╔════╝    ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
//    ██████╔╝██║   ██║██████╔╝██║     ██║██║         █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗██║     ██║██║         ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
//    ██║     ╚██████╔╝██████╔╝███████╗██║╚██████╗    ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
//    ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝
//                                                                                                                                                                                                        

#define NO_CTX                                      std::string{}
#define Raise_Error(ctx, msg)                       ([&] () -> Arcana_Result { if (!ctx.empty()) ERR(ctx); MSG(msg);                                                                              return Arcana_Result::ARCANA_RESULT__NOK; }())
#define Raise_Error_With_Hint(ctx, msg, hint)       ([&] () -> Arcana_Result { if (!ctx.empty()) ERR(ctx); MSG(msg); if (auto h = hint; !h.empty()) MSG("Did you mean " << TOKEN_CYAN(h) << "?"); return Arcana_Result::ARCANA_RESULT__NOK; }())
#define Raise_Expansion_Error(msg)                  ([&] () -> Arcana_Result { error.clear(); error << msg;                                                                                       return Arcana_Result::ARCANA_RESULT__NOK; }())
#define Raise_Expansion_Error_With_Hint(msg, hint)  ([&] () -> Arcana_Result { error.clear(); error << msg << std::endl << "Did you mean " << TOKEN_CYAN(hint) << "?";                            return Arcana_Result::ARCANA_RESULT__NOK; }())
 


Arcana_Result Parser_Error(const std::string& ctx, const Grammar::Match& match, Scan::Lexer& lexer);



/**
 * @brief Parse command-line arguments into a Support::Arguments structure.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param args Argument data structure.
 * @return ARCANA_RESULT__OK on success, otherwise a failure code.
 */
Arcana_Result ParseArgs(int argc, char** argv, Support::Arguments &args);




/**
 * @brief Handle command-line arguments pre parsing.
 *
 * @param args Argument data structure.
 * @return ARCANA_RESULT__OK on success, otherwise a failure code.
 */
Arcana_Result HandleArgsPreParse(const Arguments &args);



/**
 * @brief Handle command-line arguments post parsing.
 *
 * @param args Argument data structure.
 * @return ARCANA_RESULT__OK on success, otherwise a failure code.
 */
Arcana_Result HandleArgsPostParse(const Arguments &args, Semantic::Enviroment& env, Jobs::List& list);



/**
 * @brief Checks whether a file exists.
 *
 * @param filename Path to file.
 * @return true if the file exists and is accessible.
 */
bool file_exists(const std::string& filename);



/// @brief remove whitespace character for the left of the string
/// @param s the input string
/// @return a copy left trimmed of @ref s
std::string ltrim(const std::string& s);



/// @brief remove whitespace character for the left of the string
/// @param s the input string
/// @return a copy left trimmed of @ref s
std::string rtrim(const std::string& s);



/**
 * @brief Converts an ASCII character to lower-case.
 *
 * Only affects the range 'A'..'Z'.
 */
inline char toLowerAscii(char c) noexcept;



/**
 * @brief Splits a string on a separator character.
 *
 * @param s Input string.
 * @param sep Separator character.
 * @return Token list.
 */
std::vector<std::string> split(const std::string& s, char sep = ' ') noexcept;


std::string join(const std::vector<std::string>& vec, std::string sep = ", ") noexcept;

void sanificate(std::string& source, char target) noexcept;


/**
 * @brief Splits a string on a separator while honoring quoted segments.
 *
 * @param s Input string.
 * @param sep Separator character.
 * @return SplitResult containing tokens or an error.
 */
SplitResult split_quoted(const std::string& s, char sep = ' ') noexcept;



/**
 * @brief Converts a string to a number (base auto-detection is implementation-defined).
 *
 * @param s Input string.
 * @return Parsed number if conversion succeeds.
 */
std::optional<long long> to_number(const std::string& s);



/**
 * @brief Generates a mangled key string.
 *
 * Used to represent specialized entries (e.g. profile/OS bound) in tables.
 *
 * @param target Base identifier.
 * @param mangling Mangling suffix (e.g. profile name).
 * @return Mangled key.
 */
std::string generate_mangling(const std::string& target, const std::string& mangling);



/// @brief Function used to represent a TokenType
/// @param[in] type 
/// @return string representation of TokenType
std::string TokenTypeRepr(const Scan::TokenType type);



/// @brief Function used to represent a vector of TokenType 
/// @param[in] type 
/// @return string representation of TokenType separeted by 'or'
std::string TerminalRepr(const Grammar::Terminal& type);



/// @brief Function used to represent a vector of vector of TokenType
/// @param[in] type 
/// @return string representation of TokenType separeted by 'or'
std::string NonTerminalRepr(const Grammar::NonTerminal& type);



/// @brief Function used to represent a set of vector of TokenType
/// @param[in] type 
/// @return string representation of TokenType separeted by 'or'
std::string UniqueNonTerminalRepr(const Grammar::UniqueNonTerminal& type);



/// @brief Function used to represent a Rule
/// @param[in] type 
/// @return string representation of Rule
std::string RuleRepr(const Grammar::Rule type);




std::size_t levenshtein_distance(const std::string_view& a,
                                 const std::string_view& b) noexcept;

/**
 * @brief Finds the closest string to a target within a maximum edit distance.
 *
 * Intended for diagnostics ("did you mean ...?") style suggestions.
 *
 * @param list Candidate strings.
 * @param target Target string.
 * @param max_distance Maximum allowed distance.
 * @return Closest match if found within threshold.
 */
template<typename Range>
std::string FindClosest(const Range& list,
                        const std::string& target,
                        std::size_t max_distance = std::numeric_limits<std::size_t>::max()) noexcept
{
    std::string_view best{};
    std::size_t best_dist = max_distance;

    for (std::string_view s : list)
    {
        if (auto pos = s.find("@@"); pos != std::string_view::npos)
        {
            s = s.substr(0, pos);
        }

        if (s == target)
        {
            continue;
        }

        const std::size_t d = Support::levenshtein_distance(s, target);

        if (d < best_dist)
        {
            best_dist = d;
            best = s;
        }
    }

    return std::string(best);
}



std::vector<std::string> GetPathEntries() noexcept;


bool Is_In_Path(const std::string& what) noexcept;


/**
 * @brief Unordered map keyed by std::string_view with transparent hashing/equality.
 *
 * Allows heterogeneous lookup (std::string, std::string_view, const char*).
 */
template < typename T >
using AbstractKeywordMap = std::unordered_map<
    std::string_view,
    T,
    StringViewHash,
    StringViewEq
>;





END_MODULE(Support)

/** @} */

#endif /* __ARCANA_UTIL_SUPPORT__H__ */
