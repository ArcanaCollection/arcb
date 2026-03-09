#ifndef __ARCANA_PARSER__H__
#define __ARCANA_PARSER__H__

#include "Lexer.h"
#include "Defines.h"
#include "Grammar.h"
#include "Semantic.h"

#include <functional>



/**
 * @file Parser.h
 * @brief High-level parsing pipeline: scan + grammar match + semantic build.
 *
 * This header defines the Parsing layer which orchestrates:
 * - tokenization through Scan::Lexer
 * - grammar recognition through Grammar::Engine
 * - semantic/environment construction through Semantic::Engine
 *
 * The Parser acts as a coordinator: it consumes tokens, asks Grammar::Engine
 * to match them into high-level rules, then dispatches to semantic handlers
 * that populate/modify a Semantic::Enviroment.
 */

/**
 * @defgroup Parsing Parsing Pipeline
 * @brief Orchestrates the Arcana parsing pipeline (lexer + grammar + semantic).
 *
 * The Parsing module owns the high-level control flow:
 * - iterate tokens from Scan::Lexer
 * - match rules via Grammar::Engine into Grammar::Match
 * - execute semantic actions to build/update Semantic::Enviroment
 * - optionally invoke user-provided error handlers
 */

/**
 * @addtogroup Parsing
 * @{
 */

BEGIN_MODULE(Parsing)



//     ██████╗ █████╗ ██╗     ██╗     ██████╗  █████╗  ██████╗██╗  ██╗███████╗
//    ██╔════╝██╔══██╗██║     ██║     ██╔══██╗██╔══██╗██╔════╝██║ ██╔╝██╔════╝
//    ██║     ███████║██║     ██║     ██████╔╝███████║██║     █████╔╝ ███████╗
//    ██║     ██╔══██║██║     ██║     ██╔══██╗██╔══██║██║     ██╔═██╗ ╚════██║
//    ╚██████╗██║  ██║███████╗███████╗██████╔╝██║  ██║╚██████╗██║  ██╗███████║
//     ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═════╝ ╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝
//                                                                                                                                                       

/**
 * @brief Callback invoked on parsing (grammar) errors.
 *
 * The callback receives:
 * - ctx: a human-readable context string describing the stage/location
 * - match: the Grammar::Match containing diagnostic fields (expected terminals, etc.)
 *
 * @return Arcana_Result error code to propagate to the caller.
 */
using ParsingError  = std::function<Arcana_Result (const std::string& ctx, const Grammar::Match&)>;

/**
 * @brief Callback invoked on semantic analysis errors.
 *
 * The callback receives:
 * - ctx: a human-readable context string describing the stage/location
 * - SemanticOutput: semantic engine result and optional hint
 * - match: the Grammar::Match that triggered the semantic action
 *
 * @return Arcana_Result error code to propagate to the caller.
 */
using AnalisysError = std::function<Arcana_Result (const std::string& ctx, const Support::SemanticOutput&, const Grammar::Match&)>;

/**
 * @brief Callback invoked on post-processing errors.
 *
 * Post-processing refers to phases executed after syntactic/semantic build
 * (e.g. environment expansion, validation, mapping instantiation, etc.).
 *
 * @return Arcana_Result error code to propagate to the caller.
 */
using PostProcError = std::function<Arcana_Result (const std::string& ctx, const std::string& err)>;




//     ██████╗██╗      █████╗ ███████╗███████╗███████╗███████╗
//    ██╔════╝██║     ██╔══██╗██╔════╝██╔════╝██╔════╝██╔════╝
//    ██║     ██║     ███████║███████╗███████╗█████╗  ███████╗
//    ██║     ██║     ██╔══██║╚════██║╚════██║██╔══╝  ╚════██║
//    ╚██████╗███████╗██║  ██║███████║███████║███████╗███████║
//     ╚═════╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚══════╝
//                                                                                                                                                                  


/**
 * @brief High-level parser that builds a Semantic::Enviroment from an Arcana script.
 *
 * Parser coordinates three stages:
 * 1) Lexing: Scan::Lexer produces tokens.
 * 2) Grammar recognition: Grammar::Engine matches tokens into rules (Grammar::Match).
 * 3) Semantic actions: Semantic::Engine (instr_engine) interprets matches and fills env.
 *
 * Error handling strategy:
 * - Grammar-level mismatches can be routed to ParsingError callback (Parsing_Error).
 * - Semantic action failures return Support::SemanticOutput and can be routed to
 *   AnalisysError callback (Analisys_Error).
 * - Any later post-processing step can report errors via PostProcError callback.
 *
 * Ownership/lifetime:
 * - Parser stores references to lexer and grammar engine (must outlive Parser).
 * - Semantic::Engine is owned by Parser (instr_engine).
 */
class Parser
{
public:
    /**
     * @brief Constructs a parser with externally-owned lexer and grammar engine.
     *
     * @param l Lexer instance used as token source.
     * @param e Grammar engine used to match rule patterns.
     */
    Parser(Scan::Lexer& l, Grammar::Engine& e);

    /**
     * @brief Parses input tokens and populates the provided environment.
     *
     * This function is expected to:
     * - iterate tokens from lexer
     * - update Grammar::Match via Grammar::Engine::match
     * - dispatch to semantic handlers per recognized rule
     * - optionally perform post-processing on env
     *
     * @param env Output environment populated/updated by parsing.
     * @return Arcana_Result status code.
     */
    Arcana_Result Parse(Semantic::Enviroment& env);

private:
    /// Lexer reference (external ownership).
    Scan::Lexer&     lexer;

    /// Grammar engine reference (external ownership).
    Grammar::Engine& engine;

    /// Semantic engine owned by the parser (drives env construction).
    Semantic::Engine instr_engine;


    /**
     * @brief Handles Rule::VARIABLE_ASSIGN semantic action.
     *
     * @param match Grammar match describing a variable assignment.
     * @return SemanticOutput describing success or semantic error.
     */
    Arcana_Result Handle_VarAssign(Grammar::Match& match);


    Arcana_Result Handle_VarJoin(Grammar::Match& match);


    /**
     * @brief Handles Rule::ATTRIBUTE semantic action.
     *
     * @param match Grammar match describing an attribute statement.
     * @return SemanticOutput describing success or semantic error.
     */
    Arcana_Result Handle_Attribute(Grammar::Match& match);

    /**
     * @brief Handles Rule::TASK_DECL semantic action.
     *
     * @param match Grammar match describing a task declaration.
     * @return SemanticOutput describing success or semantic error.
     */
    Arcana_Result Handle_TaskDecl(Grammar::Match& match);

    /**
     * @brief Handles Rule::USING semantic action.
     *
     * @param match Grammar match describing a using statement.
     * @return SemanticOutput describing success or semantic error.
     */
    Arcana_Result Handle_Using(Grammar::Match& match);

    /**
     * @brief Handles Rule::MAPPING semantic action.
     *
     * @param match Grammar match describing a mapping statement.
     * @return SemanticOutput describing success or semantic error.
     */
    Arcana_Result Handle_Mapping(Grammar::Match& match);

    /**
     * @brief Handles Rule::ASSERT_MSG semantic action.
     *
     * @param match Grammar match describing an assert statement.
     * @return SemanticOutput describing success or semantic error.
     */
    Arcana_Result Handle_Assert(Grammar::Match& match, bool actions);
    
    /**
     * @brief Handles Rule::IMPORT by parsing a referenced script into a new environment.
     *
     * Typical use: build a separate environment from the imported file and
     * then merge it with the caller environment (merge strategy is implementation-defined).
     *
     * @param match Grammar match describing the import statement.
     * @param new_env Output environment produced by parsing the imported script.
     * @return Arcana_Result status code.
     */
    Arcana_Result Handle_Import(Grammar::Match& match, Semantic::Enviroment& new_env);
};



END_MODULE(Parsing)

/** @} */

#endif /* __ARCANA_PARSER__H__ */
