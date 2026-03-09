#ifndef __ARCB_CORE_H__
#define __ARCB_CORE_H__



/**
 * @defgroup Core Core Runtime
 * @brief Core runtime facilities of Arcb.
 *
 * This module provides:
 * - execution of job graphs
 * - runtime configuration options
 * - global symbol handling
 *
 * It represents the execution layer between the semantic model
 * and the operating system.
 */

/**
 * @addtogroup Core
 * @{
 */


#include "Jobs.h"
#include "Defines.h"

#include <thread>


BEGIN_MODULE(Core)
USE_MODULE(Arcb);




//    ███████╗███╗   ██╗██╗   ██╗███╗   ███╗███████╗
//    ██╔════╝████╗  ██║██║   ██║████╗ ████║██╔════╝
//    █████╗  ██╔██╗ ██║██║   ██║██╔████╔██║███████╗
//    ██╔══╝  ██║╚██╗██║██║   ██║██║╚██╔╝██║╚════██║
//    ███████╗██║ ╚████║╚██████╔╝██║ ╚═╝ ██║███████║
//    ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝
//        



/**
 * @brief Built-in Arcb symbol identifiers.
 *
 * Used to represent special runtime values accessible
 * through the {arcb::...} expansion mechanism.
 */
enum class SymbolType : std::uint8_t
{
    MAIN        = 0, ///< Main task symbol.
    ROOT,            ///< Project root directory.
    PATH,           
    VERSION,         ///< Arcb version.
    RELEASE,         ///< Arcb release name.
    PROFILE,         ///< Active build profile.
    THREADS,         ///< Active thread count.
    MAX_THREADS,     ///< Maximum available threads.
    OS,              ///< Operating system identifier.
    ARCH,            ///< Architecture identifier.

    UNDEFINED,       ///< Unknown or invalid symbol.
};




//    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗
//    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝
//    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗
//    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║
//    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║
//    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
//                                                               


/**
 * @brief Result of a single instruction execution.
 */
struct InstructionResult
{
    std::string command;    ///< Executed command.
    int         exit_code;  ///< Process exit code.
};



/**
 * @brief Result of a job execution.
 */
struct Result
{
    std::string                    name;        ///< Job name.
    bool                           ok;          ///< Overall success status.
    int                            first_error; ///< Index of first failing instruction.
    std::vector<InstructionResult> results;     ///< Per-instruction results.
};



/**
 * @brief Runtime options controlling job execution.
 */
struct RunOptions
{
    bool     verbose         = false;                               ///< enebles standard output.
    bool     stop_on_error   = true;                                ///< Stop execution on first error.
    unsigned max_parallelism = std::thread::hardware_concurrency(); ///< Max concurrent jobs.
};






//    ██████╗ ██╗   ██╗██████╗     ███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗    ██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝
//    ██████╔╝██║   ██║██████╔╝    █████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗    ██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║╚════██║
//    ██║     ╚██████╔╝██████╔╝    ██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████║
//    ╚═╝      ╚═════╝ ╚═════╝     ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝
//                                                                                                           


/**
 * @brief Executes a list of jobs.
 *
 * @param[in] jobs Job execution list.
 * @param[in] opt Runtime execution options.
 *
 * @return ARCB_RESULT__OK on success, otherwise a failure code.
 */
Arcb_Result
run_jobs(Jobs::List& jobs, const RunOptions& opt) noexcept;



/**
 * @brief Returns the current value of a built-in symbol.
 *
 * @param[in] type Symbol identifier.
 * @return Reference to the symbol value string.
 */
std::vector<std::string>
symbol(Core::SymbolType type) noexcept;


std::string
first_symbol(Core::SymbolType type) noexcept;


/**
 * @brief Checks whether a string represents a built-in symbol.
 *
 * @param[in] symbol Symbol name.
 * @return Corresponding SymbolType or SymbolType::UNDEFINED.
 */
SymbolType
is_symbol(const std::string& symbol) noexcept;



/**
 * @brief Updates the value of a built-in symbol.
 *
 * @param[in] type Symbol identifier.
 * @param[in] val New symbol value.
 */
void
update_symbol(Core::SymbolType type, const std::string& val) noexcept;



/**
 * @brief Check if a sybol is set.
 * @param type Symbol type.
 * @return True if supported.
 */
bool
is_symbol_set(Core::SymbolType type) noexcept;



/**
 * @brief Checks whether a string matches a supported operating system identifier.
 */
bool
is_os(const std::string& param) noexcept;



/**
 * @brief Checks whether a string matches a supported architecture identifier.
 */
bool
is_arch(const std::string& param) noexcept;



END_MODULE(Core)

/** @} */

#endif /* __ARCB_CORE_H__ */