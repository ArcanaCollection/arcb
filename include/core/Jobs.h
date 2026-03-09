#ifndef __ARCB_JOBS_H__
#define __ARCB_JOBS_H__


/**
 * @defgroup Jobs Job Execution Model
 * @brief Job representation and execution planning.
 *
 * This module defines the job abstraction used by Arcb at runtime.
 *
 * It is responsible for:
 * - representing executable jobs derived from semantic tasks
 * - resolving execution order and dependencies
 * - producing an executable job list for the Core runtime
 *
 * This module sits between the Semantic layer and the Core runtime.
 */


/**
 * @addtogroup Jobs
 * @{
 */


#include "Defines.h"
#include "Semantic.h"

#include <variant>
#include <unordered_set>


BEGIN_MODULE(Jobs)
USE_MODULE(Arcb);




//    ███████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ██████╗ ███████╗
//    ██╔════╝██╔═══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔══██╗██╔════╝
//    █████╗  ██║   ██║██████╔╝██║ █╗ ██║███████║██████╔╝██║  ██║███████╗
//    ██╔══╝  ██║   ██║██╔══██╗██║███╗██║██╔══██║██╔══██╗██║  ██║╚════██║
//    ██║     ╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║██████╔╝███████║
//    ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝
//                                                                                                                                                                                       


class Job;
class List;





//    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗
//    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝
//    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗
//    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║
//    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║
//    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
//                                                                                                                                                  




/**
 * @brief Executable job description.
 *
 * A Job is the runtime representation of a semantic task after
 * variable expansion and dependency resolution.
 */
struct Job
{
    std::string            name;            ///< Job name.
    Semantic::Task::Instrs instructions;    ///< Instructions to execute.
    Semantic::Executor     engine;          ///< Executor used to run the job.
    bool                   parallelizable;  ///< Whether the job can run in parallel.
    bool                   expanded;        ///< Parallellizable job
    bool                   echo;            ///< Whether command echoing is enabled.
    bool                   death;
    Semantic::InstructionTask::Cache cache;

};








//     ██████╗██╗      █████╗ ███████╗███████╗███████╗███████╗
//    ██╔════╝██║     ██╔══██╗██╔════╝██╔════╝██╔════╝██╔════╝
//    ██║     ██║     ███████║███████╗███████╗█████╗  ███████╗
//    ██║     ██║     ██╔══██║╚════██║╚════██║██╔══╝  ╚════██║
//    ╚██████╗███████╗██║  ██║███████║███████║███████╗███████║
//     ╚═════╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚══════╝
//                                                            




/**
 * @brief Collection of executable jobs.
 *
 * Builds and stores the final execution plan derived from the
 * semantic environment.
 */
class List
{
public:
    List()            = default;
    List(const List&) = default;

    /**
     * @brief Builds a job list from a semantic environment.
     *
     * @param[in]  environment Semantic environment.
     * @param[out] out Output job list.
     *
    * @return ARCB_RESULT__OK on success, otherwise a failure code.
     */
    static Arcb_Result
    FromEnv(Semantic::Enviroment& environment, List& out, std::vector<std::string>& recovery) noexcept;

    /**
     * @brief Returns all jobs in execution order.
     */
    std::vector<Job>&
    All() noexcept
    {
        return data;
    }

    std::string main_job; ///< Name of the main job.

private:
    void Insert(const std::optional<Job>& j);
    void Insert(std::vector<Job>& vj);

    std::unordered_set<std::string> index; ///< Job name index for uniqueness.
    std::vector<Job>                data;  ///< Ordered job list.
};



END_MODULE(Jobs)


/** @} */


#endif /* __ARCB_JOBS_H__ */