#ifndef __ARCB_GLOB_H__
#define __ARCB_GLOB_H__

/**
 * @defgroup Glob Glob Pattern Engine
 * @brief Glob parsing, expansion and mapping utilities.
 *
 * This module implements a custom glob engine used by Arcb.
 *
 * Features:
 * - glob pattern parsing
 * - filesystem expansion
 * - glob-to-glob mapping using captured segments
 *
 * The implementation is designed to be deterministic, exception-free,
 * and suitable for build-system use.
 */


/**
 * @addtogroup Cache
 * @{
 */


#include "Defines.h"

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <string_view>



BEGIN_MODULE(Glob)


/** @brief Filesystem namespace alias used by the glob module. */
namespace fs = std::filesystem;




//    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗
//    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝
//    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗
//    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║
//    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║
//    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
//                                                               


/**
 * @brief Error information produced during glob parsing.
 */
struct ParseError
{
    /**
     * @brief Parsing error codes.
     */
    enum class Code : std::uint8_t
    {
        NONE,                 ///< No error.
        EMPTY_PATTERN,        ///< Empty input pattern.
        INVALID_ESCAPE,       ///< Invalid escape sequence.
        UNCLOSED_CHARCLASS,   ///< Unterminated character class.
        EMPTY_CHARCLASS,      ///< Empty character class ([] or [^]).
        INVALID_RANGE,        ///< Invalid range inside a character class.
        INVALID_DOUBLESTAR    ///< Invalid usage of '**'.
    };

    Code        code   = Code::NONE; ///< Error code.
    std::size_t offset = 0;          ///< Offset in the input pattern where the error occurred.
};



/**
 * @brief Inclusive character range (e.g. a-z).
 */
struct CharRange
{
    char first = '\0'; ///< First character in the range.
    char last  = '\0'; ///< Last character in the range.
};



/**
 * @brief Character class representation ([...]).
 */
struct CharClass
{
    bool                   negated = false;   ///< True for negated classes ([^...]).
    std::vector<char>      singles;           ///< Explicit characters.
    std::vector<CharRange> ranges;            ///< Character ranges.
};



/**
 * @brief Atomic glob element.
 *
 * Atoms are the smallest matching units inside a glob segment.
 */
struct Atom
{
    /**
     * @brief Atom kinds.
     */
    enum class Kind : std::uint8_t
    {
        LITERAL,                        ///< Literal string.
        STAR,                           ///< '*' wildcard.
        QMARK,                          ///< '?' wildcard.
        CHARCLASS,                      ///< Character class ([...]).
        DOUBLESTAR                      ///< '**' wildcard (segment-only).
    };

    Kind kind = Kind::LITERAL;          ///< Atom type.

    std::string literal;                ///< Literal payload.
    CharClass   cls;                    ///< Character class payload.

    /** @brief Creates a literal atom. */
    static Atom MakeLiteral(std::string s)
    {
        Atom a{};
        a.kind    = Kind::LITERAL;
        a.literal = std::move(s);
        return a;
    }

    /** @brief Creates a '*' atom. */
    static Atom MakeStar()
    {
        Atom a{};
        a.kind = Kind::STAR;
        return a;
    }

    /** @brief Creates a '?' atom. */
    static Atom MakeQMark()
    {
        Atom a{};
        a.kind = Kind::QMARK;
        return a;
    }

    /** @brief Creates a '**' atom. */
    static Atom MakeDoubleStar()
    {
        Atom a{};
        a.kind = Kind::DOUBLESTAR;
        return a;
    }

    /** @brief Creates a character class atom. */
    static Atom MakeCharClass(CharClass c)
    {
        Atom a{};
        a.kind = Kind::CHARCLASS;
        a.cls  = std::move(c);
        return a;
    }
};



/**
 * @brief A path segment composed of glob atoms.
 */
struct Segment
{
    std::vector<Atom> atoms;  ///< Atoms composing the segment.

    /**
     * @brief Checks whether this segment consists only of '**'.
     *
     * @return true if the segment contains a single DOUBLESTAR atom.
     */
    bool IsDoubleStarOnly() const
    {
        if (atoms.size() != 1)
        {
            return false;
        }
        return atoms[0].kind == Atom::Kind::DOUBLESTAR;
    }
};



/**
 * @brief Parsed glob pattern.
 */
struct Pattern
{
    bool                 absolute = false;  ///< True if the pattern is absolute (starts with '/').
    std::vector<Segment> segments;          ///< Parsed path segments.
    std::string          normalized;        ///< Normalized pattern (debug/trace).
};



/**
 * @brief Glob parsing options.
 */
struct Options
{
    char separator               = '/';     ///< Path separator.
    bool backslash_escape        = true;    ///< Enable backslash escaping.
    bool doublestar_segment_only = true;    ///< Restrict '**' to whole segments.
};



/**
 * @brief Filesystem expansion options.
 */
struct ExpandOptions
{
    bool follow_symlinks  = false;          ///< Follow symbolic links.
    bool include_dotfiles = false;          ///< Include dotfiles.
};




/**
 * @brief Captured value during glob matching.
 */
struct Capture
{
    /**
     * @brief Capture kind.
     */
    enum class Kind : std::uint8_t
    {
        PATH,                           ///< Capture from '**'.
        SEGMENT,                        ///< Capture from '*'.
        CHAR                            ///< Capture from '?' or character class.
    };

    Kind        kind  = Kind::SEGMENT;  ///< Capture type.
    std::string value;                  ///< Captured value.
};



/**
 * @brief Error information produced during glob-to-glob mapping.
 */
struct MapError
{
    /**
     * @brief Mapping error codes.
     */
    enum class Code : std::uint8_t
    {
        NONE,                           ///< No error.
        CAPTURE,                        ///< Capture mismatch.
        INSTANTIATE                     ///< Failed instantiation of target glob.
    };

    Code code = Code::NONE;             ///< Error code.
};




//    ██████╗ ██╗   ██╗██████╗     ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗    ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
//    ██████╔╝██║   ██║██████╔╝    █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗    ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
//    ██║     ╚██████╔╝██████╔╝    ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
//    ╚═╝      ╚═════╝ ╚═════╝     ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝
//                                                                                                                                                                                     


/**
 * @brief Parses a glob pattern.
 *
 * @param[in]  input Input glob pattern.
 * @param[out] out Output parsed pattern.
 * @param[out] err Parsing error information.
 * @param[in]  opt Parsing options.
 *
 * @return true on success, false on parse error.
 */
bool Parse(std::string_view input, Pattern& out, ParseError& err, const Options& opt = Options{}) noexcept;



/**
 * @brief Expands a parsed glob pattern against the filesystem.
 *
 * @param[in]  pattern Parsed glob pattern.
 * @param[in]  base_dir Base directory for expansion.
 * @param[out] out Output list of matched paths.
 * @param[in]  opt Expansion options.
 *
 * @return true on success, false on failure.
 */
bool Expand(const Pattern& pattern, const fs::path& base_dir, std::vector<std::string>& out,
            const ExpandOptions& opt = ExpandOptions{}) noexcept;



/**
 * @brief Maps one glob expansion to another glob pattern.
 *
 * Uses captured segments from the source glob expansion to
 * instantiate the target glob.
 *
 * @param[in]  from_glob Source glob pattern.
 * @param[in]  to_glob Target glob pattern.
 * @param[in]  src_list Source glob expansion.
 * @param[out] out_list Output mapped expansion.
 * @param[out] err_from Parse error for source glob.
 * @param[out] err_to Parse error for target glob.
 * @param[out] err_map Mapping error information.
 *
 * @return true on success, false on error.
 */
bool MapGlobToGlob(std::vector<std::string>    from_glob,
                    std::string_view                to_glob,
                    std::vector<std::string> src_list,
                    std::vector<std::string>&       out_list,
                    ParseError&                     err_from,
                    ParseError&                     err_to,
                    MapError&                       err_map) noexcept;



/**
 * @brief Returns a human-readable description of a ParseError.
 */                
inline std::string ParseErrorRepr(const ParseError e)
{
    switch(e.code)
    {
        case ParseError::Code::EMPTY_PATTERN:         return "Empty Pattern";
        case ParseError::Code::INVALID_ESCAPE:        return "Invalid Escape";
        case ParseError::Code::UNCLOSED_CHARCLASS:    return "Unclosed Char Class";
        case ParseError::Code::EMPTY_CHARCLASS:       return "Empty Char Class";
        case ParseError::Code::INVALID_RANGE:         return "Invalid Range";
        case ParseError::Code::INVALID_DOUBLESTAR:    return "Invalid DoubleStar";
        default:                                      return "Not An Error";
    }
}


/**
 * @brief Returns a human-readable description of a MapError.
 */
inline std::string MapErrorRepr(const MapError e)
{
    switch(e.code)
    {
        case MapError::Code::CAPTURE:         return "Capture Error";
        case MapError::Code::INSTANTIATE:     return "Instantiation Error";
        default:                              return "Not An Error";
    }
}


END_MODULE(Glob)


/** @} */


#endif /* __ARCB_GLOB_H__ */