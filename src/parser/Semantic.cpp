#include "Semantic.h"

#include "Glob.h"
#include "Core.h"
#include "Support.h"
#include "Cache.h"
#include "TableHelper.h"
#include "Profiler.h"

#include <regex>
#include <memory>
#include <thread>
#include <charconv>
#include <algorithm>
#include <filesystem>
#include <unordered_map>

USE_MODULE(Arcana::Semantic);

namespace fs = std::filesystem;


using AttributeMap = Arcana::Support::AbstractKeywordMap<Attr::Type>;
using UsingMap     = Arcana::Support::AbstractKeywordMap<Using::Rule>;
using CacheMap     = Arcana::Support::AbstractKeywordMap<InstructionTask::Cache::Type>;
using EngineMap    = Arcana::Support::AbstractKeywordMap<Executor::Type>;



// ------------------------------
// STATIC TABLES
// ------------------------------


/**
 * @brief Map of attribute name -> normalized Attr::Type.
 */
static const AttributeMap Known_Attributes =
{
    { "profile"     , Attr::Type::PROFILE     },
    { "pub"         , Attr::Type::PUBLIC      },
    { "always"      , Attr::Type::ALWAYS      },
    { "requires"    , Attr::Type::REQUIRES    },
    { "then"        , Attr::Type::THEN        },
    { "map"         , Attr::Type::MAP         },
    { "threading"   , Attr::Type::MULTITHREAD },
    { "main"        , Attr::Type::MAIN        },
    { "engine"      , Attr::Type::ENGINE      },
    { "cache"       , Attr::Type::CACHE       },
    { "echo"        , Attr::Type::ECHO        },
    { "exclude"     , Attr::Type::EXCLUDE     },
    { "glob"        , Attr::Type::GLOB        },
    { "ifos"        , Attr::Type::IFOS        },
    { "death"       , Attr::Type::DEATH       },
};



/**
 * @brief Map of using keyword -> semantic using rule.
 */
static const UsingMap Known_Usings =
{
    { "profiles", { {               }, Using::Type::PROFILES } },
    { "engine"  , { { "file", "raw" }, Using::Type::ENGINE   } },
    { "threads" , { {               }, Using::Type::THREADS  } },
};



/**
 * @brief Canonical attribute names list for hinting/closest-match.
 */
static const std::vector<std::string> _attributes =
{
    "profile",
    "pub",
    "always",
    "requires",
    "then",
    "map",
    "threading",
    "main",
    "engine",
    "cache",
    "echo",
    "exclude",
    "glob",
    "ifos",
    "death",
};



/**
 * @brief Canonical using keywords list for hinting/closest-match.
 */
static const std::vector<std::string> _usings =
{
    "profiles",
    "engine",
    "threads",
};



/**
 * @brief Canonical cache instructions
 */
static const CacheMap _cache =
{
    { "track"   , InstructionTask::Cache::Type::TRACK   },
    { "store"   , InstructionTask::Cache::Type::STORE   },
    { "untrack" , InstructionTask::Cache::Type::UNTRACK }, 
};


/**
 * @brief 
 */
static const EngineMap _engines =
{
    { "raw"    , Executor::Type::RAW    },
    { "file"   , Executor::Type::FILE   },
};




// ------------------------------
// OUTPUT MACROS
// ------------------------------




//    ███████╗███╗   ██╗ ██████╗ ██╗███╗   ██╗███████╗
//    ██╔════╝████╗  ██║██╔════╝ ██║████╗  ██║██╔════╝
//    █████╗  ██╔██╗ ██║██║  ███╗██║██╔██╗ ██║█████╗  
//    ██╔══╝  ██║╚██╗██║██║   ██║██║██║╚██╗██║██╔══╝  
//    ███████╗██║ ╚████║╚██████╔╝██║██║ ╚████║███████╗
//    ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝╚═╝  ╚═══╝╚══════╝
//                                                    



/**
 * @brief Construct semantic engine and initialize attribute rule table.
 */
Engine::Engine()
{
    // INITIALIZE ATTRIBUTE RULES TABLE
    _attr_rules[_I(Attr::Type::PROFILE     )] = { Attr::Qualificator::REQUIRED_PROPERTY, Attr::Count::ONE      , { Attr::Target::TASK, Attr::Target::VARIABLE } };
    _attr_rules[_I(Attr::Type::PUBLIC      )] = { Attr::Qualificator::NO_PROPERY       , Attr::Count::ZERO     , { Attr::Target::TASK, Attr::Target::VARIABLE } };
    _attr_rules[_I(Attr::Type::ALWAYS      )] = { Attr::Qualificator::NO_PROPERY       , Attr::Count::ZERO     , { Attr::Target::TASK,                        } };
    _attr_rules[_I(Attr::Type::REQUIRES    )] = { Attr::Qualificator::REQUIRED_PROPERTY, Attr::Count::UNLIMITED, { Attr::Target::TASK,                        } };
    _attr_rules[_I(Attr::Type::THEN        )] = { Attr::Qualificator::REQUIRED_PROPERTY, Attr::Count::UNLIMITED, { Attr::Target::TASK,                        } };
    _attr_rules[_I(Attr::Type::MAP         )] = { Attr::Qualificator::REQUIRED_PROPERTY, Attr::Count::ONE      , {                     Attr::Target::VARIABLE } };
    _attr_rules[_I(Attr::Type::EXCLUDE     )] = { Attr::Qualificator::REQUIRED_PROPERTY, Attr::Count::ONE      , {                     Attr::Target::VARIABLE } };
    _attr_rules[_I(Attr::Type::GLOB        )] = { Attr::Qualificator::NO_PROPERY       , Attr::Count::ZERO     , {                     Attr::Target::VARIABLE } };
    _attr_rules[_I(Attr::Type::MULTITHREAD )] = { Attr::Qualificator::NO_PROPERY       , Attr::Count::ZERO     , { Attr::Target::TASK,                        } };
    _attr_rules[_I(Attr::Type::MAIN        )] = { Attr::Qualificator::NO_PROPERY       , Attr::Count::ZERO     , { Attr::Target::TASK,                        } };
    _attr_rules[_I(Attr::Type::ENGINE      )] = { Attr::Qualificator::REQUIRED_PROPERTY, Attr::Count::UNLIMITED, { Attr::Target::TASK,                        } };
    _attr_rules[_I(Attr::Type::CACHE       )] = { Attr::Qualificator::REQUIRED_PROPERTY, Attr::Count::UNLIMITED, { Attr::Target::TASK,                        } };
    _attr_rules[_I(Attr::Type::ECHO        )] = { Attr::Qualificator::NO_PROPERY       , Attr::Count::ZERO     , { Attr::Target::TASK,                        } };
    _attr_rules[_I(Attr::Type::IFOS        )] = { Attr::Qualificator::REQUIRED_PROPERTY, Attr::Count::ONE      , {                     Attr::Target::VARIABLE } };
    _attr_rules[_I(Attr::Type::DEATH       )] = { Attr::Qualificator::NO_PROPERY       , Attr::Count::ZERO     , { Attr::Target::TASK,                        } };
}



/**
 * @brief Collect an attribute statement and stage it for the next entity.
 * @param name Raw attribute name.
 * @param prop Raw properties string (tokenized via split()).
 * @return SemanticOutput with status and optional hint.
 */
Arcana_Result Engine::Collect_Attribute(const std::string& name, const std::string& prop)
{
    std::stringstream ss;

    Attr::Type       attr     = Attr::Type::ATTRIBUTE__UNKNOWN;
    Attr::Properties property = Arcana::Support::split(prop);

    // RESOLVE ATTRIBUTE NAME TO TYPE
    if (auto it = Known_Attributes.find(name); it != Known_Attributes.end())
    {
        attr = it->second;
    }

    // HANDLE UNKNOWN ATTRIBUTE
    if (attr == Attr::Type::ATTRIBUTE__UNKNOWN)
    {
        return Raise_Error_With_Hint(
            _context.str(),
            "Attribute " << TOKEN_MAGENTA(name) << " not recognized",
            Support::FindClosest(_attributes, name)
        );
    }

    // VALIDATE PROPERTIES AGAINST RULES
    const auto& rule       = _attr_rules[_I(attr)];
    auto        props_count = property.size();

    if (rule == Attr::Qualificator::REQUIRED_PROPERTY)
    {
        // ENFORCE REQUIRED PROPERTIES
        if (props_count == 0)
        {
            return Raise_Error(
                _context.str(),
                "Attribute " << TOKEN_MAGENTA(name) << " requires at least one option"
            );
        }
        else if (props_count != 1 && rule.count == Attr::Count::ONE)
        {
            return Raise_Error(
                _context.str(),
                "Attribute " << TOKEN_MAGENTA(name) << " requires one option, not " << props_count
            );
        }
    }
    else
    {
        // ENFORCE NO PROPERTIES
        if (props_count > 0)
        {
            return Raise_Error(
                _context.str(),
                "Attribute " << TOKEN_MAGENTA(name) << " requires no option"
            );
        }
    }

    // ATTRIBUTE QUALIFICATION
    switch (attr)
    {
        case Attr::Type::PROFILE:
        {
            // VALIDATE PROFILE EXISTS
            const auto& profiles = _env.profile.profiles;

            if (std::find(profiles.begin(), profiles.end(), property[0]) == profiles.end())
            {
                return Raise_Error_With_Hint(
                    _context.str(),
                    "Profile " << TOKEN_MAGENTA(property[0]) << " must be declared via " << TOKEN_MAGENTA("using profile <profilenames>") << " statement",
                    Support::FindClosest(profiles, property[0])
                );
            }
        } break;

        case Attr::Type::MAP:
        case Attr::Type::EXCLUDE:
        {
            // VALIDATE REFERENCED VARIABLE EXISTS
            if (_env.vtable.find(property[0]) == _env.vtable.end())
            {
                return Raise_Error_With_Hint(
                    _context.str(),
                    "Invalid " << TOKEN_MAGENTA(property[0]) << ": undeclared variable",
                    Support::FindClosest(Table::Keys(_env.vtable), property[0])
                );
            }
        } break;

        case Attr::Type::IFOS:
        {
            // VALIDATE OS NAME
            if (!Core::is_os(property[0]))
            {
                return Raise_Error(
                    _context.str(),
                    "Invalid OS " << TOKEN_MAGENTA(property[0])
                );
            }
        } break;

        case Attr::Type::CACHE:
        case Attr::Type::ENGINE:
        {
            if (property.size() < 2)
            {
                return Raise_Error(
                    _context.str(),
                    "Missing arguments for attribute " << TOKEN_MAGENTA(name)
                );
            }
        } break;

        default: break;
    }

    // ENQUEUE ATTRIBUTE FOR NEXT ENTITY
    Attr::Attribute new_attr = {
        name,
        attr,
        property
    };

    new_attr.context = _context;
    _attr_pending.push_back(new_attr);

    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Collect a variable assignment into the environment VTable.
 * @param name Variable name.
 * @param val  Raw value.
 * @return SemanticOutput with status.
 */
Arcana_Result Engine::Collect_Assignment(const std::string& name, const std::string& val, bool join)
{
    std::stringstream  ss;
    InstructionAssign  assign { name, nullptr };

    if (!val.empty()) assign.var_value.push_back(val);

    // ATTACH PENDING ATTRIBUTES AND CLEAR PENDING QUEUE
    assign.context    = _context;
    assign.attributes = _attr_pending;
    _attr_pending.clear();

    // VALIDATE ATTRIBUTES TARGET VARIABLES
    for (const auto& attr : assign.attributes)
    {
        const auto& rule = _attr_rules[_I(attr.type)];

        if (std::find(rule.targets.begin(), rule.targets.end(), Attr::Target::VARIABLE) == rule.targets.end())
        {
            return Raise_Error(
                _context.str(),
                "Attribute " << TOKEN_MAGENTA(attr.name) << " is not valid for variables"
            );
        }
    }

    // APPLY MANGLING FOR PROFILE/IFOS
    const auto& attr_profile = std::find(assign.attributes.begin(), assign.attributes.end(), Attr::Type::PROFILE);
    const auto& attr_if      = std::find(assign.attributes.begin(), assign.attributes.end(), Attr::Type::IFOS);

    if (attr_profile != assign.attributes.end())
    {
        _env.vtable[Support::generate_mangling(name, (*attr_profile).props[0])] = assign;
    }
    else if (attr_if != assign.attributes.end())
    {
        _env.vtable[Support::generate_mangling(name, (*attr_if).props[0])] = assign;
    }
    else
    {
        if (join) 
        {
            auto& var = _env.vtable[name];
            var.var_value.push_back(val);
        }
        else
        {
            _env.vtable[name] = assign;
        }
    }

    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Collect a task declaration into the environment FTable.
 * @param name   Task name.
 * @param inputs Raw inputs string (split into tokens).
 * @param instrs Instruction lines.
 * @return SemanticOutput with status.
 */
Arcana_Result Engine::Collect_Task(const std::string& name, const Task::Instrs& instrs)
{
    std::stringstream ss;

    // BUILD TASK INSTRUCTION
    InstructionTask task { name, instrs };
    FTable&         ftable = _env.ftable;
    
    // ATTACH PENDING ATTRIBUTES AND CLEAR PENDING QUEUE
    task.context    = _context;
    task.attributes = _attr_pending;
    _attr_pending.clear();

    // VALIDATE ATTRIBUTES TARGET TASKS
    for (const auto& attr : task.attributes)
    {
        const auto& rule = _attr_rules[_I(attr.type)];

        if (std::find(rule.targets.begin(), rule.targets.end(), Attr::Target::TASK) == rule.targets.end())
        {
            return Raise_Error(
                _context.str(),
                "Attribute " << TOKEN_MAGENTA(attr.name) << " is not valid for tasks"
            );
        }
    }

    // APPLY MANGLING FOR PROFILE
    if (task.hasAttribute(Attr::Type::PROFILE))
    {
        const auto profile = task.getProperties(Attr::Type::PROFILE);

        ftable[Support::generate_mangling(name, profile[0])] = task;

        if (task.hasAttribute(Attr::Type::MAIN))
        {
            if (!Core::is_symbol_set(Core::SymbolType::MAIN))
            {
                Core::update_symbol(Core::SymbolType::MAIN, name);
            }
            else
            {
                if (Core::first_symbol(Core::SymbolType::MAIN) != name)
                {
                    return Raise_Error(
                        _context.str(),
                        "Cannot tag multiple tasks with attribute " << TOKEN_MAGENTA("main")
                    );
                }
            }
        }
    }
    else
    {
        ftable[name] = task;

        if (task.hasAttribute(Attr::Type::MAIN))
        {
            if (!Core::is_symbol_set(Core::SymbolType::MAIN))
            {
                Core::update_symbol(Core::SymbolType::MAIN, name);
            }
            else
            {
                return Raise_Error(
                    _context.str(),
                    "Cannot tag multiple tasks with attribute " << TOKEN_MAGENTA("main")
                );
            }
        }
    }

    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Collect a `using` directive and update environment configuration.
 * @param what Directive keyword (e.g. profiles/raw/file/threads).
 * @param opt  Directive option string.
 * @return SemanticOutput with status and optional hint.
 */
Arcana_Result Engine::Collect_Using(const std::string& what, const std::string& opt)
{
    auto IsExtension = [&] (const std::string& s) -> bool
    {
        static const std::regex re(R"(^\.[A-Za-z0-9]+(\.[A-Za-z0-9]+)*$)");
        return s.empty() || std::regex_match(s, re);
    };


    std::stringstream ss;
    Attr::Properties  options = Arcana::Support::split(opt);
    Using::Rule       rule;

    UsingMap::const_iterator it;

    // RESOLVE USING RULE
    if (it = Known_Usings.find(what); it != Known_Usings.end())
    {
        rule = it->second;
    }
    else
    {
        return Raise_Error_With_Hint(
            _context.str(),
            "Unknown " << TOKEN_MAGENTA(what),
            Support::FindClosest(_usings, what)
        );
    }

    // HANDLE DEFAULT ENGINE
    if (rule.using_type == Using::Type::ENGINE)
    {
        std::size_t index = 0;

        if (options.size() == 0)
        {
            return Raise_Error(
                _context.str(),
                "Missing arguments"
            );
        }

        const auto& attr = std::find(rule.valid_attr.begin(), rule.valid_attr.end(), options[0]);

        if (attr == rule.valid_attr.end())
        {
            return Raise_Error_With_Hint(
                _context.str(),
                "Unknown attribute " << TOKEN_MAGENTA(options[0]),
                Support::FindClosest(rule.valid_attr, options[0])
            );
        }

        auto type = _engines.find(options[0])->second;

        _env.default_interpreter.is_default = true;

        switch (type)
        {
            case Executor::Type::FILE:
            {
                if (options.size() < 3)
                {
                    return Raise_Error(
                        _context.str(),
                        "Missing arguments"
                    );
                }

                _env.default_interpreter.ext     = options[1];
                _env.default_interpreter.command = options[2]; 

                index = 3;
            }
            break;

            case Executor::Type::RAW:
            {
                if (options.size() < 2)
                {
                    return Raise_Error(
                        _context.str(),
                        "Missing arguments"
                    );
                }

                _env.default_interpreter.command = options[1]; 

                index = 2;
            }
            break;
        }

        if (!IsExtension(_env.default_interpreter.ext))
        {
            return Raise_Error(
                _context.str(),
                "Extension " << TOKEN_MAGENTA(_env.default_interpreter.ext) << " is missing or wrong"
            );
        }

        if (!Support::Is_In_Path(_env.default_interpreter.command) && !Support::file_exists(_env.default_interpreter.command))
        {
            return Raise_Error(
                _context.str(),
                "Binary " << TOKEN_MAGENTA(_env.default_interpreter.command) << " is missing or unknown"
            );
        }

        _env.default_interpreter.type = type;
        _env.default_interpreter.flags.insert(_env.default_interpreter.flags.begin(), options.begin() + index, options.end());
    }
    else if (rule.using_type == Using::Type::PROFILES)
    {
        // VALIDATE PROFILES PRESENT
        if (options.size() == 0)
        {
            return Raise_Error(
                _context.str(),
                "This statement must be followed by profiles name"
            );
        }

        // COLLECT UNIQUE PROFILES
        for (uint32_t iter = 0; iter < options.size(); ++iter)
        {
            if (Core::is_os(options[iter]) || Core::is_arch(options[iter]))
            {
                return Raise_Error(
                    _context.str(),
                    "Profile " << TOKEN_MAGENTA(options[iter]) << " cannot be the OS or ARCH name"
                );
            }
            else if (std::find(_env.profile.profiles.begin(), _env.profile.profiles.end(), options[iter]) == _env.profile.profiles.end())
            {
                _env.profile.profiles.push_back(options[iter]);
            }
            else
            {
                return Raise_Error(
                    _context.str(),
                    "Duplicated item: " << TOKEN_MAGENTA(options[iter])
                );
            }
        }
    }
    else if (rule.using_type == Using::Type::THREADS)
    {
        // VALIDATE THREADS ARGUMENT
        if (options.size() != 1)
        {
            return Raise_Error(
                _context.str(),
                "This statement must be followed maximum threads allowed"
            );
        }

        // PARSE INT VALUE
        int         max_threads = 0;
        const char* begin       = options[0].data();
        const char* end         = options[0].data() + options[0].size();

        auto [ptr, ec] = std::from_chars(begin, end, max_threads);

        if (ec != std::errc{} || ptr != end || max_threads <= 0)
        {
            return Raise_Error(
                _context.str(),
                "Invalid value for multithread: " << TOKEN_MAGENTA(options[0]) << ". Expected a positive integer."
            );
        }

        // STORE THREADS CONFIG
        _env.max_threads = max_threads;
        Core::update_symbol(Core::SymbolType::THREADS, std::to_string(max_threads));
    }

    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Collect a mapping statement (item_1 -> item_2) and annotate item_2 with @map(item_1).
 * @param item_1 Source variable name.
 * @param item_2 Destination variable name.
 * @return SemanticOutput with status/hint.
 */
Arcana_Result Engine::Collect_Mapping(const std::string& item_1, const std::string& item_2)
{
    std::stringstream ss;

    // VALIDATE VARIABLES PRESENCE
    auto&      vtable   = _env.vtable;
    auto       it_item1 = vtable.find(item_1);
    auto       it_item2 = vtable.find(item_2);

    if (it_item1 == vtable.end())
    {
        return Raise_Error_With_Hint(
            _context.str(),
            "Undeclared variable " << TOKEN_MAGENTA(item_1),
            Support::FindClosest(Table::Keys(vtable), item_1)
        );
    }

    if (it_item2 == vtable.end())
    {
        return Raise_Error_With_Hint(
            _context.str(),
            "Undeclared variable " << TOKEN_MAGENTA(item_2),
            Support::FindClosest(Table::Keys(vtable), item_2)
        );
    }

    // ATTACH @map ATTRIBUTE TO DESTINATION VARIABLE
    Attr::Attribute attr = {
        "map",
        Attr::Type::MAP,
        { item_1 }
    };

    attr.context = _context;

    it_item2->second.attributes.push_back(attr);

    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Collect an assert statement into the environment.
 * @param line   Line number.
 * @param stmt   Raw statement string.
 * @param lvalue Left side.
 * @param op     Operator token (eq/ne/in).
 * @param rvalue Right side.
 * @param reason Reason string.
 * @return SemanticOutput with status.
 */
Arcana_Result Engine::Collect_Assert(std::size_t line,
                                     const std::string& stmt,
                                     const std::string& lvalue,
                                     const std::string& op,
                                     const std::string& rvalue,
                                     const std::string& reason,
                                     const bool         actions)
{
    AssertCheck acheck;

    // FILL ASSERT_MSG STRUCT
    acheck.line   = line;
    acheck.stmt   = stmt;
    acheck.lvalue = lvalue;
    acheck.raw_rvalue = rvalue;
    
    if (actions)
    {
        acheck.type    = AssertCheck::Type::ACTIONS;
        acheck.actions = Arcana::Support::split(reason);
    }
    else
    {
        acheck.type   = AssertCheck::Type::MESSAGE;
        acheck.reason = reason;
    }
    
    // MAP OPERATOR
    if (op == "eq")
    {
        acheck.check = AssertCheck::CheckType::EQUAL;
    }
    else if (op == "ne")
    {
        acheck.check = AssertCheck::CheckType::NOT_EQUAL;
    }
    else if (op == "in")
    {
        acheck.check = AssertCheck::CheckType::IN;
    }

    // STORE ASSERT_MSG
    acheck.context = _context;
    _env.atable.push_back(acheck);

    return Arcana_Result::ARCANA_RESULT__OK;
}



// ------------------------------
// ENVIRONMENT
// ------------------------------

/**
 * @brief Validate CLI arguments against the collected environment and apply overrides.
 * @param args Parsed CLI arguments.
 * @return ARCANA_RESULT__OK on success, ARCANA_RESULT__NOK on error.
 */
Arcana_Result Enviroment::CheckArgs(const Arcana::Support::Arguments& args) noexcept
{
    // HANDLE PROFILE OVERRIDE
    if (args.profile.found)
    {
        if (std::find(profile.profiles.begin(), profile.profiles.end(), args.profile.value) == profile.profiles.end())
        {
            return Raise_Error_With_Hint(
                NO_CTX,
                "Requested profile " << TOKEN_MAGENTA(args.profile.value) << " is invalid!",
                Support::FindClosest(profile.profiles, args.profile.value)
            );
        }

        profile.selected = args.profile.value;
    }
    else
    {
        // DEFAULT PROFILE SELECTION
        if (profile.profiles.empty())
        {
            return Arcana_Result::ARCANA_RESULT__OK;
        }

        profile.selected = profile.profiles[0];
    }
    
    // UPDATE PROFILE SYMBOL
    Core::update_symbol(Core::SymbolType::PROFILE, profile.selected);

    // ALIGN TABLES AFTER PROFILE SELECTION
    Table::AlignOnProfile(vtable, profile.selected);
    Table::AlignOnProfile(ftable, profile.selected);
    Table::AlignOnOS(vtable);

    // HANDLE THREADS OVERRIDE
    if (args.threads)
    {
        max_threads = args.threads.ivalue;
        Core::update_symbol(Core::SymbolType::THREADS, args.threads.svalue);
    }

    // HANDLE TASK OVERRIDE
    if (args.task.found)
    {
        // RESOLVE TASK (PROFILE-AWARE)
        auto task = Table::GetValue(ftable, args.task.value, profile.profiles);

        if (!task)
        {
            return Raise_Error_With_Hint(
                NO_CTX,
                "Unknown task " << TOKEN_MAGENTA(args.task.value),
                Support::FindClosest(Table::Keys(ftable), args.task.value)
            );
        }
        else if (!task.value().get().hasAttribute(Semantic::Attr::Type::PUBLIC))
        {
            // REQUIRE PUBLIC WHEN REQUESTED VIA CLI
            return Raise_Error(
                NO_CTX,
                "Requested task " << TOKEN_MAGENTA(args.task.value) << " does not have " << TOKEN_CYAN("public") << " attribute"
            );
        }

        // TOGGLE MAIN TO REQUESTED TASK
        auto old_main_task = Table::GetValue(ftable, Attr::Type::MAIN);

        if (old_main_task)
        {
            old_main_task.value().get().removeAttribute(Attr::Type::MAIN);
        }

        task.value().get().attributes.push_back({
            "main",
            Attr::Type::MAIN,
            {}
        });

        Core::update_symbol(Core::SymbolType::MAIN, task.value().get().task_name);
    }
    else
    {
        // REQUIRE EXPLICIT MAIN IF NO CLI TASK
        auto main_task = Table::GetValues(ftable, profile.profiles, Attr::Type::MAIN);

        if (!main_task.size())
        {
            return Raise_Error(
                NO_CTX,
                "No main task specified, make it explicit in the " << ANSI_FG(217, 150, 38) << "arcfile" << ANSI_RESET << 
                " with the @" << TOKEN_CYAN("main") << " attribute or pass any task via command line"
            );
        }
    }

    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Resolve dependencies/then links and finalize engine defaults.
 * @return Empty optional on success, error string on failure.
 */
Arcana_Result Enviroment::AlignEnviroment() noexcept
{
    std::stringstream ss;

    // RESOLVE REQUIRES/THEN LINKS
    const std::array<Attr::Type, 2> attributes = { Attr::Type::REQUIRES, Attr::Type::THEN };

    for (const auto attr : attributes)
    {
        auto tasks = Table::GetValues(ftable, profile.profiles, attr);

        for (auto& ref : tasks)
        {
            auto& task  = ref.get();
            auto  props = task.getProperties(attr);

            // VALIDATE AND LINK REFERENCED TASKS
            for (auto& p : props)
            {
                auto it = ftable.find(p);

                if (it == ftable.end())
                {
                    return Raise_Error_With_Hint(
                        task.getAttrContext(attr),
                        "Invalid dependency " << TOKEN_MAGENTA(p) << " for task " << TOKEN_CYAN(task.task_name),
                        Support::FindClosest(Table::Keys(ftable), p)
                    );
                }

                // APPEND LINK IN ORDER
                if (attr == Attr::Type::REQUIRES)
                {
                    task.dependencies.push_back(std::cref(ftable.at(p)));
                }
                else
                {
                    task.thens.push_back(std::cref(ftable.at(p)));
                }
            }
        }
    }

    // SET DEFAULT ENGINE IF MISSING
    if (default_interpreter.is_default == false)
    {
#if defined(_WIN32)
        default_interpreter.command = "C:\\Windows\\System32\\cmd.exe";
        default_interpreter.flags   = "/d /s /c"
        default_interpreter.ext     = ".bat";
#else
        default_interpreter.command = "/bin/bash";
        default_interpreter.flags.clear();
        default_interpreter.ext     = ".sh";
#endif

        default_interpreter.is_default = true;
    }

    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Expand variables/internals, compute glob expansions, expand tasks and asserts.
 * @return Empty optional on success, error string on failure.
 */
Arcana_Result Enviroment::Expand() noexcept
{
    auto IsExtension = [&] (const std::string& s) -> bool
    {
        static const std::regex re(R"(^\.[A-Za-z0-9]+(\.[A-Za-z0-9]+)*$)");
        return s.empty() || std::regex_match(s, re);
    };

    Expander ex(*this);

    // COMPUTE MAX THREADS DEFAULT
    auto machine_max_threads = std::thread::hardware_concurrency();

    if (max_threads == 0 || max_threads > machine_max_threads)
    {
        max_threads = machine_max_threads;
    }
    
    // EXPAND VTABLE AND COMPUTE GLOB EXPANSIONS
    Glob::ExpandOptions opt;

    for (auto& [name, var] : vtable)
    {
        // EXPAND GLOB TO LIST
        var.glob_expansion.clear();

        for (auto& value : var.var_value)
        {
            // EXPAND TEXT TOKENS
            if (auto err = ex.ExpandText(value, {}); err != Arcana_Result::ARCANA_RESULT__OK)
            {
                return Raise_Error(var.context.str(), ex.Get_Error());
            }

            if (!var.hasAttribute(Attr::Type::GLOB)) continue;
    
            // PARSE GLOB PATTERN
            Glob::Pattern    pattern;
            Glob::ParseError error;

            if (!Glob::Parse(value, pattern, error))
            {
                return Raise_Error(
                    var.context.str(),
                    "While expanding " << TOKEN_MAGENTA(name) << " an invalid glob was detected " 
                                       << TOKEN_MAGENTA(pattern.normalized) << ": " << ParseErrorRepr(error)
                );
            }

            Arcana::Glob::Expand(pattern, ".", var.glob_expansion, opt);
        }
    }

    
    // HANDLE MAPPED VARS EXPANSION
    auto map_required = Table::GetValues(vtable, Semantic::Attr::Type::MAP);

    if (map_required.has_value())
    {
        for (auto& stmt : map_required.value())
        {
            Glob::ParseError e1, e2;
            Glob::MapError   m1;
    
            auto& map_to   = stmt.get();
            auto& map_from = vtable[map_to.getProperties(Semantic::Attr::Type::MAP).at(0)];
    
            if (!Arcana::Glob::MapGlobToGlob(map_from.var_value, map_to.var_value[0],
                                                map_from.glob_expansion, map_to.glob_expansion, e1, e2, m1))
            {
                return Raise_Error(
                    map_to.getAttrContext(Semantic::Attr::Type::MAP),
                    "While mapping " << TOKEN_MAGENTA(map_from.var_name) << " to " << TOKEN_MAGENTA(map_to.var_name) << ": incompatible globs"
                );
            }
        }
    }


    // EXPAND ASSERTS
    for (auto& assert : atable)
    {
        for (const auto& act : assert.actions)
        {
            if (ftable.find(act) == ftable.end())
            {
                return Raise_Error_With_Hint(
                    assert.context.str(),
                    "Callback " << TOKEN_MAGENTA(act) << " is undefined in statement " << TOKEN_CYAN("assert"),
                    Support::FindClosest(Table::Keys(ftable), act)
                );
            }
        }

        if (auto err = ex.ExpandAssertSide(assert.lvalue, assert); err != Arcana_Result::ARCANA_RESULT__OK)
        {
            return Raise_Error(assert.context.str(), ex.Get_Error());
        }

        if (auto err = ex.ExpandAssertSide(assert.raw_rvalue, assert, true); err != Arcana_Result::ARCANA_RESULT__OK)
        {
            return Raise_Error(assert.context.str(), ex.Get_Error());
        }

        if (auto err = ex.ExpandText(assert.reason, {}); err != Arcana_Result::ARCANA_RESULT__OK)
        {
            return Raise_Error(assert.context.str(), ex.Get_Error());
        }
    }

    // EXPAND FTABLE
    for (auto& [name, task] : ftable)
    {
        if (task.cache.enabled = task.hasAttribute(Attr::Type::CACHE); task.cache.enabled)
        {
            auto properties = task.getProperties(Attr::Type::CACHE);

            if (auto it = _cache.find(properties[0]); it != _cache.end())
            {
                task.cache.type = it->second;
            }
            else
            {
                return Raise_Error_With_Hint(
                    task.getAttrContext(Attr::Type::CACHE),
                    "Invalid cache algorithm " << TOKEN_MAGENTA(properties[0]),
                    Support::FindClosest(Table::Keys(_cache), properties[0])
                );
            }

            for (uint32_t i = 1; i < properties.size(); ++i)
            {
                std::size_t old_size = task.cache.data.size();

                if (auto err = ex.ExpandText(properties[i], {Expander::Algorithm::LIST}, &task.cache.data); err != Arcana_Result::ARCANA_RESULT__OK)
                {
                    return Raise_Error(task.context.str(), ex.Get_Error());
                }

                for (std::size_t j = old_size; j < task.cache.data.size(); ++j)
                {
                    const auto& file = task.cache.data[j];

                    if (!Support::file_exists(file))
                    {
                        return Raise_Error(
                            task.getAttrContext(Attr::Type::CACHE),
                            "Cannot " << properties[0] << " " << TOKEN_MAGENTA(file) << ": file not exists!"
                        );
                    }
                }
            }
        }

        if (task.hasAttribute(Attr::Type::ENGINE))
        {
            std::size_t index = 0;

            task.engine.is_default = false;
            const auto& properties = task.getProperties(Attr::Type::ENGINE);

            if (auto it = _engines.find(properties[0]); it != _engines.end())
            {
                task.engine.type = it->second;
            }
            else
            {
                return Raise_Error_With_Hint(
                    task.getAttrContext(Attr::Type::ENGINE),
                    "Invalid engine algorithm " << TOKEN_MAGENTA(properties[0]),
                    Support::FindClosest(Table::Keys(_engines), properties[0])
                );
            }

            if (task.engine.type == Executor::FILE)
            {
                if (properties.size() < 3)
                {
                    return Raise_Error(
                        task.getAttrContext(Attr::Type::ENGINE),
                        "Missing arguments for attribute " << TOKEN_MAGENTA("engine")
                    );
                }

                task.engine.ext     = properties[1];
                task.engine.command = properties[2];
                index               = 3;
            }
            else
            {
                task.engine.command = properties[1];
                index               = 2;
            }

            if (auto err = ex.ExpandText(task.engine.ext, {}); err != Arcana_Result::ARCANA_RESULT__OK)
            {
                return Raise_Error(task.context.str(), ex.Get_Error());
            }

            if (!IsExtension(task.engine.ext))
            {
                return Raise_Error(
                    task.getAttrContext(Attr::Type::ENGINE),
                    "File extension is missing or wrong"
                );
            }

            if (auto err = ex.ExpandText(task.engine.command, {}); err != Arcana_Result::ARCANA_RESULT__OK)
            {
                return Raise_Error(task.context.str(), ex.Get_Error());
            }

            if (!Support::Is_In_Path(task.engine.command) && !Support::file_exists(task.engine.command))
            {
                return Raise_Error(
                    task.getAttrContext(Attr::Type::ENGINE),
                    "Binary " << TOKEN_MAGENTA(task.engine.command) << " is missing or unknown"
                );
            }

            task.engine.flags.insert(task.engine.flags.begin(), properties.begin() + index, properties.end());

            for (uint32_t i = 0; i < task.engine.flags.size(); ++i)
            {
                if (auto err = ex.ExpandText(task.engine.flags[i], {Expander::Algorithm::INLINE}); err != Arcana_Result::ARCANA_RESULT__OK)
                {
                    return Raise_Error(task.context.str(), ex.Get_Error());
                }
            }
        }
        else
        {
            task.engine = default_interpreter;
        }


        std::vector<std::string> expanded_instrs;
        task.expanded = false;

        bool can_expand = task.hasAttribute(Attr::Type::MULTITHREAD);

        // EXPAND INSTRUCTION LINES
        for (auto& instr : task.task_instrs)
        {
            bool used_algo[4] = { false };

            if (auto err = ex.ExpandText(instr, {
                Expander::Algorithm::INLINE, 
                Expander::Algorithm::LIST,
                Expander::Algorithm::SIZE,
                Expander::Algorithm::EMPTY,
            }, &expanded_instrs, used_algo); err != Arcana_Result::ARCANA_RESULT__OK)
            {
                return Raise_Error(task.context.str(), ex.Get_Error());
            }
            
            if (used_algo[1] && can_expand)
            {
                task.expanded = true;
            }
        }

        task.task_instrs = expanded_instrs;
    }
    
    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Evaluate all collected asserts after expansion.
 * @return Empty optional on success, error string on first failure.
 */
Arcana_Result Enviroment::ExecuteAsserts(std::vector<std::string>& reco_cb) noexcept
{
    bool assert_failed = false;
    std::stringstream ss;

    reco_cb.clear(); 

    for (const auto& assert : atable)
    {
        assert_failed = false;

        // EVALUATE ASSERT_MSG
        switch (assert.check)
        {
            case AssertCheck::CheckType::EQUAL:
                assert_failed = std::find(assert.rvalue.begin(), assert.rvalue.end(), assert.lvalue) == assert.rvalue.end();
                break;
            case AssertCheck::CheckType::NOT_EQUAL:
                assert_failed = std::find(assert.rvalue.begin(), assert.rvalue.end(), assert.lvalue) != assert.rvalue.end();
                break;
            case AssertCheck::CheckType::IN:
                assert_failed = !std::any_of(assert.rvalue.begin(), assert.rvalue.end(), [&] (const std::string& s)
                {
                    return s.find(assert.lvalue) != std::string::npos;
                });
                break;
            case AssertCheck::CheckType::DEPENDENCIES:
                assert_failed = !std::any_of(assert.search_paths.begin(), assert.search_paths.end(), [] (const auto& p) 
                { 
                    return fs::exists(p); 
                });
                break;
        }

        if (assert_failed)
        {
            // BUILD ERROR MESSAGE
            ss << "Assert failed!";

            if (assert.check != AssertCheck::CheckType::DEPENDENCIES)
            {
                std::stringstream ss0;
                for (uint32_t i = 0; i < assert.rvalue.size(); ++i)
                {
                    ss0 << TOKEN_MAGENTA(assert.rvalue[i]);

                    if ((i + 1) != assert.rvalue.size())
                    {
                        ss0 << ", ";
                    }
                }

                ss << "\n        lvalue: " << TOKEN_MAGENTA(assert.lvalue) << "\n        rvalue: " << ss0.str() << std::endl;
            }
            else
            {
                ss << "\n        Dependency " << TOKEN_MAGENTA(assert.lvalue) << " not found!" << std::endl;
            }

            if (assert.type == AssertCheck::Type::MESSAGE)
            {
                ss << "Reason: " << assert.reason;
            }
            else
            {
                for (auto& task : assert.actions)
                {
                    ss << "[" << "\x1b[93m" << "WARN" << "\x1b[0m" << "] " << "Scheduling recovery callback " << TOKEN_MAGENTA(task) << std::endl;
                    reco_cb.push_back(task);
                } 
                ss << std::endl;
            }

            return Raise_Error(assert.context.str(), ss.str());
        }
    }

    return Arcana_Result::ARCANA_RESULT__OK;
}




//    ███████╗██╗  ██╗██████╗  █████╗ ███╗   ██╗██████╗ ███████╗██████╗ 
//    ██╔════╝╚██╗██╔╝██╔══██╗██╔══██╗████╗  ██║██╔══██╗██╔════╝██╔══██╗
//    █████╗   ╚███╔╝ ██████╔╝███████║██╔██╗ ██║██║  ██║█████╗  ██████╔╝
//    ██╔══╝   ██╔██╗ ██╔═══╝ ██╔══██║██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗
//    ███████╗██╔╝ ██╗██║     ██║  ██║██║ ╚████║██████╔╝███████╗██║  ██║
//    ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝
//                                                                      


/**
 * @brief Expand internal symbols `{arc:__...__}`.
 * @param s String to expand in-place.
 * @return Empty optional on success, error string on failure.
 */
Arcana_Result Enviroment::Expander::ExpandInternals(std::string& s) noexcept
{
    for (int depth = 0; depth < 256; ++depth)
    {
        // SEARCH NEXT INTERNAL TOKEN
        std::smatch m;
        if (!std::regex_search(s, m, re_intern))
        {
            return Arcana_Result::ARCANA_RESULT__OK;
        }

        const std::string sym = m[1].str();

        // RESOLVE SYMBOL AND REPLACE
        if (auto st = Core::is_symbol(sym); st != Core::SymbolType::UNDEFINED)
        {
            const auto rep = Core::symbol(st);

            if (st == Core::SymbolType::PATH)
            {
                std::string s_out;
                for (uint32_t i = 0; i < rep.size(); ++i)
                {
                    std::string s1 = s;
                    s_out += s1.replace(static_cast<std::size_t>(m.position(0)), static_cast<std::size_t>(m.length(0)), rep[i]);
                    if ((i+1) != rep.size()) s_out += "\n";
                }

                s = s_out;
            }
            else
            {
                s.replace(static_cast<std::size_t>(m.position(0)), static_cast<std::size_t>(m.length(0)), Support::join(rep));
            }
        }
        else
        {
            return Raise_Expansion_Error("Internal symbol expansion failed for " << TOKEN_MAGENTA("arc::" << sym));
        }
    }

    return Raise_Expansion_Error("Too deep internal symbol expansion (depth limit reached)");
}



/**
 * @brief Expand variable references `{arc:NAME}` using env.vtable.
 * @param s String to expand in-place.
 * @return Empty optional on success, error string on failure.
 */
Arcana_Result Enviroment::Expander::ExpandArcAll(std::string& s, const std::vector<Algorithm>& allowed_algorithms,
                                                 std::vector<std::string>* list_exp, bool* used_algo) noexcept
{
    struct ListExpansionItem
    {
        Algorithm   algo;
        std::vector<std::string>* buffer;
        size_t      lower_index;
        size_t      higher_index;
    };

    List expanded;
    expanded.source  = s;

    std::vector<ListExpansionItem> list_expansions;

    std::sregex_iterator rx_it1(expanded.source.begin(), expanded.source.end(), re_arc_mode);
    std::sregex_iterator end;

    bool list_expanded = false;

    auto expand = [&] (std::string& src, uint32_t pos = 0) noexcept -> Arcana_Result
    {
        ssize_t span = 0;
        for (const auto& match : expanded.matches)
        {
            std::string content{};
            
            switch (match.algo)
            {
                case Algorithm::NORMAL:     content = match.datasource->GetListValue(1, 0);                                           break;
                case Algorithm::LIST:       content = match.buffer->at(pos);                                                          break;
                case Algorithm::INLINE:     content = match.datasource->GetList(match.higher_index, match.lower_index, match.buffer); break;
                case Algorithm::SIZE:       content = std::to_string(match.buffer->size());                                           break;
                case Algorithm::EMPTY:      content = match.datasource->empty() ? "1" : "0";                                          break;
                case Algorithm::FILESYSTEM: content = match.buffer->at(pos);                                                          break;
                default: break;
            }   

            src.replace(match.start + span, match.count, content);
    
            span += content.length() - match.pattern_len;
        }

        return Arcana_Result::ARCANA_RESULT__OK;
    };

    auto validate_expansion_args = [&] (const std::string& src, 
                                        std::vector<std::string>* data, 
                                        const std::string& args, 
                                        size_t& l, 
                                        size_t& r) noexcept -> Arcana_Result
    {
        const auto&              values = Support::split(args);
        const auto               size   = values.size();
        std::optional<long long> conversion;

        if (size > 2)
        {
            return Raise_Expansion_Error("Invalid argument count " << TOKEN_MAGENTA(values.size()) << " in statement:\n" << TOKEN_LYELLOW(src));
        }

        l = r = 0;

        switch (size)
        {
            case 0:
                r = data->size();
                break;

            case 1: 
                l = 0; 
                if (conversion = Support::to_number(values[0]); !conversion.has_value())
                {
                    return Raise_Expansion_Error("Invalid argument " << TOKEN_MAGENTA(values[0]) << " in statement:\n" << TOKEN_LYELLOW(src) << " must be a positive number");
                }
                r = conversion.value();

                if (r > data->size())
                {
                    return Raise_Expansion_Error("Invalid argument " << TOKEN_MAGENTA(r) << " in statement:\n" << TOKEN_LYELLOW(src) << ", OOL value! (max is " << data->size() << ")");
                }

                break;
            
            case 2: 
                if (conversion = Support::to_number(values[0]); !conversion.has_value())
                {
                    return Raise_Expansion_Error("Invalid argument " << TOKEN_MAGENTA(values[0]) << " in statement:\n" << TOKEN_LYELLOW(src) << " must be a positive number");
                }
                l = conversion.value();

                if (conversion = Support::to_number(values[1]); !conversion.has_value())
                {
                    return Raise_Expansion_Error("Invalid argument " << TOKEN_MAGENTA(values[1]) << " in statement:\n" << TOKEN_LYELLOW(src) << " must be a positive number");
                }
                r = conversion.value();

                if (l >= data->size())
                {
                    return Raise_Expansion_Error("Invalid argument " << TOKEN_MAGENTA(l) << " in statement:\n" << TOKEN_LYELLOW(src) << ": OOL value! (max is " << data->size() << ")");
                }

                if (r > data->size())
                {
                    return Raise_Expansion_Error("Invalid argument " << TOKEN_MAGENTA(r) << " in statement:\n" << TOKEN_LYELLOW(src) << ": OOL value! (max is " << data->size() << ")");
                }

                if (l >= r)
                {
                    return Raise_Expansion_Error("Invalid argument " << TOKEN_MAGENTA(l) << " in statement:\n" << TOKEN_LYELLOW(src) << ": lvalue is greather then rvalue");
                }

                break;

            default:
                return Raise_Expansion_Error(
                    "Invalid argument count " << TOKEN_MAGENTA(values.size())
                    << " in statement:\n" << TOKEN_LYELLOW(src)
                );
        }

        return Arcana_Result::ARCANA_RESULT__OK;
        
    };


    auto allowed_algos_str_list = [&] (const std::vector<Algorithm>& allowed_algorithms) -> std::vector<std::string>
    {
        std::vector<std::string> data;

        for (const auto& algo : allowed_algorithms)
        {
            data.push_back(Expander::AlgorithmRepr(algo));
        }

        return data;
    };


    for (; rx_it1 != end; ++rx_it1)
    {
        const std::smatch& m         = *rx_it1;
        const std::string  name      = m[1].str();
        const std::string  algorithm = m[2].str();
        const std::string  args      = m[3].str();

        // LOOKUP VARIABLE VALUE
        auto it = env.vtable.find(name);
        if (it == env.vtable.end())
        {
            return Raise_Expansion_Error_With_Hint(
                "Undefined variable " << TOKEN_MAGENTA(name) << " in statement:\n" << TOKEN_LYELLOW(s),
                Support::FindClosest(Table::Keys(env.vtable), name)
            );
        }

        Algorithm algo;

        if (auto eit = Expansion_Map.find(algorithm); eit != Expansion_Map.end())
        {
            algo = eit->second;
        }
        else
        {
            const auto& algos = allowed_algos_str_list(allowed_algorithms);

            if (algos.empty())
            {
                return Raise_Expansion_Error(
                    "Undefined algorithm " << TOKEN_MAGENTA(algorithm) << " in statement:\n" << TOKEN_LYELLOW(s)
                );
            }
            else
            {
                return Raise_Expansion_Error_With_Hint(
                    "Undefined algorithm " << TOKEN_MAGENTA(algorithm) << " in statement:\n" << TOKEN_LYELLOW(s),
                    Support::FindClosest(allowed_algos_str_list(allowed_algorithms), algorithm)
                );
            }
        }

        if (auto found_algo = std::find(allowed_algorithms.begin(), allowed_algorithms.end(), algo); found_algo != allowed_algorithms.end())
        {
            if (used_algo != nullptr)
            {
                used_algo[found_algo - allowed_algorithms.begin()] = true;
            }
            
            size_t l = 0, r = 0;

            auto* buffer = &it->second.glob_expansion;
            if (buffer->size() == 0)
            {
                buffer = &it->second.var_value;
            }

            if (algo == Algorithm::LIST || algo == Algorithm::INLINE)
            {
                if (algo == Algorithm::LIST) 
                {
                    list_expanded = true;
                }

                if (buffer->empty())
                {
                    return Raise_Expansion_Error("Invalid algorithm " << TOKEN_MAGENTA(algorithm) << " for empty variable " << TOKEN_CYAN(name) << " in statement:\n" << TOKEN_LYELLOW(s));
                }

                if (auto err = validate_expansion_args(s, buffer, args, l, r); err != Arcana_Result::ARCANA_RESULT__OK)
                {
                    return err;
                }
                
                list_expansions.push_back(ListExpansionItem {algo, buffer, l, r});
            }

            expanded.matches.push_back( List::Match {algo, (size_t) m.position(0), (size_t) m.length(0), m[0].str().length(), &it->second, buffer, l, r} );
        }
        else
        {
            const auto& algos = allowed_algos_str_list(allowed_algorithms);

            if (algos.empty())
            {
                return Raise_Expansion_Error(
                    "Invalid algorithm " << TOKEN_MAGENTA(algorithm) << " in statement:\n" << TOKEN_LYELLOW(s)
                );
            }
            else
            {
                return Raise_Expansion_Error_With_Hint(
                    "Invalid algorithm " << TOKEN_MAGENTA(algorithm) << " in statement:\n" << TOKEN_LYELLOW(s),
                    Support::FindClosest(allowed_algos_str_list(allowed_algorithms), algorithm)
                );
            }
        }
    }

    std::sregex_iterator rx_it2(expanded.source.begin(), expanded.source.end(), re_arc);

    for (; rx_it2 != end; ++rx_it2)
    {
        const std::smatch& m     = *rx_it2;
        std::size_t        start = m.position(0);
        std::size_t        len   = m.length(0);
        const std::string  name  = m[1].str();

        // LOOKUP VARIABLE VALUE
        auto it = env.vtable.find(name);
        if (it == env.vtable.end())
        {
            return Raise_Expansion_Error_With_Hint(
                "Undefined variable " << TOKEN_MAGENTA(name) << " in statement:\n" << TOKEN_LYELLOW(s),
                Support::FindClosest(Table::Keys(env.vtable), name)
            );
        }

        expanded.matches.push_back( List::Match {Algorithm::NORMAL, start, len, m[0].str().length(), &it->second, NULL, 0, 0});
    }

    std::sort(expanded.matches.begin(), expanded.matches.end(), [] (const List::Match& a, const List::Match& b)
    {
        return a.start < b.start;
    });

    if (list_expansions.size() > 0 && list_expanded)
    {
        if (list_exp == nullptr)
        {
            return Raise_Expansion_Error("Invalid algorithm in statement:\n" << TOKEN_MAGENTA(s));
        }

        std::size_t low_index  = list_expansions[0].lower_index;
        std::size_t high_index = list_expansions[0].higher_index;

        for (const auto& item : list_expansions)
        {
            if (item.algo == Algorithm::LIST)
            {
                low_index  = (item.lower_index  < low_index)  ? item.lower_index  : low_index;
                high_index = (item.higher_index > high_index) ? item.higher_index : high_index;
            }
        }

        const bool max_index_valid = std::all_of(list_expansions.begin(), list_expansions.end(), [&] (const ListExpansionItem item)
        {
            if (item.algo != Algorithm::LIST) return true;
            return high_index <= item.buffer->size();
        });

        const bool low_index_valid = std::all_of(list_expansions.begin(), list_expansions.end(), [&] (const ListExpansionItem item)
        {
            if (item.algo != Algorithm::LIST) return true;
            return low_index <= item.buffer->size();
        });

        if (!low_index_valid || !max_index_valid)
        {
            return Raise_Expansion_Error("Invalid indexes " << TOKEN_MAGENTA(low_index) << 
                                         " and " << TOKEN_MAGENTA(high_index) << " in statement:\n" << TOKEN_LYELLOW(s));
        }

        for (uint32_t i = low_index; i < high_index && max_index_valid; ++i)
        {
            std::string src = s;
            if (auto err = expand(src, i); err != Arcana_Result::ARCANA_RESULT__OK)
            {
                return err;
            }
            list_exp->push_back(src);
        }
    } 
    else
    {
        if (auto err = expand(s); err != Arcana_Result::ARCANA_RESULT__OK)
        {
            return err;
        }
    
        if (list_exp != nullptr)
        {
            list_exp->push_back(s);
        }
    }

    return Arcana_Result::ARCANA_RESULT__OK;
}

 

/**
 * @brief Expand a text using internals + variable expansion.
 * @param s String to expand in-place.
 * @return Empty optional on success, error string on failure.
 */
Arcana_Result Enviroment::Expander::ExpandText(std::string& s, const std::vector<Algorithm>& allowed_algorithms,
                                               std::vector<std::string>* list_exp, bool* used_algo) noexcept
{
    // EXPAND INTERNALS
    if (auto err = ExpandInternals(s); err != Arcana_Result::ARCANA_RESULT__OK)
    {
        return err;
    }

    // CHECK IF __path__ SYMBOLS IS USED IN ASSERT STATEMENT.
    // THE ASSERT STATEMENT IS THE ONLY WHO USE THE ALGORITHM
    // FILESYSTEM, SO IF ITS CONTAINED INTO allowed_algorithms GET THE VALUES
    // AS ARRAY OF PATHS
    for (auto it = allowed_algorithms.begin(); it < allowed_algorithms.end(); ++it)
    {
        if ((*it) == Algorithm::FILESYSTEM && list_exp != nullptr && !list_exp->empty())
        {
            used_algo[it - allowed_algorithms.begin()] = true;
            break;
        }
    }

    // EXPAND VARIABLES
    if (auto err = ExpandArcAll(s, allowed_algorithms, list_exp, used_algo);  err != Arcana_Result::ARCANA_RESULT__OK)
    {
        return err;
    }

    return Arcana_Result::ARCANA_RESULT__OK;
}



/**
 * @brief Expand one assert side and update dependency-mode if `{fs:...}` is present.
 * @param stmt Assert side string (lvalue/rvalue), expanded in-place.
 * @param assert Assert object to update.
 * @return Empty optional on success, error string on failure.
 */
Arcana_Result Enviroment::Expander::ExpandAssertSide(std::string& stmt, AssertCheck& assert, bool rvalue) noexcept
{
    std::vector<std::string> paths;
    bool                     used_algo[2] = { false };

    // EXPAND TEXT TOKENS
    if (auto err = ExpandText(stmt, {Algorithm::FILESYSTEM, Algorithm::LIST}, &paths, used_algo); err != Arcana_Result::ARCANA_RESULT__OK)
    {
        return err;
    }

    if (used_algo[0] && used_algo[1])
    {
        return Raise_Expansion_Error("Cannot use " << TOKEN_MAGENTA("fs") << " and " << TOKEN_MAGENTA("list") << " together in statement:\n" << TOKEN_LYELLOW(stmt));
    }

    if (used_algo[0])
    {
        assert.check = AssertCheck::CheckType::DEPENDENCIES;

        for (auto& path : paths)
        {
            assert.search_paths.push_back(fs::path(path) / assert.lvalue);
        }
    }
    else if (rvalue)
    {
        const auto& refs = Support::split(stmt, '\n');

        if (refs.size() > 1)
        {
            assert.check = AssertCheck::CheckType::DEPENDENCIES;
            for (auto& path : refs)
            {
                assert.search_paths.push_back(fs::path(path) / assert.lvalue);
            }
        }
        else
        {
            assert.rvalue.insert(assert.rvalue.end(), paths.begin(), paths.end());
        }

    }

    return Arcana_Result::ARCANA_RESULT__OK;
}
