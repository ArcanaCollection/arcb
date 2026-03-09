#ifndef __ARCB_GENERATOR_H__
#define __ARCB_GENERATOR_H__


#include "Defines.h"

#include <ostream>
#include <fstream>
#include <filesystem>


BEGIN_MODULE(Generator)


/**
 * @brief Default Arcb project template.
 *
 * This template represents the initial Arcb build file generated
 * when creating a new project. It defines:
 * - default profiles
 * - engine and threading configuration
 * - core variables and mappings
 * - example assertions
 * - skeleton public and private tasks
 *
 * The template is emitted verbatim either to stdout or to a file.
 */
static const char* ARCB_TEMPLATE = R"TEMPLATE(
#!/usr/bin/arcb

using profiles Debug Release;
using engine file .sh /bin/bash;
using threads 1;

@profile Debug;   FLAGS = -Wall -g3 -O0
@profile Release; FLAGS = -Wall -g0 -O2

ECHO     = echo
COMPILER = gcc
INCLUDES = -Iincludes
SRCDIR   = src
OBJDIR   = src
TARGET   = app

@glob 
SOURCES  = arcb::SRCDIR/*.c
OBJECTS  = arcb::OBJDIR/*.o

map SOURCES -> OBJECTS;

assert "arcb::__os__"   eq "linux"          -> "This project can only be build under linux, arcb::__os__ not admitted";
assert "arcb::ECHO"     in "arcb::__path__" -> "arcb::ECHO is required for this project";
assert "arcb::COMPILER" in "arcb::__path__" -> Death;

###########################
# PRIVATE TASKS
###########################

@death
@engine raw arcb::ECHO
task Death()
{
Error Bye! :P
}

@echo
@cache track arcb::SOURCES.list()
@threading
task Compile() 
{
arcb::COMPILER arcb::FLAGS arcb::INCLUDES -c arcb::SOURCES.list() -o arcb::OBJECTS.list()
}
    
@cache store arcb::SOURCES.list() 
task Link()
{        
arcb::COMPILER arcb::FLAGS arcb::OBJECTS.inline() -o arcb::TARGET
}

###########################
# PUBLIC TASKS
###########################

@pub
@cache untrack arcb::SOURCES.list()
task Clean() 
{ 
rm -rf arcb::BUILDDIR
}

@pub
@main Compile Link
task Build() {}

@pub
@requires Clean Build
task Rebuild() {}

@pub
@requires Rebuild
task Install()
{

}

)TEMPLATE";



/**
 * @brief Generates an Arcb project template.
 *
 * Writes the default Arcb template either to a file or to stdout.
 *
 * If the output string is empty, the default filename "arcfile" is used.
 * If the output string equals "stdout", the template is printed to stdout
 * instead of being written to a file.
 *
 * Parent directories are created automatically if they do not exist.
 *
 * @param[inout] output Output destination:
 *        - empty string: defaults to "arcfile"
 *        - "stdout": prints the template to stdout
 *        - otherwise: treated as a filesystem path
 *
 * @return true on success, false on I/O or filesystem errors.
 */
inline bool Generate_Template(std::string& output)
{
    namespace fs = std::filesystem;

    if (output.empty())
    {
        output = "arcfile";
    }
    
    if (output.compare("stdout") == 0)
    {
        MSG(ARCB_TEMPLATE);
        return true;
    }
    
    fs::path file = output;
    std::error_code ec;
    if (!file.parent_path().empty())
    {
        if (!fs::exists(file.parent_path(), ec))
        {
            if (!fs::create_directories(file.parent_path(), ec))
            {
                return false;
            }
        }
    }

    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }

    out << ARCB_TEMPLATE;
    return out.good();
}




END_MODULE(Generator)



#endif /* __ARCB_GENERATOR_H__ */