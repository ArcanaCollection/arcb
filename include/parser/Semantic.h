#ifndef __ARCB_SEMANTIC__H__
#define __ARCB_SEMANTIC__H__


#include <array>
#include <regex>
#include <vector>
#include <optional>
#include <algorithm>
#include <filesystem>

#include "Support.h"
#include "Defines.h"




BEGIN_MODULE(Cache)

class Manager;

END_MODULE(Cache)




BEGIN_MODULE(Semantic)



/**
 * @file Semantic.h
 * @brief Semantic data model and collector for Arcb DSL.
 *
 * This header defines the *semantic layer* used after lexing/parsing:
 * - attribute model (`Attr::Attribute`) and validation rules (`Semantic::Rule`)
 * - task / variable instruction containers (`InstructionTask`, `InstructionAssign`)
 * - assertion model (`AssertCheck`)
 * - environment container (`Enviroment`) holding all collected artifacts
 * - semantic engine (`Semantic::Engine`) responsible for collecting and building the environment
 *
 * The semantic layer is meant to be fed by the parser and then post-processed
 * (alignment, expansion, asserts execution) before producing runnable jobs.
 *
 * @note The code intentionally keeps storage as simple STL containers (maps/vectors)
 *       to preserve deterministic behavior and predictable iteration order.
 */


struct Context
{
    std::string source_file;
    std::string input;
    std::size_t lineno;

    std::string str() const
    {
        std::stringstream ss;
        ss << "In file " << TOKEN_ORANGE(ANSI_BOLD << source_file) 
           << ":"        << TOKEN_ORANGE(ANSI_BOLD << lineno)  
           << " near the statement:\n" 
           << TOKEN_LYELLOW(input << std::endl);
        return ss.str();
    }
};



//    ███████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ██████╗ ███████╗
//    ██╔════╝██╔═══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔══██╗██╔════╝
//    █████╗  ██║   ██║██████╔╝██║ █╗ ██║███████║██████╔╝██║  ██║███████╗
//    ██╔══╝  ██║   ██║██╔══██╗██║███╗██║██╔══██║██╔══██╗██║  ██║╚════██║
//    ██║     ╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║██████╔╝███████║
//    ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝
//                                                                                                                                                                                                    


struct Rule;







//    ███╗   ██╗ █████╗ ███╗   ███╗███████╗███████╗██████╗  █████╗  ██████╗███████╗     █████╗ ████████╗████████╗██████╗ 
//    ████╗  ██║██╔══██╗████╗ ████║██╔════╝██╔════╝██╔══██╗██╔══██╗██╔════╝██╔════╝    ██╔══██╗╚══██╔══╝╚══██╔══╝██╔══██╗
//    ██╔██╗ ██║███████║██╔████╔██║█████╗  ███████╗██████╔╝███████║██║     █████╗      ███████║   ██║      ██║   ██████╔╝
//    ██║╚██╗██║██╔══██║██║╚██╔╝██║██╔══╝  ╚════██║██╔═══╝ ██╔══██║██║     ██╔══╝      ██╔══██║   ██║      ██║   ██╔══██╗
//    ██║ ╚████║██║  ██║██║ ╚═╝ ██║███████╗███████║██║     ██║  ██║╚██████╗███████╗    ██║  ██║   ██║      ██║   ██║  ██║
//    ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝╚══════╝    ╚═╝  ╚═╝   ╚═╝      ╚═╝   ╚═╝  ╚═╝
//                                                                                                                       




BEGIN_NAMESPACE(Attr)



/**
 * @brief Attribute kinds supported by Arcb DSL.
 *
 * These values represent recognized annotations for variables/tasks.
 * The parser collects raw attributes, then semantic logic validates them
 * using `Attr::Rules` (a per-Type rule table).
 *
 * @note `ATTRIBUTE__COUNT` is used for fixed-size arrays that map rules to types.
 */
enum class Type
{
    PROFILE          = 0,   //!< Profile-scoped entity (mangled with @@<profile>)
    PUBLIC              ,   //!< Expose task as public entrypoint
    ALWAYS              ,   //!< Force task/job execution ignoring cache heuristics
    REQUIRES            ,   //!< Task dependencies (must run before current task)
    THEN                ,   //!< Successor tasks (run after current task)
    MAP                 ,   //!< Mapping directive for glob mapping (SOURCES -> OBJECTS)
    MULTITHREAD         ,   //!< Allow multi-thread expansion/execution semantics
    MAIN                ,   //!< Marks the main task (entry)
    ENGINE              ,   //!< Select engine for task (or default env engine)
    CACHE               ,   //!< Task triggers cache
    ECHO                ,   //!< Control command echoing
    EXCLUDE             ,   //!< Exclusion pattern(s) from glob/expansion
    GLOB                ,   //!< Glob pattern(s)
    IFOS                ,   //!< OS-specific selection (mangled with @@<os>)
    DEATH               ,

    ATTRIBUTE__UNKNOWN  ,   //!< Sentinel for invalid/unrecognized attribute
    ATTRIBUTE__COUNT    ,   //!< Total number of attribute types (must be last valid index + 1)
};



/**
 * @brief Requirement on whether an attribute must have properties.
 *
 * Example:
 * - `@pub` -> NO_PROPERTY
 * - `@requires A B` -> REQUIRED_PROPERTY
 */
enum class Qualificator
{
    NO_PROPERY       = 0,   //!< Attribute must not carry extra properties
    REQUIRED_PROPERTY   ,   //!< Attribute requires at least one property
};



/**
 * @brief Cardinality constraint on number of properties for an attribute.
 */
enum class Count
{
    ZERO             = 0,   //!< 0 properties
    ONE                 ,   //!< exactly 1 property
    UNLIMITED           ,   //!< 0..N properties depending on qualificator
};



/**
 * @brief Entities that may host an attribute.
 */
enum class Target
{
    TASK             = 0,   //!< Attribute applies to task declaration
    VARIABLE            ,   //!< Attribute applies to variable assignment
};



/**
 * @brief Forward declaration for `Attr::Attribute`.
 */
struct Attribute;



/**
 * @brief A list of strings used as attribute properties.
 *
 * Properties are already tokenized at parser level (based on grammar)
 * and stored as-is.
 */
using Properties = std::vector<std::string>;

/**
 * @brief Attribute list attached to a semantic entity (task or variable).
 */
using List       = std::vector<Attribute>;

/**
 * @brief List of allowed targets for an attribute type.
 */
using Targets    = std::vector<Target>;

/**
 * @brief Table of semantic attribute rules indexed by `Attr::Type`.
 *
 * This structure is used by the semantic engine to validate that an attribute:
 * - targets correct entity type (task/variable)
 * - has correct number of properties
 * - has correct qualificator requirements
 */
using Rules      = std::array<Semantic::Rule, _I(Type::ATTRIBUTE__COUNT)>;



/**
 * @brief Concrete attribute instance attached to a task or variable.
 *
 * It contains:
 * - `name`: original spelling used in the script (useful for diagnostics)
 * - `type`: normalized/recognized type
 * - `props`: property strings (if any)
 *
 * @note Equality operator is intentionally limited to compare with `Attr::Type`.
 */
struct Attribute
{
    std::string name;      //!< Raw attribute name as typed in source (e.g. "pub", "requires")
    Type        type;      //!< Normalized attribute kind
    Properties  props;     //!< Attribute property tokens
    Context     context;

    /**
     * @brief Default constructor yields an unknown attribute.
     */
    Attribute() 
        :
        type(Type::ATTRIBUTE__UNKNOWN)
    {}

    /**
     * @brief Construct an attribute with name/type and explicit properties.
     * @param name Attribute name (raw)
     * @param t    Normalized type
     * @param p    Properties list
     */
    Attribute(const std::string& name, const Type t, const Properties& p) 
        :
        name(name),
        type(t),
        props(p)
    {}

    /**
     * @brief Compare attribute instance with a type.
     * @param t The type to compare.
     * @return true if this attribute has the given type.
     */
    bool operator == (const Type t) const { return this->type == t; }
};


END_NAMESPACE(Attr)

                                                                                                                   






//    ███╗   ██╗ █████╗ ███╗   ███╗███████╗███████╗██████╗  █████╗  ██████╗███████╗    ████████╗ █████╗ ███████╗██╗  ██╗
//    ████╗  ██║██╔══██╗████╗ ████║██╔════╝██╔════╝██╔══██╗██╔══██╗██╔════╝██╔════╝    ╚══██╔══╝██╔══██╗██╔════╝██║ ██╔╝
//    ██╔██╗ ██║███████║██╔████╔██║█████╗  ███████╗██████╔╝███████║██║     █████╗         ██║   ███████║███████╗█████╔╝ 
//    ██║╚██╗██║██╔══██║██║╚██╔╝██║██╔══╝  ╚════██║██╔═══╝ ██╔══██║██║     ██╔══╝         ██║   ██╔══██║╚════██║██╔═██╗ 
//    ██║ ╚████║██║  ██║██║ ╚═╝ ██║███████╗███████║██║     ██║  ██║╚██████╗███████╗       ██║   ██║  ██║███████║██║  ██╗
//    ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝╚══════╝       ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝
//                                                                                                                                                                                                      

                                                                                                                    

BEGIN_NAMESPACE(Task)

/**
 * @brief Task input variable names as declared in `task Name(INPUTS)`.
 *
 * Inputs are *names* of variables in the VTable; they are later resolved to values
 * and possibly glob-expanded.
 */
using Inputs = std::vector<std::string>;

/**
 * @brief Task instruction lines (the command templates to be executed).
 *
 * Instructions are stored as strings and later expanded (`arcb::...` placeholders, etc.)
 * and eventually executed by the runtime/job system.
 */
using Instrs = std::vector<std::string>;

END_NAMESPACE(Task)



                     




//    ███╗   ██╗ █████╗ ███╗   ███╗███████╗███████╗██████╗  █████╗  ██████╗███████╗    ██╗   ██╗███████╗██╗███╗   ██╗ ██████╗ 
//    ████╗  ██║██╔══██╗████╗ ████║██╔════╝██╔════╝██╔══██╗██╔══██╗██╔════╝██╔════╝    ██║   ██║██╔════╝██║████╗  ██║██╔════╝ 
//    ██╔██╗ ██║███████║██╔████╔██║█████╗  ███████╗██████╔╝███████║██║     █████╗      ██║   ██║███████╗██║██╔██╗ ██║██║  ███╗
//    ██║╚██╗██║██╔══██║██║╚██╔╝██║██╔══╝  ╚════██║██╔═══╝ ██╔══██║██║     ██╔══╝      ██║   ██║╚════██║██║██║╚██╗██║██║   ██║
//    ██║ ╚████║██║  ██║██║ ╚═╝ ██║███████╗███████║██║     ██║  ██║╚██████╗███████╗    ╚██████╔╝███████║██║██║ ╚████║╚██████╔╝
//    ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝╚══════╝     ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝ 
//                                                                                                                            




BEGIN_NAMESPACE(Using)

/**
 * @brief `using ...` directive kinds supported by Arcb DSL.
 *
 * They configure environment-wide defaults, e.g. list of profiles, default engine,
 * max threads.
 */
enum class Type
{
    PROFILES             = 0,  //!< `using profiles ...`
    ENGINE                  ,  //!< `using default engine ...`
    THREADS                 ,  //!< `using threads ...`
};



/**
 * @brief Semantic rule for a `using` directive.
 *
 * `valid_attr` is used to validate which attributes may accompany that directive
 * (if you support attributes on `using` lines).
 *
 * @note If attributes are not allowed on `using`, this still can be useful for future extension.
 */
struct Rule
{
    std::vector<std::string> valid_attr;  //!< List of allowed attribute names
    Type                     using_type;  //!< Normalized `using` kind
};

END_NAMESPACE(Using)










//    ███████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ██████╗ ███████╗
//    ██╔════╝██╔═══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔══██╗██╔════╝
//    █████╗  ██║   ██║██████╔╝██║ █╗ ██║███████║██████╔╝██║  ██║███████╗
//    ██╔══╝  ██║   ██║██╔══██╗██║███╗██║██╔══██║██╔══██╗██║  ██║╚════██║
//    ██║     ╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║██████╔╝███████║
//    ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝
//           



class  Engine;
struct AssertCheck;
struct InstructionAssign;
struct InstructionTask;
struct Enviroment;



//    ██╗   ██╗███████╗██╗███╗   ██╗ ██████╗ ███████╗
//    ██║   ██║██╔════╝██║████╗  ██║██╔════╝ ██╔════╝
//    ██║   ██║███████╗██║██╔██╗ ██║██║  ███╗███████╗
//    ██║   ██║╚════██║██║██║╚██╗██║██║   ██║╚════██║
//    ╚██████╔╝███████║██║██║ ╚████║╚██████╔╝███████║
//     ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝
//                                                                                                                                                                        


/**
 * @brief Bring Support symbols into Semantic scope.
 *
 * You use `SemanticOutput` and other utility types without prefixing.
 */
USE_MODULE(Arcb::Support);

/**
 * @brief Mutable reference wrapper alias.
 */
template<typename T>
using Ref  = std::reference_wrapper<T>;

/**
 * @brief Const reference wrapper alias.
 */
template<typename T> 
using CRef = std::reference_wrapper<const T>;

/**
 * @brief Variable table: maps variable name to assignment instruction.
 *
 * @note `std::map` ensures stable ordering (useful for deterministic output).
 */
using VTable      = std::map<std::string, InstructionAssign>;

/**
 * @brief Task table: maps task name to task instruction.
 */
using FTable      = std::map<std::string, InstructionTask>;

/**
 * @brief Assertions list.
 */
using ATable      = std::vector<AssertCheck>;

/**
 * @brief Convenience list forms.
 */
using VList       = std::vector<InstructionAssign>;
using FList       = std::vector<InstructionTask>;
using FListCRef   = std::vector<CRef<InstructionTask>>;







//    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗     █████╗ ███╗   ██╗██████╗      ██████╗██╗      █████╗ ███████╗███████╗███████╗███████╗
//    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝    ██╔══██╗████╗  ██║██╔══██╗    ██╔════╝██║     ██╔══██╗██╔════╝██╔════╝██╔════╝██╔════╝
//    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗    ███████║██╔██╗ ██║██║  ██║    ██║     ██║     ███████║███████╗███████╗█████╗  ███████╗
//    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║    ██╔══██║██║╚██╗██║██║  ██║    ██║     ██║     ██╔══██║╚════██║╚════██║██╔══╝  ╚════██║
//    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║    ██║  ██║██║ ╚████║██████╔╝    ╚██████╗███████╗██║  ██║███████║███████║███████╗███████║
//    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝    ╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝      ╚═════╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚══════╝
//       



struct Executor
{
    std::string              command;
    std::vector<std::string> flags;
    std::string              argument;
    std::string              ext;
    bool                     is_default;

    enum Type { RAW, FILE }  type = FILE;

    Executor ()
        : is_default(false)
    {}

    inline std::string Get_Repr(const bool truncated = false) const  
    {
        if (truncated)
        {
            return command + " " + Support::join(flags, " ");
        }

        return command + " " + Support::join(flags, " ") + " \"" + argument + "\"";
    }

    Executor& operator=(const Executor& other) = default;
};



/**
 * @brief Attribute semantic rule descriptor.
 *
 * This models constraints for a given `Attr::Type`, such as:
 * - whether properties are required
 * - how many properties are allowed
 * - which targets (task/variable) are valid
 *
 * @note This struct is used by `Engine` to validate collected attributes.
 */
struct Rule 
{
    Attr::Qualificator qual;   //!< Whether the attribute needs properties
    Attr::Count        count;  //!< How many properties are allowed
    Attr::Targets      targets;//!< Which entities can carry the attribute

    /**
     * @brief Compare rule with a qualificator value.
     */
    bool operator == (const Attr::Qualificator q) const { return this->qual == q; }
};



/**
 * @brief Assertion statement collected from the script.
 *
 * An assert compares two sides with an operator, or triggers a dependency check.
 * It can be evaluated after expansion.
 *
 * @note The `search_path` is used for dependency-related asserts (e.g. `{fs:...}`).
 * @warning `search_path` is meaningful only if `check == DEPENDENCIES`.
 */
struct AssertCheck
{
    /**
     * @brief Supported assert check types.
     */
    enum class CheckType : std::uint8_t
    {
        EQUAL,         //!< Equality check (==)
        NOT_EQUAL,     //!< Inequality check (!=)
        IN,            //!< Membership check ("A" in "B")
        DEPENDENCIES,  //!< File/dependency existence check under a base path
    };

    /**
     * @brief Supported assert check types.
     */
    enum class Type : std::uint8_t
    {
        MESSAGE,     //!< Standard assert, message to stdout
        ACTIONS,     //!< Callback Action, no early exit
    };

    std::size_t line;                 //!< Source line number (1-based)
    std::string stmt;                 //!< Raw assert statement string (for diagnostics)
    std::string lvalue;               //!< Left side expression (expanded later)
    std::string raw_rvalue;               //!< Left side expression (expanded later)
    std::vector<std::string> rvalue;               //!< Right side expression (expanded later)
    CheckType   check;                //!< Operation kind
    std::string reason;               //!< Human-readable failure reason template
    std::vector<std::string> actions; //!< Human-readable failure reason template
    Type        type;

    Context     context;

    std::vector<std::filesystem::path> search_paths;//!< Base path used for DEPENDENCIES (lvalue appended externally)
};



/**
 * @brief Variable assignment instruction.
 *
 * Holds:
 * - variable name and its text value
 * - attributes attached to the variable
 * - `glob_expansion`: computed list of filesystem matches for glob values
 *
 * @note `glob_expansion` is filled by environment expansion, not by the parser.
 */
struct InstructionAssign
{
    std::string              var_name;        //!< Variable identifier
    std::vector<std::string> var_value;       //!< Raw value string (may contain `{arcb::...}` tokens)
    std::vector<std::string> glob_expansion;  //!< Result of glob expansion (if var_value is a glob)
    Attr::List               attributes;      //!< Attributes attached to this variable
    Context                  context;

    InstructionAssign() = default;

    /**
     * @brief Construct a variable assignment.
     * @param var Variable name.
     * @param val Variable value (raw, not expanded).
     */
    InstructionAssign(const std::string& var, const std::string* val)
        :
        var_name(var)
    {
        if (val) var_value.push_back(*val);
    }

    // copy
    InstructionAssign(const InstructionAssign& other)             = default;
    InstructionAssign& operator=(const InstructionAssign & other) = default;

    inline std::string GetListValue(const std::size_t end = 0, const std::size_t start = 0) const
    {
        if (var_value.empty()) return "";
        
        size_t _s = start;
        size_t _e = (end) ? end : var_value.size(); 

        std::stringstream ss;
        
        for ( ; _s < _e ; ++_s) 
        {
            ss << var_value[_s];
            if ((_s + 1) != _e) ss << " ";
        }

        return ss.str();
    }

    inline std::string GetList(const std::size_t end, const std::size_t start, const std::vector<std::string>* buffer) const
    {
        size_t _s = start;
        size_t _e = (end) ? end : buffer->size(); 

        std::stringstream ss;
        
        for ( ; _s < _e ; ++_s) 
        {
            ss << buffer->at(_s);
            if ((_s + 1) != _e) ss << " ";
        }

        return ss.str();
    }

    inline std::string GetListGlob(const std::size_t end = 0, const std::size_t start = 0) const
    {
        size_t _s = start;
        size_t _e = (end) ? end : glob_expansion.size(); 

        std::stringstream ss;
        
        for ( ; _s < _e ; ++_s) 
        {
            ss << glob_expansion[_s];
            if ((_s + 1) != _e) ss << " ";
        }

        return ss.str();
    }

    /**
     * @brief Check whether an attribute is present.
     * @param attr Attribute type.
     * @return true if attribute list contains `attr`.
     */
    bool hasAttribute(const Attr::Type attr) const
    {
        return (std::find(attributes.begin(), attributes.end(), attr) != attributes.end());
    }

    /**
     * @brief Get properties for a given attribute type.
     * @param attr Attribute type to search for.
     * @return Properties list (copy). Empty if not present.
     *
     * @note This returns by value; if you need performance, consider returning a pointer/view.
     */
    const Attr::Properties
    getProperties(const Attr::Type attr) const
    {
        for (const auto& a : attributes)
            if (a.type == attr)
                return a.props;

        return {};
    }

    const std::string
    getAttrContext(const Attr::Type attr) const
    {
        for (const auto& a : attributes)
            if (a.type == attr)
                return a.context.str();

        return {};
    }

    bool empty() 
    {
        return glob_expansion.empty() && (var_value.empty() || var_value[0] == "");
    }
};



/**
 * @brief Task declaration instruction.
 *
 * Holds:
 * - name, declared input variables, and instruction strings
 * - resolved dependencies (`dependencies`) and successor tasks (`thens`) as references
 * - attached attributes
 * - engine override and cache-related flags
 *
 * @note Dependencies are stored as const references to tasks, meaning they must refer
 *       to tasks that outlive the `InstructionTask` instance (managed by `FTable`).
 */
struct InstructionTask
{
    std::string  task_name;      //!< Task identifier
    Task::Instrs task_instrs;    //!< Instruction strings (command templates)
    FListCRef    dependencies;   //!< Resolved dependency tasks (const references)
    FListCRef    thens;          //!< Resolved successor tasks (const references)
    Attr::List   attributes;     //!< Attributes attached to task
    Executor     engine;         //!< Executor override (if any)
    Context      context;
    bool         expanded;

    
    struct Cache
    {
        enum Type { TRACK, UNTRACK, STORE } type = UNTRACK;
        bool enabled = false;
        std::vector<std::string> data = {};
    }
    cache;

    InstructionTask() = default;

    /**
     * @brief Construct task instruction with basic fields.
     * @param name Task name.
     * @param inputs Declared input variable names.
     * @param instrs Task instruction templates.
     */
    InstructionTask(const std::string&  name,
                    const Task::Instrs& instrs)
        :
        task_name(name),
        task_instrs(instrs)
    {}

    // copy
    InstructionTask(const InstructionTask& other)            = default;
    InstructionTask& operator=(const InstructionTask& other) = default;

    /**
     * @brief Check whether an attribute is present.
     * @param attr Attribute type.
     * @return true if attribute list contains `attr`.
     */
    bool hasAttribute(const Attr::Type attr) const
    {
        return (std::find(attributes.begin(), attributes.end(), attr) != attributes.end());
    }

    /**
     * @brief Get properties for a given attribute type.
     * @param attr Attribute type to search for.
     * @return Properties list (copy). Empty if not present.
     */
    const Attr::Properties
    getProperties(const Attr::Type attr) const
    {
        for (const auto& a : attributes)
            if (a.type == attr)
                return a.props;

        return {};
    }

    const std::string
    getAttrContext(const Attr::Type attr) const
    {
        for (const auto& a : attributes)
            if (a.type == attr)
                return a.context.str();

        return {};
    }

    /**
     * @brief Remove the first occurrence of an attribute type.
     * @param attr Attribute type to remove.
     *
     * @note This removes only the first match and returns immediately.
     */
    void removeAttribute(const Attr::Type attr)
    {
        for (auto it = attributes.begin(); it != attributes.end(); ++it)
        {    
            if (it->type == attr)
            {
                attributes.erase(it);
                return;
            }
        }
    }
};



/**
 * @brief Profile configuration extracted from `using profiles ...`.
 *
 * `profiles` holds all declared profile names.
 * `selected` is the active profile chosen from CLI or defaults.
 *
 * @note `merge()` is used during environment import/merge.
 */
struct Profile
{
    std::vector<std::string> profiles; //!< Declared profiles
    std::string              selected; //!< Active profile name

    /**
     * @brief Merge another profile list into this one.
     * @param other Other profile container.
     *
     * @note This performs an append; it does not de-duplicate.
     */
    void merge(Profile& other)
    {
        for (const auto& val : other.profiles)
            this->profiles.push_back(val);
    }
};



/**
 * @brief Semantic environment produced by `Semantic::Engine`.
 *
 * This is the central state object after parsing:
 * - variables (VTable)
 * - tasks (FTable)
 * - asserts (ATable)
 * - selected profile / defaults
 *
 * It also provides post-processing:
 * - alignment (`AlignEnviroment`) to resolve profile/OS mangling overlays
 * - expansion (`Expand`) to replace `{arcb::...}` symbols and compute globs
 * - assert execution (`ExecuteAsserts`)
 *
 * @note `Expand()` has historically grown complex; the nested `Expander` helper
 *       is a step toward isolating the transformation logic.
 */
struct Enviroment
{
    friend class Engine;
    friend class Manager;
    friend inline void EnvMerge(Enviroment& dst, Enviroment& src);

public:
    /**
     * @brief Construct environment with no explicit max thread limit.
     *
     * `max_threads == 0` usually means "use machine default / hardware_concurrency".
     */
    Enviroment() : max_threads(0) {}

    VTable   vtable; //!< Collected variables
    FTable   ftable; //!< Collected tasks
    ATable   atable; //!< Collected assertions

    /**
     * @brief Align environment according to selected profile and OS.
     *
     * This typically resolves profile-scoped keys (mangled with @@<profile>)
     * and OS-scoped keys (mangled with @@<os>).
     *
     * @return optional error message. Empty optional on success.
     */
    Arcb_Result AlignEnviroment() noexcept;

    /**
     * @brief Validate CLI arguments against the collected environment.
     *
     * Example checks:
     * - selected task exists and is public (if required)
     * - profile exists
     * - thread count is valid
     *
     * @param args Parsed CLI arguments.
     * @return ARCB_RESULT__OK or ARCB_RESULT__NOK (or other codes you define).
     */
    Arcb_Result CheckArgs(const Arcb::Support::Arguments& args) noexcept;

    /**
     * @brief Expand all strings in the environment.
     *
     * Typical operations:
     * - expand internal symbols: `{arcb::__profile__}`, `{arcb::__os__}`, etc.
     * - expand variables: `{arcb::NAME}`
     * - extract filesystem placeholders: `{fs:...}`
     * - compute glob expansion lists for variables
     * - expand strings inside tasks and asserts
     *
     * @return optional error message. Empty optional on success.
     */
    Arcb_Result Expand() noexcept;

    /**
     * @brief Evaluate collected assertions after expansion.
     * @return optional error message. Empty optional on success.
     */
    Arcb_Result ExecuteAsserts(std::vector<std::string>& reco_cb) noexcept;


    
    /**
     * @brief Get the default engine configured by `using default engine`.
     */
    Executor                      GetInterpreter() noexcept { return default_interpreter; }

    /**
     * @brief Get the configured max threads.
     * @return Thread count (0 may mean "auto").
     */
    uint32_t                         GetThreads()     noexcept { return max_threads;         }

    /**
     * @brief Get profile configuration and selection.
     */
    Profile&                         GetProfile()     noexcept { return profile;             }

private:
    Profile     profile;             //!< Profiles list and selected profile
    Executor default_interpreter; //!< Default engine for tasks without override
    uint32_t    max_threads;         //!< Max parallelism configured by `using threads`

    /**
     * @brief Helper that encapsulates expansion logic.
     *
     * The helper owns pre-compiled regex patterns, and operates on strings
     * by reference, using the parent env for lookups.
     */
    struct Expander
    {        
        /**
         * @brief Expansion algorithm selector for `{arcb::<mode>:<var>}` patterns.
         */
        enum class Algorithm : uint8_t
        {
            NORMAL,
            LIST,
            INLINE,
            FILESYSTEM,
            SIZE,
            EMPTY,
        };

        using ExpansionMap = Support::AbstractKeywordMap<Algorithm>;

        Enviroment& env;                  //!< Reference to parent environment

        const std::regex re_intern;       //!< Matches internal `{arcb::__...__}` symbols
        const std::regex re_arc;          //!< Matches `{arcb::NAME}` variable references
        const std::regex re_arc_mode;     //!< Matches `{arcb::<mode>:<var>}` variable references

        const ExpansionMap Expansion_Map;

        std::stringstream  error;

        std::map<Algorithm, std::string> Reversed_Expansion_Map = 
        {
            { Algorithm::NORMAL    , ""       },
            { Algorithm::LIST      , "list"   },
            { Algorithm::INLINE    , "inline" },
            { Algorithm::FILESYSTEM, "fs"     },
            { Algorithm::SIZE      , "size"   },
            { Algorithm::EMPTY     , "empty"  },
        };

        struct List
        {
            struct Match
            {
                Algorithm   algo;
                size_t      start;
                size_t      count;
                size_t      pattern_len;
                InstructionAssign* datasource;
                std::vector<std::string>* buffer;


                size_t      lower_index;
                size_t      higher_index;
            };

            std::string        source;
            std::vector<Match> matches;
        };

        /**
         * @brief Construct an expander for the given environment.
         * @param e Environment reference.
         */
        explicit Expander(Enviroment& e) noexcept
            : env(e)
            , re_intern(R"(arcb::(__profile__|__version__|__release__|__main__|__root__|__max_threads__|__threads__|__os__|__arch__|__path__))")
            , re_arc(R"(arcb::([A-Za-z_][A-Za-z0-9_]*)(?![A-Za-z0-9_\.]))")
            , re_arc_mode(R"(arcb::([a-zA-Z][a-zA-Z0-9]*)\.([A-Za-z][a-z]+)\(\s*([^)]*)\s*\))")
            , Expansion_Map ({
                { "list"   , Algorithm::LIST       },
                { "inline" , Algorithm::INLINE     },
                { "fs"     , Algorithm::FILESYSTEM },
                { "size"   , Algorithm::SIZE       },
                { "empty"  , Algorithm::EMPTY      },
            })
        {}

        /**
         * @brief Expand one string:
         * - internal expansion
         * - variable expansion
         *
         * @param s String to modify in-place.
         * @return optional error message.
         */
        Arcb_Result ExpandText(std::string& s,
                                 const std::vector<Algorithm>& allowed_algorithms,
                                 std::vector<std::string>* list_exp = nullptr, 
                                 bool* used_algo = nullptr) noexcept;

        /**
         * @brief Expand one side of an assert and update `AssertCheck` accordingly.
         *
         * This is typically responsible for:
         * - expanding `{arcb::...}` tokens inside assert side
         * - collecting glob expansions for referenced variables (if needed)
         * - extracting `{fs:...}` dependencies base path and setting assert.check
         *
         * @param stmt   String side (lvalue or rvalue) to expand in-place.
         * @param assert Assert structure to mutate.
         * @return optional error message.
         */
        Arcb_Result ExpandAssertSide(std::string& stmt, AssertCheck& assert, bool rvalue = false) noexcept;

        
        std::string Get_Error() const { return error.str(); }

        static std::string AlgorithmRepr(Algorithm algo)
        {
            switch (algo)
            {
                case Algorithm::NORMAL    : return ""      ;
                case Algorithm::LIST      : return "list"  ;
                case Algorithm::INLINE    : return "inline";
                case Algorithm::FILESYSTEM: return "fs"    ;
                case Algorithm::SIZE      : return "size"  ;
                case Algorithm::EMPTY     : return "empty" ;
            }

            return "";
        }
    private:
        /**
         * @brief Extract all `{fs:...}` occurrences from an expanded string.
         * @param s Expanded string to scan.
         * @param out Output list of extracted filesystem base paths.
         *
         * @note This does *not* validate paths on filesystem; it only parses.
         */
        Arcb_Result ExtractFsPaths(const std::string& s, std::vector<std::filesystem::path>& out) noexcept;

        /**
         * @brief Expand internal tokens (`{arcb::__...__}`) inside a string.
         * @param s String to modify in-place.
         * @return optional error message.
         */
        Arcb_Result ExpandInternals(std::string& s) noexcept;

        /**
         * @brief Expand all `{arcb::NAME}` occurrences repeatedly (handles chaining/nesting).
         * @param s String to modify in-place.
         * @return optional error message (e.g. undefined variable or depth limit).
         */
        Arcb_Result ExpandArcAll(std::string& s,
                                   const std::vector<Algorithm>& allowed_algorithms,
                                   std::vector<std::string>* list_exp,
                                   bool* used_algo = nullptr) noexcept;
    };
};



/**
 * @brief Merge two environments.
 *
 * Semantics:
 * - variables/tasks from `src` move into `dst` (overwriting same keys)
 * - profile list merged (append)
 * - default engine overwritten by src engine
 * - max_threads overwritten only if src.max_threads != 0
 * - asserts appended
 *
 * @warning This merge is destructive for `src` (moves out values).
 */
inline void EnvMerge(Enviroment& dst, Enviroment& src)
{
    for (auto& [k, v] : src.vtable)
        dst.vtable[k] = std::move(v);

    for (auto& [k, v] : src.ftable)
        dst.ftable[k] = std::move(v);

    dst.profile.merge(src.profile);

    dst.default_interpreter = src.default_interpreter;

    if (src.max_threads != 0)
    {
        dst.max_threads = src.max_threads;
    }

    for (auto& a : src.atable)
        dst.atable.push_back(a);
}



/**
 * @brief Semantic engine collecting instructions from parser events.
 *
 * The parser calls `Collect_*` methods as it recognizes DSL statements.
 * The engine:
 * - validates and normalizes attributes
 * - builds up VTable/FTable/ATable
 * - enforces invariants (e.g. single MAIN task)
 *
 * @note The engine owns its environment; caller can obtain a copy via GetEnvironment()
 *       or a mutable reference via EnvRef().
 */
class Engine
{
public:
    /**
     * @brief Construct semantic engine with attribute rule table initialized.
     */
    Engine();

    void Set_Context(const Context& c) { _context = c; }

    /**
     * @brief Collect one attribute statement.
     * @param name Attribute name (raw).
     * @param prop Attribute property string (raw, may need splitting).
     * @return SemanticOutput containing status and error/hint if any.
     */
    Arcb_Result Collect_Attribute (const std::string& name, const std::string&  prop);

    /**
     * @brief Collect one variable assignment statement.
     * @param name Variable name.
     * @param val  Variable value (raw).
     * @return SemanticOutput containing status and error/hint if any.
     */
    Arcb_Result Collect_Assignment(const std::string& name, const std::string&  val, bool join = false); 

    /**
     * @brief Collect one task declaration.
     * @param name   Task name.
     * @param instrs Instruction lines.
     * @return SemanticOutput containing status and error/hint if any.
     */
    Arcb_Result Collect_Task      (const std::string& name, const Task::Instrs& instrs);

    /**
     * @brief Collect a `using` directive.
     * @param what Directive keyword (e.g. "profiles", "default engine", "threads").
     * @param opt  Directive argument (raw string).
     * @return SemanticOutput containing status and error/hint if any.
     */
    Arcb_Result Collect_Using     (const std::string& what, const std::string&  opt); 

    /**
     * @brief Collect a mapping statement.
     * @param item_1 Left item (source).
     * @param item_2 Right item (destination).
     * @return SemanticOutput containing status and error/hint if any.
     */
    Arcb_Result Collect_Mapping   (const std::string& item_1, const std::string& item_2);

    /**
     * @brief Collect an assert statement.
     * @param line   Source line number.
     * @param stmt   Raw assert statement string.
     * @param lvalue Left side text.
     * @param op     Operator text (==, !=, in, etc.)
     * @param rvalue Right side text.
     * @param reason Reason string.
     * @return SemanticOutput containing status and error/hint if any.
     */
    Arcb_Result Collect_Assert    (std::size_t line, const std::string& stmt, const std::string& lvalue, 
                                      const std::string& op, const std::string& rvalue, const std::string& reason, const bool actions);

    /**
     * @brief Get a *copy* of the currently collected environment.
     * @return Environment value copy.
     *
     * @note Copying may be expensive if tables are large.
     */
    Enviroment                       GetEnvironment()  const noexcept { return _env; }

    /**
     * @brief Get a mutable reference to the collected environment.
     * @return Environment reference.
     *
     * @warning Mutating the environment can break invariants expected by semantic engine.
     */
    Enviroment&                      EnvRef()                         { return _env; }

private:
    Attr::Rules  _attr_rules;   //!< Attribute rule table (indexed by Attr::Type)
    Attr::List   _attr_pending; //!< Attributes pending attachment to next entity (variable/task)
    
    Enviroment   _env;          //!< Owned environment
    Context      _context;
};


END_MODULE(Semantic)


#endif /* __ARCB_SEMANTIC__H__ */
