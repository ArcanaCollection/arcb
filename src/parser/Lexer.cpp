#include "Lexer.h"

#include <cctype>
#include <fstream>
#include <algorithm>

USE_MODULE(Arcb::Scan);

/**
 * @file Lexer.cpp
 * @brief Arcb DSL lexer implementation.
 *
 * The lexer converts the input script stream into a flat token stream.
 * It also keeps an in-memory copy of all source lines to support diagnostics.
 *
 * Design notes:
 * - Newline tokens are preserved (parser/grammar relies on them).
 * - Comments starting with '#' are skipped until newline.
 * - CR characters are ignored to normalize CRLF inputs.
 *
 * Known limitations:
 * - Strings are not lexed as a single token: '"' is tokenized separately.
 * - The '!' / '!=' style operators are not recognized (keywords "ne/eq/in" are used instead).
 * - The comment-skip logic consumes the newline inside the comment loop and then
 *   treats it as the current character (so NEWLINE token can still be produced).
 */




//     ██████╗██╗      █████╗ ███████╗███████╗    ██╗███╗   ███╗██████╗ ██╗     
//    ██╔════╝██║     ██╔══██╗██╔════╝██╔════╝    ██║████╗ ████║██╔══██╗██║     
//    ██║     ██║     ███████║███████╗███████╗    ██║██╔████╔██║██████╔╝██║     
//    ██║     ██║     ██╔══██║╚════██║╚════██║    ██║██║╚██╔╝██║██╔═══╝ ██║     
//    ╚██████╗███████╗██║  ██║███████║███████║    ██║██║ ╚═╝ ██║██║     ███████╗
//     ╚═════╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝    ╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝
//                                                                              


/**
 * @brief Construct a lexer bound to an Arcb script file path.
 *
 * The constructor opens the file, caches all lines for later diagnostics,
 * then primes the lexer by reading the first character.
 *
 * @param arcscript Path to the source script file.
 */
Lexer::Lexer(const std::string& arcscript)
    :
    current_('\0'),
    line_(1),
    col_(0),
    nlcol_(0),
    file_(arcscript),
    in_(file_),
    arcscript_(arcscript)
{
    // CACHE SOURCE LINES FOR DIAGNOSTICS
    load_file_lines(in_);

    // PRIME CURRENT_ WITH FIRST CHAR
    advance();
}



/**
 * @brief Produce the next token from the input stream.
 *
 * The lexer:
 * 1) skips whitespace (but preserves '\n')
 * 2) returns ENDOFFILE if stream is invalid
 * 3) lexes identifiers/keywords, numbers, or single-char tokens
 *
 * @return Next scanned Token.
 */
Token Lexer::next()
{
    // SKIP WHITESPACE BUT PRESERVE NEWLINE
    skipWhitespace();

    // IF STREAM IS INVALID RETURN EOF TOKEN
    if (!in_)
    {
        return makeToken(TokenType::ENDOFFILE, "", line_, col_, col_);
    }

    // IDENTIFIER / KEYWORD
    if (std::isalpha(static_cast<unsigned char>(current_)) || current_ == '_')
    {
        return identifier();
    }

    // NUMBER
    if (std::isdigit(static_cast<unsigned char>(current_)))
    {
        return number();
    }

    // SINGLE-CHAR TOKENS
    switch (current_)
    {
        case '=':  return simpleToken(TokenType::ASSIGN);
        case '"':  return simpleToken(TokenType::DQUOTE);
        case '+':  return simpleToken(TokenType::PLUS);
        case '-':  return simpleToken(TokenType::MINUS);
        case '*':  return simpleToken(TokenType::STAR);
        case '/':  return simpleToken(TokenType::SLASH);
        case '(':  return simpleToken(TokenType::ROUNDLP);
        case ')':  return simpleToken(TokenType::ROUNDRP);
        case '[':  return simpleToken(TokenType::SQUARELP);
        case ']':  return simpleToken(TokenType::SQUARERP);
        case '{':  return simpleToken(TokenType::CURLYLP);
        case '}':  return simpleToken(TokenType::CURLYRP);
        case '<':  return simpleToken(TokenType::ANGULARLP);
        case '>':  return simpleToken(TokenType::ANGULARRP);
        case '@':  return simpleToken(TokenType::AT);
        case ';':  return simpleToken(TokenType::SEMICOLON);
        case '\n': return simpleToken(TokenType::NEWLINE);
        default:   return simpleToken(TokenType::UNKNOWN);
    }
}



/**
 * @brief Load the entire input stream into a vector of lines.
 *
 * This is used for error reporting (line preview) and is independent from tokenization.
 *
 * @param in Input stream (must be seekable).
 */
void Lexer::load_file_lines(std::istream& in)
{
    // READ ALL LINES INTO MEMORY
    std::string line;

    while (std::getline(in, line))
    {
        lines.push_back(line);
    }

    // RESET STREAM FOR LEXING
    in.clear();
    in.seekg(0);
}



/**
 * @brief Advance the underlying stream by one logical character.
 *
 * This updates:
 * - current_  : current character
 * - line_     : 1-based line index
 * - col_      : column (0-based-ish, see below)
 * - nlcol_    : "running column" used to compute NEWLINE token position
 *
 * Behavior:
 * - EOF sets eofbit, current_ = '\0'
 * - '\r' is ignored (CRLF normalization)
 * - '#' starts a comment, which is skipped until '\n' or EOF
 */
void Lexer::advance()
{
    // LOOP UNTIL WE PRODUCE A STABLE current_
    for (;;)
    {
        // READ NEXT CHAR
        int c = in_.get();

        // HANDLE EOF
        if (c == EOF)
        {
            in_.setstate(std::ios::eofbit);
            current_ = '\0';
            return;
        }

        // NORMALIZE CRLF BY DROPPING '\r'
        if (c == '\r')
        {
            continue;
        }

        // SKIP COMMENTS STARTING WITH '#'
        if (c == '#')
        {
            // CONSUME UNTIL NEWLINE OR EOF
            do
            {
                c = in_.get();
                ++nlcol_;
                ++col_;
            }
            while (c != '\n' && c != EOF);
        }

        // COMMIT CURRENT CHAR
        current_ = static_cast<char>(c);

        // UPDATE POSITIONS
        if (current_ == '\n')
        {
            // NEW LINE: INCREMENT LINE AND RESET col_
            ++line_;
            nlcol_ = col_;
            col_   = 0;
        }
        else
        {
            // NORMAL CHAR: ADVANCE COLUMNS
            ++nlcol_;
            ++col_;
        }

        return;
    }
}



/**
 * @brief Skip whitespace excluding newline.
 *
 * This is intentionally conservative: NEWLINE is a real token.
 *
 * @note Uses ::isspace which depends on C locale; you cast to unsigned char,
 *       which is correct (avoids UB).
 */
void Lexer::skipWhitespace()
{
    // ADVANCE WHILE WHITESPACE BUT NOT NEWLINE
    while (in_ && current_ != '\n' && ::isspace(static_cast<unsigned char>(current_)))
    {
        advance();
    }
}



/**
 * @brief Emit a single-character token from current_.
 *
 * @param type Token type to assign.
 * @return Token with lexeme = the current character and proper source position.
 *
 * Critique:
 * - The NEWLINE special-case (line_-1 / nlcol_-1) is correct given current bookkeeping,
 *   but it is easy to break if you adjust advance() counters later.
 */
Token Lexer::simpleToken(TokenType type)
{
    // CAPTURE CURRENT CHAR BEFORE ADVANCING
    char c = current_;

    // COMPUTE TOKEN POSITION (SPECIAL-CASE NEWLINE)
    auto tokLine = (type == TokenType::NEWLINE) ? line_  - 1 : line_;
    auto tokCol  = (type == TokenType::NEWLINE) ? nlcol_ - 1 : col_ - 1;

    // MOVE FORWARD
    advance();

    // BUILD TOKEN (END PARAM IS USED AS "LENGTH" HERE)
    return makeToken(type, std::string(1, c), tokLine, tokCol, 1);
}



/**
 * @brief Lex an identifier and classify keywords.
 *
 * Grammar:
 * - starts with alpha or '_'
 * - continues with alnum or '_'
 *
 * Keywords are detected case-insensitively by lowercasing the scanned lexeme.
 *
 * @return IDENTIFIER or a keyword TokenType.
 */
Token Lexer::identifier()
{
    // CAPTURE START POSITION
    std::string lexeme;
    auto tokLine = line_;
    auto tokCol  = col_ - 1;

    TokenType tt = TokenType::IDENTIFIER;

    // CONSUME IDENT CHARS
    while (in_ && (std::isalnum(static_cast<unsigned char>(current_)) || current_ == '_'))
    {
        lexeme.push_back(current_);
        advance();
    }

    // LOWERCASE COPY FOR KEYWORD CHECK
    std::string lower = lexeme;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [] (unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // MATCH RESERVED KEYWORDS
    if (lower.compare("task") == 0)
    {
        tt = TokenType::TASK;
    }
    else if (lower.compare("import") == 0)
    {
        tt = TokenType::IMPORT;
    }
    else if (lower.compare("using") == 0)
    {
        tt = TokenType::USING;
    }
    else if (lower.compare("map") == 0)
    {
        tt = TokenType::MAPPING;
    }
    else if (lower.compare("assert") == 0)
    {
        tt = TokenType::ASSERT;
    }
    else if (lower.compare("ne") == 0)
    {
        tt = TokenType::NE;
    }
    else if (lower.compare("eq") == 0)
    {
        tt = TokenType::EQ;
    }
    else if (lower.compare("in") == 0)
    {
        tt = TokenType::IN;
    }

    // EMIT TOKEN (END PARAM USED AS LENGTH)
    return makeToken(tt, std::move(lexeme), tokLine, tokCol, lexeme.size());
}



/**
 * @brief Lex an unsigned integer number.
 *
 * Grammar:
 * - one or more digits
 *
 * @return NUMBER token.
 */
Token Lexer::number()
{
    // CAPTURE START POSITION
    std::string lexeme;
    auto tokLine = line_;
    auto tokCol  = col_ - 1;

    // CONSUME DIGITS
    while (in_ && std::isdigit(static_cast<unsigned char>(current_)))
    {
        lexeme.push_back(current_);
        advance();
    }

    // EMIT TOKEN
    return makeToken(TokenType::NUMBER, std::move(lexeme), tokLine, tokCol, lexeme.size());
}



/**
 * @brief Build a Token object.
 *
 * @param type   Token type.
 * @param lexeme Token lexeme payload.
 * @param line   1-based line index.
 * @param start  Start column (as tracked by lexer).
 * @param end    End column OR length.
 * @return Token value.
 */
Token Lexer::makeToken(TokenType type, std::string lexeme,
                       std::size_t line, std::size_t start, std::size_t end)
{
    // BUILD TOKEN STRUCT
    return Token{ type, std::move(lexeme), line, start, end };
}
