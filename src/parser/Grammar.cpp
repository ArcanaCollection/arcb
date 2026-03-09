#include "Grammar.h"
#include <algorithm>

USE_MODULE(Arcb);
USE_MODULE(Arcb::Grammar);



//    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗
//    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝
//    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗
//    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║
//    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║
//    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
//                                                                                                                                                                               


/**
 * @brief Local helper used to build grammar non-terminals with fluent operators.
 *
 * This helper is used only in this translation unit to build `NonTerminal`
 * instances in a readable way (sequence `|` and alternation `||`).
 */
struct Rules
{
    /**
     * @brief Append a single token as a new terminal node.
     * @param type TokenType to append.
     * @return Reference to this builder.
     */
    Rules& operator |  (const Scan::TokenType type);

    /**
     * @brief Append another builder (concatenate non-terminals).
     * @param streamer Other builder.
     * @return Reference to this builder.
     */
    Rules& operator |  (const Rules& streamer);

    /**
     * @brief Append an alternative TokenType to the last terminal node.
     * @param type TokenType alternative.
     * @return Reference to this builder.
     */
    Rules& operator || (Scan::TokenType type);

    /**
     * @brief The produced non-terminal stream.
     */
    NonTerminal buffer;
};



/**
 * @brief Build a 2-token sequence as a new Rules builder.
 * @param lhs First token in sequence.
 * @param rhs Second token in sequence.
 * @return Builder containing a `NonTerminal` with two terminals.
 */
Rules operator | (Scan::TokenType lhs, Scan::TokenType rhs)
{
    Rules    b;
    Terminal lnode { lhs };
    Terminal rnode { rhs };

    b.buffer.push_back(lnode);
    b.buffer.push_back(rnode);
    return b;
}



/**
 * @brief Build a 2-token alternative as a single terminal node.
 * @param lhs First alternative token.
 * @param rhs Second alternative token.
 * @return Builder containing one terminal with two alternatives.
 */
Rules operator || (Scan::TokenType lhs, Scan::TokenType rhs)
{
    Rules    b;
    Terminal node { lhs, rhs };

    b.buffer.push_back(node);
    return b;
}



/**
 * @brief Append a single token as a new terminal node.
 * @param type TokenType to append.
 * @return Reference to this builder.
 */
Rules& Rules::operator | (const Scan::TokenType type)
{
    Terminal node { type };

    buffer.push_back(node);
    return *this;
}



/**
 * @brief Concatenate another Rules builder buffer into this buffer.
 * @param streamer Other builder to append.
 * @return Reference to this builder.
 */
Rules& Rules::operator | (const Rules& streamer)
{
    buffer.insert(buffer.end(), streamer.buffer.begin(), streamer.buffer.end());
    return *this;
}



/**
 * @brief Append an alternative token to the last terminal node.
 * @param type TokenType alternative.
 * @return Reference to this builder.
 */
Rules& Rules::operator || (const Scan::TokenType type)
{
    buffer.back().push_back(type);
    return *this;
}






//    ██████╗ ██╗   ██╗██╗     ███████╗███████╗
//    ██╔══██╗██║   ██║██║     ██╔════╝██╔════╝
//    ██████╔╝██║   ██║██║     █████╗  ███████╗
//    ██╔══██╗██║   ██║██║     ██╔══╝  ╚════██║
//    ██║  ██║╚██████╔╝███████╗███████╗███████║
//    ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝╚══════╝
//                                                                                                                 




/**
 * @brief Grammar rule: variable assignment.
 *
 * Pattern:
 *   IDENTIFIER '=' ANY (NEWLINE | ';' | EOF)
 */
static Rules rule_VARIABLE_ASSIGNMENT =
{
    Scan::TokenType::IDENTIFIER                           |
    Scan::TokenType::ASSIGN                               |
    Scan::TokenType::ANY                                  |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON || Scan::TokenType::ENDOFFILE )
};


/**
 * @brief Grammar rule: variable join.
 *
 * Pattern:
 *   IDENTIFIER '+=' ANY (NEWLINE | ';' | EOF)
 */
static Rules rule_VARIABLE_JOIN =
{
    Scan::TokenType::IDENTIFIER                           |
    Scan::TokenType::PLUS                                 |
    Scan::TokenType::ASSIGN                               |
    Scan::TokenType::ANY                                  |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON || Scan::TokenType::ENDOFFILE )
};


/**
 * @brief Grammar rule: empty line.
 *
 * Pattern:
 *   (NEWLINE | EOF)
 */
static Rules rule_EMPTY_LINE =
{
    ( Scan::TokenType::NEWLINE || Scan::TokenType::ENDOFFILE )
};



/**
 * @brief Grammar rule: attribute line.
 *
 * Pattern:
 *   '@' IDENTIFIER ANY (NEWLINE | ';')
 */
static Rules rule_ATTRIBUTE =
{
    Scan::TokenType::AT           |
    Scan::TokenType::IDENTIFIER   |
    Scan::TokenType::ANY          |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON )
};

     

/** 
 * @brief Grammar rule: task declaration (including body).
 *
 * Pattern:
 *   'task' IDENTIFIER '(' ANY ')' OPT_NEWLINE '{' ANY '}' (NEWLINE | ';' | EOF)
 */
static Rules rule_TASK_DECL =
{
    Scan::TokenType::TASK         |
    Scan::TokenType::IDENTIFIER   |
    Scan::TokenType::ROUNDLP      |
    Scan::TokenType::ROUNDRP      |
    Scan::TokenType::OPT_NEWLINE  |
    Scan::TokenType::CURLYLP      |
    Scan::TokenType::ANY          |
    Scan::TokenType::CURLYRP      |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON || Scan::TokenType::ENDOFFILE )
};



/**
 * @brief Grammar rule: import directive.
 *
 * Pattern:
 *   'import' ANY (NEWLINE | ';' | EOF)
 */
static Rules rule_IMPORT =
{
    Scan::TokenType::IMPORT       |
    Scan::TokenType::ANY          |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON || Scan::TokenType::ENDOFFILE )
};



/**
 * @brief Grammar rule: using directive.
 *
 * Pattern:
 *   'using' IDENTIFIER ANY (NEWLINE | ';' | EOF)
 */
static Rules rule_USING =
{
    Scan::TokenType::USING        |
    Scan::TokenType::IDENTIFIER   |
    Scan::TokenType::ANY          |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON || Scan::TokenType::ENDOFFILE )
};



/**
 * @brief Grammar rule: mapping directive.
 *
 * Pattern:
 *   'map' IDENTIFIER '-' '>' IDENTIFIER (NEWLINE | ';' | EOF)
 */
static Rules rule_MAP =
{
    Scan::TokenType::MAPPING      |
    Scan::TokenType::IDENTIFIER   |
    Scan::TokenType::MINUS        |
    Scan::TokenType::ANGULARRP    |
    Scan::TokenType::IDENTIFIER   |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON || Scan::TokenType::ENDOFFILE )
};



/**
 * @brief Grammar rule: assert directive.
 *
 * Pattern:
 *   'assert' '"' ANY '"' (EQ | NE | IN) '"' ANY '"' '-' '>' '"' ANY '"' (NEWLINE | ';' | EOF)
 */
static Rules rule_ASSERT_MSG =
{
    Scan::TokenType::ASSERT                         |
    Scan::TokenType::DQUOTE                         |
    Scan::TokenType::ANY                            |
    Scan::TokenType::DQUOTE                         |
    ( Scan::TokenType::EQ || Scan::TokenType::NE || Scan::TokenType::IN )  |
    Scan::TokenType::DQUOTE                         |
    Scan::TokenType::ANY                            |
    Scan::TokenType::DQUOTE                         |
    Scan::TokenType::MINUS                          |
    Scan::TokenType::ANGULARRP                      |
    Scan::TokenType::DQUOTE                         |
    Scan::TokenType::ANY                            |
    Scan::TokenType::DQUOTE                         |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON || Scan::TokenType::ENDOFFILE )
};



/**
 * @brief Grammar rule: assert directive.
 *
 * Pattern:
 *   'assert' '"' ANY '"' (EQ | NE | IN) '"' ANY '"' '-' '>' ANY (NEWLINE | ';' | EOF)
 */
static Rules rule_ASSERT_ACT =
{
    Scan::TokenType::ASSERT                         |
    Scan::TokenType::DQUOTE                         |
    Scan::TokenType::ANY                            |
    Scan::TokenType::DQUOTE                         |
    ( Scan::TokenType::EQ || Scan::TokenType::NE || Scan::TokenType::IN )  |
    Scan::TokenType::DQUOTE                         |
    Scan::TokenType::ANY                            |
    Scan::TokenType::DQUOTE                         |
    Scan::TokenType::MINUS                          |
    Scan::TokenType::ANGULARRP                      |
    Scan::TokenType::ANY                            |
    ( Scan::TokenType::NEWLINE || Scan::TokenType::SEMICOLON || Scan::TokenType::ENDOFFILE )
};






//     ██████╗██╗      █████╗ ███████╗███████╗    ██╗███╗   ███╗██████╗ ██╗     
//    ██╔════╝██║     ██╔══██╗██╔════╝██╔════╝    ██║████╗ ████║██╔══██╗██║     
//    ██║     ██║     ███████║███████╗███████╗    ██║██╔████╔██║██████╔╝██║     
//    ██║     ██║     ██╔══██║╚════██║╚════██║    ██║██║╚██╔╝██║██╔═══╝ ██║     
//    ╚██████╗███████╗██║  ██║███████║███████║    ██║██║ ╚═╝ ██║██║     ███████╗
//     ╚═════╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝    ╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝
//                                                                                                                                                                  


/**
 * @brief Construct grammar engine and initialize production rules and index buffers.
 *
 * The engine pre-builds:
 * - `_rules` map: Rule -> NonTerminal
 * - `_index` map: Rule -> vector of Index entries (one per terminal node)
 */
Engine::Engine()
{
    // REGISTER PRODUCTIONS
    _rules[Rule::VARIABLE_ASSIGN  ] = rule_VARIABLE_ASSIGNMENT.buffer;
    _rules[Rule::VARIABLE_JOIN    ] = rule_VARIABLE_JOIN.buffer;
    _rules[Rule::EMPTY_LINE       ] = rule_EMPTY_LINE.buffer;
    _rules[Rule::ATTRIBUTE        ] = rule_ATTRIBUTE.buffer;
    _rules[Rule::TASK_DECL        ] = rule_TASK_DECL.buffer;
    _rules[Rule::IMPORT           ] = rule_IMPORT.buffer;
    _rules[Rule::USING            ] = rule_USING.buffer;
    _rules[Rule::MAPPING          ] = rule_MAP.buffer;
    _rules[Rule::ASSERT_MSG       ] = rule_ASSERT_MSG.buffer;
    _rules[Rule::ASSERT_ACT       ] = rule_ASSERT_ACT.buffer;

    // ALLOCATE INDEX BUFFERS (ONE PER RULE POSITION)
    _index[Rule::VARIABLE_ASSIGN  ] = std::vector<Index>(rule_VARIABLE_ASSIGNMENT.buffer.size());
    _index[Rule::VARIABLE_JOIN    ] = std::vector<Index>(rule_VARIABLE_JOIN.buffer.size());
    _index[Rule::EMPTY_LINE       ] = std::vector<Index>(rule_EMPTY_LINE.buffer.size());
    _index[Rule::ATTRIBUTE        ] = std::vector<Index>(rule_ATTRIBUTE.buffer.size());
    _index[Rule::TASK_DECL        ] = std::vector<Index>(rule_TASK_DECL.buffer.size());
    _index[Rule::IMPORT           ] = std::vector<Index>(rule_IMPORT.buffer.size());
    _index[Rule::USING            ] = std::vector<Index>(rule_USING.buffer.size());
    _index[Rule::MAPPING          ] = std::vector<Index>(rule_MAP.buffer.size());
    _index[Rule::ASSERT_MSG       ] = std::vector<Index>(rule_ASSERT_MSG.buffer.size());
    _index[Rule::ASSERT_ACT       ] = std::vector<Index>(rule_ASSERT_ACT.buffer.size());
}



/**
 * @brief Feed one token into the grammar engine and update match state.
 *
 * This function implements a rule-stream matching procedure with caching:
 * - keeps a set of candidate rules (`_cache.keys`)
 * - advances per-rule cursor (`_cache.data[Rule]`)
 * - tracks matched spans (`_index[Rule][pos]`)
 * - produces a complete match when a rule reaches its end
 *
 * @param token Incoming lexer token.
 * @param match Output match structure updated in-place.
 */
void Engine::match(const Scan::Token& token, Match& match)
{
    bool                         error       = false;
    bool                         cached      = false;
    bool                         matched     = false;
    bool                         remove      = false;
    uint32_t                     position    = 0;
    Scan::TokenType              ttype       = token.type;
    Rule                         stype       = Rule::UNDEFINED;
    UniqueNonTerminal            estream;
    UniqueRule                   semtypes;
    std::set<Rule>               new_key_cache;
    Terminal::const_iterator     found;
    Terminal::const_iterator     wildcard;
    Terminal::const_iterator     opttoken;

    // INITIALIZE THE CACHE ON FIRST RUN
    if (_cache.data.size() == 0)
    {
        for (const auto& pair : _rules)
        {
            _cache.keys.insert(pair.first);
        }
    }
    else
    {
        cached = true;
    }

    // GET THE WORKING KEY SET
    UniqueRule& keys = _cache.keys;

    // TRY MATCHING ALL CANDIDATE RULES
    for (auto it = keys.begin(); it != keys.end() && !matched; )
    {
        // LOAD CURRENT RULE STATE
        const auto  key   = *it;
        const auto& value = _rules[key];
        position          = cached ? _cache.data[key] : 0;
        remove            = false;

        // CHECK RULE CURSOR BOUNDS
        if (position < value.size())
        {
            // READ CURRENT NODE AND SEARCH TOKEN
            auto& node     = value[position];
                  found    = std::find(node.begin(), node.end(), ttype);
                  wildcard = std::find(node.begin(), node.end(), Scan::TokenType::ANY);
                  opttoken = std::find(node.begin(), node.end(), Scan::TokenType::OPT_NEWLINE);

            // ACCUMULATE ERROR STREAM CONTEXT
            estream.insert(node);
            semtypes.insert(key);

            // REGULAR TOKEN MATCH
            if (found != node.end())
            {
                // COLLECT TOKEN SPAN
                _collect_input(token, *found, key, position);

                // ADVANCE CURSOR AND CHECK COMPLETION
                _cache.data[key] = ++position;
                matched          = (position == value.size());

                // RECORD MATCH TYPE ON COMPLETION
                if (matched)
                {
                    stype = key;
                }

                // UPDATE CURLY BRACES COUNTER FOR TASK BODY
                if (key == Rule::TASK_DECL)
                {
                    if (ttype == Scan::TokenType::CURLYLP)
                    {
                        ++_cache.opened_curly_braces;
                    }
                    else if (ttype == Scan::TokenType::CURLYRP)
                    {
                        --_cache.opened_curly_braces;
                    }
                }

                // KEEP RULE IN NEXT ITERATION SET
                new_key_cache.insert(key);
            }

            // WILDCARD NODE (ANY)
            else if (wildcard != node.end())
            {
                // LOOKAHEAD TO DECIDE WHETHER TO CONSUME AS ANY OR ADVANCE
                position++;
                auto& lookahead = value[position];
                found           = std::find(lookahead.begin(), lookahead.end(), ttype);

                // UPDATE CURLY BRACES COUNTER FOR TASK BODY
                if (key == Rule::TASK_DECL)
                {
                    if (ttype == Scan::TokenType::CURLYLP)
                    {
                        ++_cache.opened_curly_braces;
                    }
                    else if (ttype == Scan::TokenType::CURLYRP)
                    {
                        --_cache.opened_curly_braces;
                    }
                }

                // IF LOOKAHEAD DOES NOT MATCH, CONSUME TOKEN AS ANY
                if (found == lookahead.end())
                {
                    position -= 1;
                    _collect_input(token, *wildcard, key, position);
                }
                else
                {
                    // SPECIAL HANDLING FOR TASK BODY TERMINATION
                    if (key == Rule::TASK_DECL)
                    {
                        if (_cache.opened_curly_braces == 0)
                        {
                            position += 1;
                            _collect_input(token, *found, key, position - 1);
                        }
                        else
                        {
                            position -= 1;
                            _collect_input(token, *wildcard, key, position);
                        }
                    }
                    else
                    {
                        position += 1;
                        _collect_input(token, *found, key, position - 1);
                    }
                }

                // CACHE CURSOR AND CHECK COMPLETION
                _cache.data[key] = position;
                matched          = (position == value.size());

                // RECORD MATCH TYPE ON COMPLETION
                if (matched)
                {
                    stype = key;
                }
            }

            // OPTIONAL NEWLINE NODE
            else if (opttoken != node.end())
            {
                // ADVANCE RULE CURSOR OVER OPTIONAL SLOT
                _cache.data[key] = ++position;

                // CONSUME TOKEN ONLY IF IT IS A NEWLINE
                if (ttype == Scan::TokenType::NEWLINE)
                {
                    _collect_input(token, ttype, key, position);
                }
                else
                {
                    continue;
                }
            }

            // NO MATCH FOR THIS RULE
            else
            {
                remove = true;
            }

            // REMOVE RULE FROM CANDIDATES
            if (remove)
            {
                it = keys.erase(it);

                // IF NO RULES LEFT, EMIT ERROR AND RESET CACHE IF NEEDED
                if (keys.empty())
                {
                    error = true;

                    if (cached)
                    {
                        _cache.reset();
                    }

                    break;
                }
            }
            else
            {
                ++it;
            }
        }
    }

    // UPDATE CANDIDATE SET FOR NEXT TOKEN
    if (new_key_cache.size())
    {
        _cache.keys = new_key_cache;
    }

    // POPULATE OUTPUT MATCH STRUCT
    match.valid          = matched;
    match.type           = stype;
    match.indexes        = _index[stype];
    match.Error.token    = token;
    match.Error.estream  = estream;
    match.Error.semtypes = semtypes;
    match.Error.presence = error;

    // RESET STATE AFTER A COMPLETE MATCH
    if (matched)
    {
        _reset();
    }
}



/**
 * @brief Record token span information into the per-rule index buffer.
 *
 * The index buffer tracks:
 * - `start`: where a token (or an ANY region) begins
 * - `end`: where the current token ends
 * - `token`: the latest token associated with this position
 *
 * @param token Current token.
 * @param tt    Token type used for this rule node (may be ANY).
 * @param st    Semantic rule currently being matched.
 * @param pos   Position inside the rule stream.
 */
void Engine::_collect_input(const Scan::Token& token, const Scan::TokenType tt, const Rule st, const uint32_t pos)
{
    // GET INDEX SLOT FOR THIS RULE POSITION
    auto& index = _index[st][pos];

    // STORE TOKEN AND END OFFSET
    index.token = token;
    index.end   = token.start + token.lexeme.size();

    // HANDLE ANY RANGE START/CONTINUATION
    if (tt == Scan::TokenType::ANY)
    {
        if (index.any == false)
        {
            index.start = token.start;
            index.any   = true;
        }
    }
    else
    {
        // HANDLE REGULAR TOKEN START OR ANY RANGE TERMINATION
        if (index.any == false)
        {
            index.start = token.start;
        }
        else
        {
            index.any = false;
        }
    }
}



/**
 * @brief Reset engine cache and clear all index entries.
 *
 * This function is called after a full rule match to ensure the next statement
 * starts from a clean state.
 */
void Engine::_reset()
{
    // RESET CACHE STATE
    _cache.reset();

    // RESET ALL INDEX SLOTS
    for (auto& pair : _index)
    {
        for (auto& idx : pair.second)
        {
            idx.reset();
        }
    }
}
