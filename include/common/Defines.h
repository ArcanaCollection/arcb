#ifndef __ARCANA_COMMON_DEFINES__H__
#define __ARCANA_COMMON_DEFINES__H__



//    ███╗   ██╗ █████╗ ███╗   ███╗███████╗███████╗██████╗  █████╗  ██████╗███████╗    ██╗   ██╗████████╗██╗██╗     ███████╗
//    ████╗  ██║██╔══██╗████╗ ████║██╔════╝██╔════╝██╔══██╗██╔══██╗██╔════╝██╔════╝    ██║   ██║╚══██╔══╝██║██║     ██╔════╝
//    ██╔██╗ ██║███████║██╔████╔██║█████╗  ███████╗██████╔╝███████║██║     █████╗      ██║   ██║   ██║   ██║██║     ███████╗
//    ██║╚██╗██║██╔══██║██║╚██╔╝██║██╔══╝  ╚════██║██╔═══╝ ██╔══██║██║     ██╔══╝      ██║   ██║   ██║   ██║██║     ╚════██║
//    ██║ ╚████║██║  ██║██║ ╚═╝ ██║███████╗███████║██║     ██║  ██║╚██████╗███████╗    ╚██████╔╝   ██║   ██║███████╗███████║
//    ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝╚══════╝     ╚═════╝    ╚═╝   ╚═╝╚══════╝╚══════╝
//                                                                                                                                                                                                                                                  


/**
 * @brief Begins a namespace block.
 *
 * Expands to a standard C++ namespace declaration.
 *
 * @param ns Namespace name.
 */
#define BEGIN_NAMESPACE(ns)         namespace ns {



/**
 * @brief Ends a namespace block.
 *
 * Closes a namespace previously opened with BEGIN_NAMESPACE().
 *
 * @param ns Namespace name (unused, for symmetry and readability only).
 */
#define END_NAMESPACE(ns)           }



/**
 * @brief Name of the main Arcana module namespace.
 */
#define MAIN_MODULE Arcana



/**
 * @brief Begins a sub-module namespace inside the main module.
 *
 * Expands to a nested namespace declaration of the form:
 * `namespace Arcana::mod {`
 *
 * @param mod Sub-module name.
 */
#define BEGIN_MODULE(mod)       BEGIN_NAMESPACE(MAIN_MODULE::mod)



/**
 * @brief Ends a sub-module namespace.
 *
 * Closes a namespace previously opened with BEGIN_MODULE().
 *
 * @param mod Module name (unused, for symmetry and readability only).
 */
#define END_MODULE(mod)         END_NAMESPACE(mod)



/**
 * @brief Imports a module namespace into the current scope.
 *
 * Expands to a using-directive for the specified namespace.
 *
 * @param mod Module namespace to import.
 */
#define USE_MODULE(mod)         using namespace mod







//    ██╗   ██╗████████╗██╗██╗     ██╗████████╗██╗   ██╗    ███╗   ███╗ █████╗  ██████╗██████╗  ██████╗ ███████╗
//    ██║   ██║╚══██╔══╝██║██║     ██║╚══██╔══╝╚██╗ ██╔╝    ████╗ ████║██╔══██╗██╔════╝██╔══██╗██╔═══██╗██╔════╝
//    ██║   ██║   ██║   ██║██║     ██║   ██║    ╚████╔╝     ██╔████╔██║███████║██║     ██████╔╝██║   ██║███████╗
//    ██║   ██║   ██║   ██║██║     ██║   ██║     ╚██╔╝      ██║╚██╔╝██║██╔══██║██║     ██╔══██╗██║   ██║╚════██║
//    ╚██████╔╝   ██║   ██║███████╗██║   ██║      ██║       ██║ ╚═╝ ██║██║  ██║╚██████╗██║  ██║╚██████╔╝███████║
//     ╚═════╝    ╚═╝   ╚═╝╚══════╝╚═╝   ╚═╝      ╚═╝       ╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝
//                                                                                                                                                                                                                          


/**
 * @brief Suppresses unused variable warnings.
 *
 * Explicitly marks a variable as intentionally unused.
 *
 * @param x Variable to mark as unused.
 */
#define UNUSED(x)                               ((void) x)




/**
 * @brief Casts an enum value to size_t.
 *
 * Intended for safe use when an enum value is used as an index
 * (e.g. array or vector access).
 *
 * @param enum Enum value to cast.
 */
#define _I(enum)                                ((size_t) enum) 



/**
 * @brief Current Arcana version string.
 */
#define __ARCANA__VERSION__                     "0.6.1"


/**
 * @brief Current Arcana major release name.
 */
#define __ARCANA__RELEASE__                     "Lushy Lion"




//     █████╗ ███╗   ██╗███████╗██╗    ███████╗███████╗ ██████╗ █████╗ ██████╗ ███████╗███████╗
//    ██╔══██╗████╗  ██║██╔════╝██║    ██╔════╝██╔════╝██╔════╝██╔══██╗██╔══██╗██╔════╝██╔════╝
//    ███████║██╔██╗ ██║███████╗██║    █████╗  ███████╗██║     ███████║██████╔╝█████╗  ███████╗
//    ██╔══██║██║╚██╗██║╚════██║██║    ██╔══╝  ╚════██║██║     ██╔══██║██╔═══╝ ██╔══╝  ╚════██║
//    ██║  ██║██║ ╚████║███████║██║    ███████╗███████║╚██████╗██║  ██║██║     ███████╗███████║
//    ╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚═╝    ╚══════╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝     ╚══════╝╚══════╝
//                                                                                                                                                                                       

#define ANSI_FG(r,g,b)                          "\033[38;2;" #r ";" #g ";" #b "m"                            
#define ANSI_RESET                              "\x1b[0m"

// ==== Foreground (text) ====
#define ANSI_BLACK                              "\x1b[30m"
#define ANSI_RED                                "\x1b[31m"
#define ANSI_GREEN                              "\x1b[32m"
#define ANSI_YELLOW                             "\x1b[33m"
#define ANSI_BLUE                               "\x1b[34m"
#define ANSI_MAGENTA                            "\x1b[35m"
#define ANSI_CYAN                               "\x1b[36m"
#define ANSI_WHITE                              "\x1b[37m"
#define ANSI_GRAY                               "\x1b[37;2m"

// ==== Foreground bright =====
#define ANSI_BBLACK                             "\x1b[90m"
#define ANSI_BRED                               "\x1b[91m"
#define ANSI_BGREEN                             "\x1b[92m"
#define ANSI_BYELLOW                            "\x1b[93m"
#define ANSI_BBLUE                              "\x1b[94m"
#define ANSI_BMAGENTA                           "\x1b[95m"
#define ANSI_BCYAN                              "\x1b[96m"
#define ANSI_BWHITE                             "\x1b[97m"

// ==== Background ====
#define ANSI_BG_BLACK                           "\x1b[40m"
#define ANSI_BG_RED                             "\x1b[41m"
#define ANSI_BG_GREEN                           "\x1b[42m"
#define ANSI_BG_YELLOW                          "\x1b[43m"
#define ANSI_BG_BLUE                            "\x1b[44m"
#define ANSI_BG_MAGENTA                         "\x1b[45m"
#define ANSI_BG_CYAN                            "\x1b[46m"
#define ANSI_BG_WHITE                           "\x1b[47m"

// ==== Background bright ====
#define ANSI_BG_BBLACK                          "\x1b[100m"
#define ANSI_BG_BRED                            "\x1b[101m"
#define ANSI_BG_BGREEN                          "\x1b[102m"
#define ANSI_BG_BYELLOW                         "\x1b[103m"
#define ANSI_BG_BBLUE                           "\x1b[104m"
#define ANSI_BG_BMAGENTA                        "\x1b[105m"
#define ANSI_BG_BCYAN                           "\x1b[106m"
#define ANSI_BG_BWHITE                          "\x1b[107m"

// ==== Styles ====
#define ANSI_BOLD                               "\x1b[1m"
#define ANSI_DIM                                "\x1b[2m"
#define ANSI_ITALIC                             "\x1b[3m"
#define ANSI_UNDERLINE                          "\x1b[4m"
#define ANSI_BLINK                              "\x1b[5m"
#define ANSI_REVERSE                            "\x1b[7m"
#define ANSI_HIDDEN                             "\x1b[8m"


// utils
/**
 * @brief Wraps a token with bright magenta ANSI color codes.
 *
 * Intended for stream output (e.g. std::cout / std::cerr).
 *
 * @param token Token or expression to be colorized.
 */
#define TOKEN_MAGENTA(token)                    ANSI_BMAGENTA << token << ANSI_RESET

/**
 * @brief Wraps a token with bright cyan ANSI color codes.
 *
 * Intended for stream output (e.g. std::cout / std::cerr).
 *
 * @param token Token or expression to be colorized.
 */
#define TOKEN_CYAN(token)                       ANSI_BCYAN << token << ANSI_RESET

/**
 * @brief Wraps a token with bright red ANSI color codes.
 *
 * Intended for stream output (e.g. std::cout / std::cerr).
 *
 * @param token Token or expression to be colorized.
 */
#define TOKEN_RED(token)                        ANSI_BRED << token << ANSI_RESET

/**
 * @brief Wraps a token with bright green ANSI color codes.
 *
 * Intended for stream output (e.g. std::cout / std::cerr).
 *
 * @param token Token or expression to be colorized.
 */
#define TOKEN_GREEN(token)                      ANSI_BGREEN << token << ANSI_RESET


/**
 * @brief Wraps a token with bright yellow ANSI color codes.
 *
 * Intended for stream output (e.g. std::cout / std::cerr).
 *
 * @param token Token or expression to be colorized.
 */
#define TOKEN_YELLOW(token)                      ANSI_BYELLOW << token << ANSI_RESET

#define TOKEN_ORANGE(token)                      ANSI_FG(217, 150, 38) << token << ANSI_RESET
#define TOKEN_LYELLOW(token)                     ANSI_FG(252, 240, 190) << token << ANSI_RESET



//    ██████╗ ██████╗ ██╗███╗   ██╗████████╗███████╗██████╗     ███╗   ███╗ █████╗  ██████╗██████╗  ██████╗ ███████╗
//    ██╔══██╗██╔══██╗██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗    ████╗ ████║██╔══██╗██╔════╝██╔══██╗██╔═══██╗██╔════╝
//    ██████╔╝██████╔╝██║██╔██╗ ██║   ██║   █████╗  ██████╔╝    ██╔████╔██║███████║██║     ██████╔╝██║   ██║███████╗
//    ██╔═══╝ ██╔══██╗██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗    ██║╚██╔╝██║██╔══██║██║     ██╔══██╗██║   ██║╚════██║
//    ██║     ██║  ██║██║██║ ╚████║   ██║   ███████╗██║  ██║    ██║ ╚═╝ ██║██║  ██║╚██████╗██║  ██║╚██████╔╝███████║
//    ╚═╝     ╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝    ╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝
//                                                                                                                  

#include <iostream>



/**
 * @brief Prints a formatted Arcana informational message to stdout.
 *
 * Prefixes the message with a colored "[ARCANA]" tag.
 *
 * @param msg Message to print.
 */
#define ARC(msg)                              std::cout << "[" << ANSI_BGREEN << ANSI_DIM << "ARCANA" << ANSI_RESET "] " << msg << std::endl



/**
 * @brief Prints a formatted debug message to stdout.
 *
 * Prefixes the message with a colored "[DEBUG]" tag and renders the message
 * in gray for reduced visual prominence.
 *
 * @param msg Debug message to print.
 */
#define DBG(msg)                              std::cout << "[" << ANSI_BYELLOW << ANSI_DIM << "DEBUG" << ANSI_RESET "] " << ANSI_GRAY << msg << ANSI_RESET << std::endl



/**
 * @brief Prints a raw message to stdout.
 *
 * No prefix, no formatting beyond a trailing newline.
 *
 * @param msg Message to print.
 */
#define MSG(msg)                              std::cout << msg << std::endl



/**
 * @brief Prints a formatted error message to stderr.
 *
 * Prefixes the message with a colored "[ERROR]" tag.
 *
 * @param msg Error message to print.
 */
#define ERR(msg)                              std::cerr << "[" << ANSI_BRED << "ERROR" << ANSI_RESET << "] " << msg << std::endl



/**
 * @brief Prints a formatted warning message to stdout.
 *
 * Prefixes the message with a colored "[WARN]" tag.
 *
 * @param msg Warning message to print.
 */
#define WARN(msg)                             std::cout << "[" << ANSI_BYELLOW << "WARN" << ANSI_RESET << "] " << msg << std::endl



/**
 * @brief Prints a formatted hint message to stdout.
 *
 * Prefixes the message with a colored "[HINT]" tag.
 *
 * @param msg Hint message to print.
 */
#define HINT(msg)                             std::cout << "[" << ANSI_BGREEN << "HINT" << ANSI_RESET << "]  " << msg << std::endl






//    ████████╗██╗   ██╗██████╗ ███████╗███████╗
//    ╚══██╔══╝╚██╗ ██╔╝██╔══██╗██╔════╝██╔════╝
//       ██║    ╚████╔╝ ██████╔╝█████╗  ███████╗
//       ██║     ╚██╔╝  ██╔═══╝ ██╔══╝  ╚════██║
//       ██║      ██║   ██║     ███████╗███████║
//       ╚═╝      ╚═╝   ╚═╝     ╚══════╝╚══════╝
//                                                                                                                                                        

#include <stdint.h>


/**
 * @brief Result codes returned by Arcana core operations.
 *
 * This enum represents the generic success or failure status
 * of Arcana execution steps.
 */
enum Arcana_Result : int32_t
{
    ARCANA_RESULT__OK          =  0,  ///< Operation completed successfully.
    ARCANA_RESULT__OK_AND_EXIT =  1,  ///< Operation completed successfully, then exit.
    ARCANA_RESULT__NOK         = -1,  ///< Operation failed.
};



/**
 * @brief Result codes returned by semantic analysis stages.
 *
 * This enum class represents the outcome of semantic or AST-related
 * operations.
 */
enum class Semantic_Result : int32_t
{
    AST_RESULT__OK  =  0,  ///< Semantic analysis completed successfully.
    AST_RESULT__NOK = -1,  ///< Semantic analysis failed.
};




#endif /* __ARCANA_COMMON_DEFINES__H__ */