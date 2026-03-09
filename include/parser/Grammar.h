#ifndef __ARCB_GRAMMAR__H__
#define __ARCB_GRAMMAR__H__


#include <map>
#include <set>
#include <vector>

#include "Lexer.h"
#include "Defines.h"
#include "Support.h"



/**
 * @file Grammar.h
 * @brief Token stream matching engine and grammar rule metadata.
 *
 * This header defines the runtime matcher used by Arcb to classify the
 * incoming token stream into higher-level language constructs (rules).
 *
 * The grammar engine is intended to be fed one token at a time. It collects
 * sufficient context to decide whether the current stream prefix matches a
 * known rule, and when a rule is complete it emits a Match describing:
 * - which rule was recognized
 * - which spans/tokens were captured at each positional index
 *
 * On invalid token sequences, the engine can also populate diagnostic fields
 * to support meaningful error messages (unexpected token, expected terminals,
 * candidate rules, etc.).
 *
 * Provided entities:
 * - Grammar::Rule: high-level rule identifiers
 * - Per-rule positional enums (VARIABLE_ASSIGN, TASK_DECL, ...)
 * - Grammar::Index: capture span and token metadata
 * - Grammar::Match: rule match + error diagnostic payload
 * - Grammar::Engine: runtime matching engine
 */

/**
 * @defgroup Grammar Grammar Engine
 * @brief Token stream matching engine and grammar rule metadata.
 *
 * The Grammar module consumes tokens produced by the scanner and recognizes
 * Arcb language constructs. Each recognized construct is represented by
 * a Rule value and a list of positional captures (Index entries).
 *
 * Typical flow:
 * 1. Scanner produces tokens (Scan::Token).
 * 2. Grammar::Engine consumes tokens in order.
 * 3. Grammar::Match is updated to signal:
 *    - successful recognition (Match::valid)
 *    - error condition with diagnostics (Match::Error.presence)
 */

/**
 * @addtogroup Grammar
 * @{
 */

BEGIN_MODULE(Grammar)




//    ███████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ██████╗ ███████╗
//    ██╔════╝██╔═══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔══██╗██╔════╝
//    █████╗  ██║   ██║██████╔╝██║ █╗ ██║███████║██████╔╝██║  ██║███████╗
//    ██╔══╝  ██║   ██║██╔══██╗██║███╗██║██╔══██║██╔══██╗██║  ██║╚════██║
//    ██║     ╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║██████╔╝███████║
//    ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝
//                                                                                                                                                                                                                                                                                                                                            

class Engine;





//    ██████╗ ██╗   ██╗██████╗ ██╗     ██╗ ██████╗    ███████╗███╗   ██╗██╗   ██╗███╗   ███╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗██║     ██║██╔════╝    ██╔════╝████╗  ██║██║   ██║████╗ ████║██╔════╝
//    ██████╔╝██║   ██║██████╔╝██║     ██║██║         █████╗  ██╔██╗ ██║██║   ██║██╔████╔██║███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗██║     ██║██║         ██╔══╝  ██║╚██╗██║██║   ██║██║╚██╔╝██║╚════██║
//    ██║     ╚██████╔╝██████╔╝███████╗██║╚██████╗    ███████╗██║ ╚████║╚██████╔╝██║ ╚═╝ ██║███████║
//    ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝    ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝
//                                                                                                                                                                                                                  


/// @brief Enum used to define rules
///
/// This is the high-level classification produced by the grammar engine.
/// Each rule has an associated positional enum describing the meaning of
/// captures inside Match::indexes.
enum class Rule : uint32_t
{
    /// No rule recognized / initial state
    UNDEFINED          = 0,

    /// Variable assignment statement (e.g. NAME = VALUE)
    VARIABLE_ASSIGN       ,

    /// Variable join statement (e.g. NAME += VALUE)
    VARIABLE_JOIN         ,

    /// Empty line (e.g. newline / whitespace-only line)
    EMPTY_LINE            ,

    /// Attribute statement (e.g. @attr or @attr option)
    ATTRIBUTE             ,

    /// Task declaration (e.g. task Name(args) { ... })
    TASK_DECL             ,

    /// Import statement (e.g. import "file.arc")
    IMPORT                ,

    /// Using statement (e.g. using profiles ..., using threads ..., etc.)
    USING                 ,

    /// Mapping statement (e.g. map A -> B)
    MAPPING               ,

    /// Assert statement (e.g. assert "x" in "{fs:...}" -> "reason")
    ASSERT_MSG            ,

    /// Assert statement (e.g. assert "x" in "{fs:...}" -> action list)
    ASSERT_ACT            ,
};



/**
 * @brief Positional capture indices for Rule::VARIABLE_ASSIGN.
 *
 * Captures describe the structure of a variable assignment statement.
 * Consumers interpret Match::indexes positions using these values.
 */
enum class VARIABLE_ASSIGN : uint32_t 
{
    /// Variable name token/span (left-hand identifier)
    VARNAME            = 0,

    /// Assignment operator token/span (typically '=')
    ASSIGN                ,

    /// Assigned value token/span (right-hand side)
    VALUE                 ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};


enum class VARIABLE_JOIN : uint32_t 
{
    /// Variable name token/span (left-hand identifier)
    VARNAME            = 0,

    PLUS                  ,

    /// Assignment operator token/span (typically '=')
    ASSIGN                ,

    /// Assigned value token/span (right-hand side)
    VALUE                 ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};


/**
 * @brief Positional capture indices for Rule::EMPTY_LINE.
 *
 * Empty line typically has no meaningful captures; GRAMMAR_END is the sentinel.
 */
enum class EMPTY_LINE : uint32_t 
{
    /// Sentinel: end of this positional enum
    GRAMMAR_END        = 0,
};



/**
 * @brief Positional capture indices for Rule::ATTRIBUTE.
 *
 * Attributes begin with '@' and may carry a name and optional option/value.
 */
enum class ATTRIBUTE : uint32_t 
{
    /// '@' token/span
    AT                 = 0,

    /// Attribute name token/span (e.g. "main", "pub", "requires", ...)
    ATTRNAME              ,

    /// Optional attribute option/value token/span (depends on attribute)
    ATTROPTION            ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};
  
 

/**
 * @brief Positional capture indices for Rule::TASK_DECL.
 *
 * A task declaration includes:
 * - reserved keyword ("task")
 * - task name
 * - input list in parentheses (optional/empty)
 * - a body delimited by curly braces containing instructions
 *
 * The exact tokenization of INSTRUCTIONS depends on your lexer strategy.
 */
enum class TASK_DECL : uint32_t 
{
    /// Reserved keyword token/span (e.g. "task")
    RESERVED           = 0,

    /// Task name token/span
    TASKNAME              ,

    /// '(' token/span
    ROUNDLP               ,

    /// ')' token/span
    ROUNDRP               ,

    /// Newline token/span separating signature from body (if required)
    NEWLINE               ,

    /// '{' token/span opening the body
    CURLYLP               ,

    /// Instructions token/span (body content)
    INSTRUCTIONS          ,

    /// '}' token/span closing the body
    CURLYRP               ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};



/**
 * @brief Positional capture indices for Rule::IMPORT.
 *
 * Import statements typically capture the reserved keyword and a script path.
 */
enum class IMPORT : uint32_t 
{
    /// Reserved keyword token/span (e.g. "import")
    RESERVED           = 0,

    /// Script specifier token/span (path / string literal)
    SCRIPT                ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};



/**
 * @brief Positional capture indices for Rule::USING.
 *
 * Using statements configure environment aspects (profiles, engine, threads, etc.).
 * OPT meaning is subcommand-dependent.
 */
enum class USING : uint32_t 
{
    /// Reserved keyword token/span (e.g. "using")
    RESERVED           = 0,

    /// Subject token/span (what is being configured: profiles/default/engine/threads/...)
    WHAT                  ,

    /// Optional parameter token/span (e.g. list of items, numeric value, etc.)
    OPT                   ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};



/**
 * @brief Positional capture indices for Rule::MAPPING.
 *
 * Mappings describe transforms (e.g. map A -> B).
 * RESERVED values typically correspond to fixed keywords/operators.
 */
enum class MAPPING : uint32_t 
{
    /// Reserved keyword token/span (e.g. "map")
    RESERVED1          = 0,

    /// Left item token/span (source)
    ITEM_1                ,

    /// Reserved token/span (e.g. whitespace or fixed keyword, depends on lexer)
    RESERVED2             ,

    /// Reserved token/span (e.g. "->" operator or part of it)
    RESERVED3             ,

    /// Right item token/span (destination)
    ITEM_2                ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};



/**
 * @brief Positional capture indices for Rule::ASSERT_MSG.
 *
 * Assertions typically have the form:
 * assert ITEM_1 OP ITEM_2 -> REASON
 *
 * Many RESERVED slots exist because the matcher preserves positional structure
 * for fixed keywords/operators/punctuation as part of the grammar definition.
 */
enum class ASSERT_MSG : uint32_t 
{
    /// Reserved keyword token/span (e.g. "assert")
    RESERVED1          = 0,

    /// Reserved token/span (grammar-specific)
    RESERVED2             ,

    /// Left item token/span (assertion LHS)
    ITEM_1                ,

    /// Reserved token/span (grammar-specific)
    RESERVED3             ,

    /// Operator token/span (e.g. "in", "==", "!=", ...)
    OP                    ,

    /// Reserved token/span (grammar-specific)
    RESERVED4             ,

    /// Right item token/span (assertion RHS)
    ITEM_2                ,

    /// Reserved token/span (grammar-specific)
    RESERVED5             ,

    /// Reserved token/span (grammar-specific)
    RESERVED6             ,

    /// Reserved token/span (grammar-specific)
    RESERVED7             ,

    /// Reserved token/span (grammar-specific)
    RESERVED8             ,

    /// Human-readable reason token/span (typically a string literal)
    REASON                ,

    /// Reserved token/span (grammar-specific)
    RESERVED9             ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};



/**
 * @brief Positional capture indices for Rule::ASSERT_ACT.
 *
 * Assertions typically have the form:
 * assert ITEM_1 OP ITEM_2 -> ACTION
 *
 * Many RESERVED slots exist because the matcher preserves positional structure
 * for fixed keywords/operators/punctuation as part of the grammar definition.
 */
enum class ASSERT_ACT : uint32_t 
{
    /// Reserved keyword token/span (e.g. "assert")
    RESERVED1          = 0,

    /// Reserved token/span (grammar-specific)
    RESERVED2             ,

    /// Left item token/span (assertion LHS)
    ITEM_1                ,

    /// Reserved token/span (grammar-specific)
    RESERVED3             ,

    /// Operator token/span (e.g. "in", "==", "!=", ...)
    OP                    ,

    /// Reserved token/span (grammar-specific)
    RESERVED4             ,

    /// Right item token/span (assertion RHS)
    ITEM_2                ,

    /// Reserved token/span (grammar-specific)
    RESERVED5             ,

    /// Reserved token/span (grammar-specific)
    RESERVED6             ,

    /// Reserved token/span (grammar-specific)
    RESERVED7             ,

    /// Callbacks
    ACTIONS               ,

    /// Sentinel: end of this positional enum
    GRAMMAR_END           ,
};


//    ██████╗ ██╗   ██╗██████╗ ██╗     ██╗ ██████╗     █████╗  ██████╗  ██████╗ ██████╗ ███████╗ ██████╗  █████╗ ████████╗███████╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗██║     ██║██╔════╝    ██╔══██╗██╔════╝ ██╔════╝ ██╔══██╗██╔════╝██╔════╝ ██╔══██╗╚══██╔══╝██╔════╝██╔════╝
//    ██████╔╝██║   ██║██████╔╝██║     ██║██║         ███████║██║  ███╗██║  ███╗██████╔╝█████╗  ██║  ███╗███████║   ██║   █████╗  ███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗██║     ██║██║         ██╔══██║██║   ██║██║   ██║██╔══██╗██╔══╝  ██║   ██║██╔══██║   ██║   ██╔══╝  ╚════██║
//    ██║     ╚██████╔╝██████╔╝███████╗██║╚██████╗    ██║  ██║╚██████╔╝╚██████╔╝██║  ██║███████╗╚██████╔╝██║  ██║   ██║   ███████╗███████║
//    ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝    ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝╚══════╝
//                                                                                                                                                                                                                                                         


/**
 * @brief Captured token span and associated metadata.
 *
 * Index represents a capture produced during rule matching.
 *
 * Semantics:
 * - start/end are logical offsets in the input stream. They are not guaranteed
 *   to be byte offsets unless your scanner defines them that way.
 * - token stores the last (or representative) token associated with this capture.
 *
 * Lifetime/ownership:
 * - Index is primarily managed by Grammar::Engine (friend).
 * - External consumers should treat it as read-only.
 */
class Index
{
    friend Engine;
public:
    Index() : start(0), end(0), any(false) {}

    /// Start offset (inclusive) of the capture span in the input stream.
    std::size_t start;

    /// End offset (exclusive) of the capture span in the input stream.
    std::size_t end;

    /// Token associated with this capture.
    Scan::Token token;

private:
    /// Internal matcher flag used by Engine to track presence/state.
    bool any;

    /// Reset capture state (Engine-internal).
    void reset() noexcept
    {
        start = 0;
        end   = 0;
        any   = false;
    }
};



/**
 * @brief Match produced by Grammar::Engine.
 *
 * A Match can be in three logical states:
 * - valid == true: a rule has been recognized and Match::type / indexes are meaningful.
 * - Error.presence == true: an error occurred; Error payload describes the diagnostics.
 * - neither: no completed rule yet (engine is still collecting tokens).
 *
 * One-shot queries:
 * - isValid() returns the current valid flag AND clears it.
 * - isError() returns the current error flag AND clears it.
 *
 * This supports a streaming API where the caller checks flags per token without
 * repeating the same event multiple times.
 */
struct Match
{
    /// True if a rule was successfully matched.
    bool               valid;

    /// Matched rule identifier.
    Rule               type;

    /// Captured spans/tokens for the matched rule.
    std::vector<Index> indexes;
    
    struct
    {
        /// Token that caused the engine to enter the error state.
        Scan::Token       token;

        /// Expected terminal alternatives at the error point (diagnostic).
        UniqueNonTerminal estream;

        /// Candidate rules compatible with the already seen prefix (diagnostic).
        UniqueRule        semtypes;

        /// True if this error payload is present/valid.
        bool              presence;
    } Error;

    /// @brief Returns current validity and clears the valid flag.
    bool isValid () noexcept { bool v = valid         ; valid          = false; return v; }

    /// @brief Returns current error presence and clears the error flag.
    bool isError () noexcept { bool v = Error.presence; Error.presence = false; return v; }

    /**
     * @brief Direct access to a capture index.
     *
     * @param s Capture index (positional, interpreted via per-rule enums).
     * @return Pointer to the capture at position s.
     *
     * @note No bounds check is performed.
     */
    const Index* operator [] (const std::size_t s) { return &indexes[s]; }
};



/**
 * @brief Runtime grammar matching engine.
 *
 * The engine consumes scanner tokens and attempts to recognize the Arcb grammar.
 *
 * Expected usage:
 * - Create Engine once.
 * - For each token produced by the lexer, call match(token, match_out).
 * - The match_out object will be updated to signal completion or errors.
 *
 * Internals:
 * - _rules holds grammar productions.
 * - _index stores capture indices per rule.
 * - EngineCache tracks current in-flight rule candidates and auxiliary state
 *   (e.g. opened curly braces for multi-line bodies).
 */
class Engine
{
public:
    /// Constructs and initializes internal grammar tables.
    Engine();

    /**
     * @brief Feeds a token to the engine and updates the match object.
     *
     * On completion:
     * - match.valid will be set and match.type/indexes are meaningful.
     *
     * On failure:
     * - match.Error.presence will be set and match.Error will contain
     *   diagnostic information (offending token, expected terminals, candidates).
     *
     * @param token Input token from Scan::Lexer.
     * @param match Output match updated by the engine.
     */
    void match(const Scan::Token& token, Match& match);
    
private:
    void _collect_input(const Scan::Token& token, const Scan::TokenType tt, const Rule st, const uint32_t pos);
    void _reset();

    struct EngineCache
    {
        EngineCache() : opened_curly_braces(0) {}

        /// Candidate rules still compatible with the current input prefix.
        UniqueRule                  keys;

        /// Per-rule cursor/progress position in the production.
        std::map<Rule, uint32_t>    data;

        /// Per-rule captured raw items (implementation-defined usage).
        std::map<Rule, std::string> items;

        /// Tracks nested curly braces when parsing task bodies.
        std::size_t                 opened_curly_braces;

        /// Resets the cache to the initial state.
        void reset() { keys.clear(); data.clear(); items.clear(); opened_curly_braces = 0; }
    } _cache;
    
    /// Grammar productions table.
    Production                         _rules;

    /// Captured indices per rule during matching.
    std::map<Rule, std::vector<Index>> _index;
};



END_MODULE(Grammar)

/** @} */

#endif /* __ARCB_GRAMMAR__H__ */
