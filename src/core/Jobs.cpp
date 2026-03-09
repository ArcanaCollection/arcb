#include "Jobs.h"
#include "Cache.h"
#include "TableHelper.h"

#include <string_view>
#include <unordered_map>

USE_MODULE(Arcb::Jobs);




//    ████████╗██╗   ██╗██████╗ ███████╗███████╗
//    ╚══██╔══╝╚██╗ ██╔╝██╔══██╗██╔════╝██╔════╝
//       ██║    ╚████╔╝ ██████╔╝█████╗  ███████╗
//       ██║     ╚██╔╝  ██╔═══╝ ██╔══╝  ╚════██║
//       ██║      ██║   ██║     ███████╗███████║
//       ╚═╝      ╚═╝   ╚═╝     ╚══════╝╚══════╝
//                                              

/**
 * @brief DFS mark used for cycle detection and topo ordering.
 */
enum class VisitMark : uint8_t
{
    NONE,
    TEMP,
    PERM,
};







/**
 * @brief Result container for topo traversal.
 */
struct TopoResult
{
    TopoResult() : ok(true) {}

    bool             ok;
    std::string      error_task;
    std::vector<Job> jobs;
};


using Graph = std::unordered_map<std::string, std::array<std::vector<std::string>, 2>>;






//    ██████╗ ██████╗ ██╗██╗   ██╗ █████╗ ████████╗███████╗███████╗
//    ██╔══██╗██╔══██╗██║██║   ██║██╔══██╗╚══██╔══╝██╔════╝██╔════╝
//    ██████╔╝██████╔╝██║██║   ██║███████║   ██║   █████╗  ███████╗
//    ██╔═══╝ ██╔══██╗██║╚██╗ ██╔╝██╔══██║   ██║   ██╔══╝  ╚════██║
//    ██║     ██║  ██║██║ ╚████╔╝ ██║  ██║   ██║   ███████╗███████║
//    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝  ╚═╝  ╚═╝   ╚═╝   ╚══════╝╚══════╝
//                                                                 


/**
 * @brief Remove instructions that are not affected by any changed input file.
 *
 * The pruning is driven by task inputs (variables). For each input variable:
 * - if it has `glob_expansion`, each expanded file is checked;
 * - otherwise `var_value` is treated as a single file path.
 *
 * For each file, any instruction that contains the file substring is considered "dependent".
 * If the cache reports the file as unchanged, the instruction is pruned.
 *
 * @param job   Job to prune in-place.
 * @param task  Source semantic task (provides inputs).
 * @param vtable Variable table used to resolve input variables.
 */
static void PruneUnchangedInstructions(Jobs::Job& job, const Semantic::InstructionTask& task) noexcept
{
    // FAST-EXIT ON EMPTY INPUTS OR EMPTY INSTRUCTIONS
    if (job.instructions.empty() || !task.cache.enabled)
    {
        return;
    }

    // TRACK WHICH INSTRUCTIONS MUST BE KEPT
    std::vector<bool> keep(job.instructions.size(), true);
    bool never_found = true; 
    bool any_changes = false;

    // MAP FILE -> INSTRUCTION DEPENDENCY AND PRUNE IF UNCHANGED
    auto process_file = [&] (const std::string& file)
    {       
        const bool changed = Cache::Manager::Instance().HasFileChanged(file);

        if (changed) any_changes = true;
        
        for (std::size_t i = 0; i < job.instructions.size(); ++i)
        {
            // SKIP ALREADY PRUNED INSTRUCTIONS
            if (!keep[i]) continue;

            const std::string& instr = job.instructions[i];

            // SIMPLE DEPENDENCY HEURISTIC: INSTRUCTION CONTAINS FILE
            if (instr.find(file) != std::string::npos)
            {
                never_found = false;

                // IF NOT CHANGED, PRUNE INSTRUCTION
                if (!changed)
                {
                    keep[i] = false;
                }
            }
        } 

    };

    if (task.cache.type == Semantic::InstructionTask::Cache::Type::TRACK)
    {
        for (auto& file : task.cache.data)
        {
            process_file(file);
        }
    }
    else if (task.cache.type == Semantic::InstructionTask::Cache::Type::UNTRACK)
    {
        Arcb::Cache::Manager::Instance().ClearCache(task.cache.data);
    }
    else
    {
        for (auto& file : task.cache.data)
        {
            process_file(file);
        }
    }
    

    if (never_found && !any_changes && task.cache.type != Semantic::InstructionTask::Cache::Type::UNTRACK)
    {
        job.instructions.clear();
    }
    else if (!never_found)
    {
        // REBUILD INSTRUCTION LIST WITH KEPT ITEMS ONLY
        Semantic::Task::Instrs filtered;
        filtered.reserve(job.instructions.size());
    
        for (std::size_t i = 0; i < job.instructions.size(); ++i)
        {
            if (keep[i])
            {
                filtered.emplace_back(std::move(job.instructions[i]));
            }
        }
    
        // SWAP FILTERED INSTRUCTIONS BACK
        job.instructions.swap(filtered);
    }
}



/**
 * @brief Build a runtime Job from a semantic InstructionTask.
 *
 * This function expands task instructions, prunes unchanged ones based on inputs,
 * and applies per-task execution flags (multithread, echo, flushcache).
 *
 * @param task Semantic task description.
 * @return Pair (ExpansionError, optional Job). If expansion fails, Job is nullopt.
 */
static std::optional<Job>
FromInstruction(const Semantic::InstructionTask& task, bool prunable = true) noexcept
{
    Job new_job {};

    // ENCODE MULTIPLE LINES INTO A SINGLE SCRIPT STRING
    auto make_single_instruction = [] (const Semantic::Task::Instrs& instrs) noexcept -> std::string
    {
        std::stringstream ss;
        const auto size = instrs.size();

        for (uint32_t i = 0; i < size; ++i)
        {
            ss << instrs[i];
            
            if ((i + 1) != size) ss << std::endl;
        } 

        return ss.str();
    }; 

    // IF NOTHING TO RUN, RETURN EMPTY JOB
    if (!task.task_instrs.size())
    {
        return std::nullopt;
    }

    // INIT JOB HEADER
    new_job.name         = task.task_name; 
    new_job.engine       = task.engine;
    new_job.cache        = task.cache;
    new_job.instructions = {};

    if (task.expanded)
    {
        new_job.instructions = std::move(task.task_instrs);
        new_job.expanded = true;
    }
    else 
    {
        new_job.instructions.push_back(make_single_instruction(task.task_instrs));
        new_job.expanded = false;
    }

    if (prunable)
    {
        PruneUnchangedInstructions(new_job, task);
    }

    // IF NOTHING TO RUN, RETURN EMPTY JOB
    if (!new_job.instructions.size())
    {
        return std::nullopt;
    }

    if (new_job.engine.type == Semantic::Executor::Type::RAW)
    {
        for (auto& instr : new_job.instructions)
        {
            Support::sanificate(instr, '"');
        }
    }

    // APPLY EXECUTION ATTRIBUTES
    new_job.parallelizable = task.hasAttribute(Semantic::Attr::Type::MULTITHREAD);
    new_job.echo           = task.hasAttribute(Semantic::Attr::Type::ECHO);
    new_job.death          = task.hasAttribute(Semantic::Attr::Type::DEATH);

    return new_job;
}



/**
 * @brief Build a graph representation of task dependencies and successors.
 *
 * The graph node stores two adjacency lists:
 * - index 0: dependencies (REQUIRES)
 * - index 1: successors   (THEN)
 *
 * @param table Task table.
 * @return Graph keyed by task name.
 */
static Graph BuildGraph(const Semantic::FTable& table)
{
    Graph g;

    // BUILD GRAPH NODES
    for (const auto& [name, task] : table)
    {
        // INIT BOTH SIDES
        g[name][0] = {};
        g[name][1] = {};

        // DEPENDENCIES: dep -> task
        if (task.hasAttribute(Semantic::Attr::Type::REQUIRES))
        {
            auto& deps = task.getProperties(Semantic::Attr::Type::REQUIRES);

            for (const auto& dep_name : deps)
            {
                g[name][0].push_back(dep_name);
            }
        }

        // SUCCESSORS: task -> succ
        if (task.hasAttribute(Semantic::Attr::Type::THEN))
        {
            auto& succs = task.getProperties(Semantic::Attr::Type::THEN);

            for (const auto& succ_name : succs)
            {
                g[name][1].push_back(succ_name);
            }
        }
    }

    return g;
}



/**
 * @brief DFS visit used to build an ordered job list starting from a root task.
 *
 * The visit:
 * - checks task existence,
 * - detects cycles via TEMP/PERM marks,
 * - visits dependencies first, then collects node, then visits successors.
 *
 * @param name Current task name.
 * @param table Task table.
 * @param vtable Variable table used by expansion.
 * @param graph Dependency/successor graph.
 * @param mark DFS marks.
 * @param out Output ordered jobs.
 * @param err Output expansion error.
 * @return True on success, false on error.
 */
static bool dfs_visit(const std::string&                name,
                      const Semantic::FTable&           table,
                      const Graph&                      graph,
                      std::map<std::string, VisitMark>& mark,
                      std::vector<Job>&                 out,
                      std::string& err,
                      bool prunable = true) noexcept
{
    // COLLECT CURRENT NODE INTO OUT VECTOR
    auto collect_node = [&] (Arcb::Semantic::FTable::const_iterator& tit, bool pruning) noexcept -> void
    {
        const Semantic::InstructionTask& t = tit->second;
        const auto& result = FromInstruction(t, pruning);

        if (result.has_value())
        {
            out.push_back(result.value());
        }

        return ;
    };

    std::stringstream ss;

    // CHECK TASK EXISTENCE
    auto tit = table.find(name);

    if (tit == table.end())
    {
        ss << "Unknown task '" << ANSI_BMAGENTA << name << ANSI_RESET << "'";
        err = ss.str();
        return false;
    }

    // FETCH MARK
    VisitMark& m = mark[name];

    // ALREADY VISITED
    if (m == VisitMark::PERM)
    {
        return true;
    }

    // CYCLE DETECTED
    if (m == VisitMark::TEMP)
    {
        ss << "Cyclic dependency involving task '" << ANSI_BMAGENTA << name << ANSI_RESET << "'";
        err = ss.str();
        return false;
    }

    // MARK AS IN-PROGRESS
    m = VisitMark::TEMP;

    // VISIT ADJACENCY LISTS
    auto git = graph.find(name);

    if (git != graph.end())
    {
        // VISIT DEPENDENCIES FIRST
        for (const auto& succ : git->second[0])
        {
            if (!dfs_visit(succ, table, graph, mark, out, err, prunable))
            {
                return false;
            }
        }

        // COLLECT CURRENT NODE
        collect_node(tit, prunable);

        // VISIT SUCCESSORS
        for (const auto& succ : git->second[1])
        {
            if (!dfs_visit(succ, table, graph, mark, out, err, prunable))
            {
                return false;
            }
        }
    }

    // MARK AS DONE
    m = VisitMark::PERM;

    // FINAL COLLECT (KEEP ORIGINAL BEHAVIOR)
    // collect_node(tit, prunable);
    return true;
}



// ============================================================================
// JOBS LIST API
// ============================================================================

/**
 * @brief Insert a job if present and not already in the list.
 * @param j Optional job.
 */
void List::Insert(const std::optional<Job>& j)
{
    if (j)
    {
        auto [it, inserted] = index.insert(j.value().name);

        if (inserted)
        {
            data.push_back(j.value());
        }
    }
}



/**
 * @brief Generate a job list from a semantic environment.
 *
 * The list is built by:
 * - building a graph from the task table,
 * - DFS visiting starting from the MAIN task (if present),
 * - inserting ALWAYS tasks afterwards.
 *
 * @param environment Semantic environment containing tables and expansions.
 * @param out Output job list.
 * @return ARCB_RESULT__OK on success, otherwise a failure code.
 */
Arcb_Result List::FromEnv(Semantic::Enviroment& environment, List& out, std::vector<std::string>& recovery) noexcept
{
    // BUILD GRAPH FROM FTABLE
    Graph graph = BuildGraph(environment.ftable);

    for (const auto& task_name : recovery)
    {
        std::string                      err;
        std::vector<Job>                 ordered;
        std::map<std::string, VisitMark> mark;

        // DFS VISIT ROOT
        if (!dfs_visit(task_name, environment.ftable, graph, mark, ordered, err, false))
        {
            ERR(err);
            return Arcb_Result::ARCB_RESULT__NOK;
        }

        // INSERT ORDERED JOBS
        for (const Job& j : ordered)
        {
            out.Insert(j);
        }
    }

    // START FROM MAIN TASK
    if (auto main_task_opt = Table::GetValue(environment.ftable, Semantic::Attr::Type::MAIN))
    {
        const auto&       main_task = main_task_opt.value();
        const std::string main_name = main_task.get().task_name;

        std::string                      err;
        std::vector<Job>                 ordered;
        std::map<std::string, VisitMark> mark;

        // DFS VISIT ROOT
        if (!dfs_visit(main_name, environment.ftable, graph, mark, ordered, err))
        {
            ERR(err);
            return Arcb_Result::ARCB_RESULT__NOK;
        }  

        // INSERT ORDERED JOBS
        for (const Job& j : ordered)
        {
            out.Insert(j);
        }

        out.main_job = main_name;
    }

    // COLLECT ALWAYS TASKS
    if (auto always_opt = Table::GetValues(environment.ftable, Semantic::Attr::Type::ALWAYS))
    {
        for (const auto& task : always_opt.value())
        {
            const auto& result = FromInstruction(task);

            if (result.has_value())
            {
                out.Insert(result.value());
            }
        }
    }

    return Arcb_Result::ARCB_RESULT__OK;
}
 