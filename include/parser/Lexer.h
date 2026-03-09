#ifndef __ARCB_LEXER__H__
#define __ARCB_LEXER__H__


#include <set>
#include <vector>
#include <sstream>
#include <istream>
#include <fstream>

#include "Defines.h"



/**
 * @file Lexer.h
 * @brief Scanner (lexer) for Arcb scripts.
 *
 * This header defines the tokenization layer. The lexer consumes an Arcb script
 * source and produces a stream of Token objects. Each Token records:
 * - token type (TokenType)
 * - matched lexeme (raw text)
 * - source location metadata (line and span indices)
 *
 * The lexer also retains the original source lines to support diagnostics and
 * error reporting with line excerpts.
 */

/**
 * @defgroup Scan Scanner / Lexer
 * @brief Tokenization utilities for Arcb scripts.
 *
 * The Scan module provides the lexical analysis stage:
 * - TokenType: token classification
 * - Token: token payload + source location
 * - Lexer: stateful token generator
 */

/**
 * @addtogroup Scan
 * @{
 */

BEGIN_MODULE(Scan)



//    ██████╗ ██╗   ██╗██████╗ ██╗     ██╗ ██████╗    ███████╗███╗   ██╗██╗   ██╗███╗   ███╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗██║     ██║██╔════╝    ██╔════╝████╗  ██║██║   ██║████╗ ████║██╔════╝
//    ██████╔╝██║   ██║██████╔╝██║     ██║██║         █████╗  ██╔██╗ ██║██║   ██║██╔████╔██║███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗██║     ██║██║         ██╔══╝  ██║╚██╗██║██║   ██║██║╚██╔╝██║╚════██║
//    ██║     ╚██████╔╝██████╔╝███████╗██║╚██████╗    ███████╗██║ ╚████║╚██████╔╝██║ ╚═╝ ██║███████║
//    ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝    ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝
//                                                                                                                                                                                            

/// @brief Enum used to define lexer tokens
///
/// TokenType values represent the lexical categories recognized by the scanner.
/// Some tokens correspond to reserved keywords (TASK, IMPORT, ...), while others
/// represent operators, delimiters, literals, and control tokens (NEWLINE, ENDOFFILE).
enum class TokenType : uint32_t
{
    /// Identifier (user-defined symbol: variable name, task name, attribute name, ...)
    IDENTIFIER           =  0,

    /// Reserved keyword: "task"
    TASK                     ,

    /// Reserved keyword: "import"
    IMPORT                   ,

    /// Reserved keyword: "using"
    USING                    ,

    /// Reserved keyword: "map" / mapping construct
    MAPPING                  ,

    /// Reserved keyword: "assert"
    ASSERT                   ,

    /// Numeric literal (integer form as recognized by the lexer)
    NUMBER                   ,

    /// Assignment operator (typically '=')
    ASSIGN                   ,

    /// Double quote token (")
    DQUOTE                   ,

    /// Plus operator (+)
    PLUS                     ,

    /// Minus operator (-)
    MINUS                    ,

    /// Star operator (*)
    STAR                     ,

    /// Slash operator (/)
    SLASH                    ,

    /// '(' delimiter
    ROUNDLP                  ,

    /// ')' delimiter
    ROUNDRP                  ,

    /// '[' delimiter
    SQUARELP                 ,

    /// ']' delimiter
    SQUARERP                 ,

    /// '{' delimiter
    CURLYLP                  ,

    /// '}' delimiter
    CURLYRP                  ,

    /// '<' delimiter (if used by your grammar)
    ANGULARLP                ,

    /// '>' delimiter (if used by your grammar)
    ANGULARRP                ,

    /// '@' attribute introducer
    AT                       ,

    /// Not-equal operator (!=)
    NE                       ,

    /// Equal operator (==)
    EQ                       ,

    /// Membership / containment operator ("in")
    IN                       ,

    /// ';' delimiter
    SEMICOLON                ,

    /// Newline token (line break in source)
    NEWLINE                  ,

    /// End-of-file token
    ENDOFFILE                ,

    // SPECIAL TOKENS

    /// Unknown/unrecognized character or sequence
    UNKNOWN                  ,

    /// Wildcard token used internally by grammar/matcher (not produced by normal lexing in many designs)
    ANY                      ,

    /// Optional newline token used internally by grammar/matcher (treat newline as present/absent)
    OPT_NEWLINE              ,        
};




//    ██████╗ ██╗   ██╗██████╗ ██╗     ██╗ ██████╗    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗██║     ██║██╔════╝    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝
//    ██████╔╝██║   ██║██████╔╝██║     ██║██║         ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗██║     ██║██║         ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║
//    ██║     ╚██████╔╝██████╔╝███████╗██║╚██████╗    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║
//    ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
//                                                                                                                                                                                                         


/// @brief Struct used to hold the lexer output
///
/// Token represents a single lexical unit produced by Lexer::next().
///
/// Location fields:
/// - line: 1-based line number in the source file
/// - start/end: indices of the lexeme span (implementation-defined; commonly column offsets)
///              start is inclusive, end is exclusive.
struct Token 
{
    TokenType    type;         //<! Type of token
    std::string  lexeme;       //<! lexeme matched   
    std::size_t  line;         //<! Line of match
    std::size_t  start;        //<! Lexeme start index match  
    std::size_t  end;          //<! Lexeme end index match
};




//    ██████╗ ██╗   ██╗██████╗ ██╗     ██╗ ██████╗     ██████╗██╗      █████╗ ███████╗███████╗███████╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗██║     ██║██╔════╝    ██╔════╝██║     ██╔══██╗██╔════╝██╔════╝██╔════╝██╔════╝
//    ██████╔╝██║   ██║██████╔╝██║     ██║██║         ██║     ██║     ███████║███████╗███████╗█████╗  ███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗██║     ██║██║         ██║     ██║     ██╔══██║╚════██║╚════██║██╔══╝  ╚════██║
//    ██║     ╚██████╔╝██████╔╝███████╗██║╚██████╗    ╚██████╗███████╗██║  ██║███████║███████║███████╗███████║
//    ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝     ╚═════╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚══════╝
//                                                                                                            


/**
 * @brief Arcb script lexer (scanner).
 *
 * Lexer is a stateful token generator:
 * - it reads from an input stream (file_ / in_)
 * - tracks current character and source position (line_/col_)
 * - exposes Lexer::next() to obtain the next Token
 *
 * The lexer also stores all source lines (lines vector) to support diagnostics.
 *
 * Ownership/lifetime notes:  
 * - arcscript_ is stored as a reference to the constructor argument, so the
 *   referenced string must outlive the Lexer instance.  
 * - file_ holds the underlying file stream when constructed from a path.
 */
class Lexer 
{
public:
    /**
     * @brief Constructs a lexer for the given script path.
     *
     * @param arcscript Path to the Arcb script to be lexed.
     *
     * @note The constructor stores arcscript as a reference (arcscript_).
     *       The passed string object must outlive this Lexer.
     */
    explicit Lexer(const std::string& arcscript);

    /**
     * @brief Returns the next token in the input stream.
     *
     * The returned Token carries its lexeme and source location metadata.
     * When the end of input is reached, TokenType::ENDOFFILE is returned.
     *
     * @return Next Token from the stream.
     */
    Token next();

    /**
     * @brief Returns the raw source line at index pos (0-based).
     *
     * @param pos 0-based line index.
     * @return Copy of the stored source line.
     */
    inline std::string operator[] (const size_t pos) 
    {
        return lines[pos];
    }

    /**
     * @brief Returns the raw source line containing the given token.
     *
     * Token.line is treated as 1-based, therefore the lookup uses (line - 1).
     *
     * @param token Token whose line should be retrieved.
     * @return Reference to the stored source line.
     */
    inline std::string& operator[] (const Token& token) 
    {
        return lines[token.line - 1];
    }

    /**
     * @brief Returns the source script path used to construct this lexer.
     *
     * @return Reference to the script path string.
     */
    inline const std::string& source() const { return arcscript_; }

private:
    char                     current_;   ///< Current character under examination.
    std::size_t              line_;      ///< Current 1-based line counter.
    std::size_t              col_;       ///< Current column counter (implementation-defined).
    std::size_t              nlcol_;     ///< Column counter used for newline handling (implementation-defined).
    std::ifstream            file_;      ///< File stream backing the lexer (when reading from file).
    std::istream&            in_;        ///< Input stream reference used by the lexer.
    const std::string&       arcscript_; ///< Reference to script path string (must outlive Lexer).
    std::vector<std::string> lines;      ///< Cached source lines for diagnostics.

    /// Loads the entire input stream into the lines vector (diagnostic support).
    void  load_file_lines(std::istream& in);

    /// Advances the stream by one character, updating position counters.
    void  advance();

    /// Skips whitespace (except newlines if NEWLINE is significant).
    void  skipWhitespace();

    /// Creates a token for single-character/simple tokens.
    Token simpleToken(TokenType type);

    /// Scans an identifier or reserved keyword.
    Token identifier();

    /// Scans a numeric literal.
    Token number();

    /// Constructs a token with explicit payload and location.
    Token makeToken(TokenType type, std::string lexeme,
                    std::size_t line, std::size_t start, std::size_t end);
};




END_MODULE(Scan)

/** @} */

#endif /* __ARCB_LEXER__H__ */
