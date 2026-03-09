#ifndef __ARCANA_GENERATOR_H__
#define __ARCANA_GENERATOR_H__


#include "Defines.h"

#include <ostream>
#include <fstream>
#include <filesystem>


BEGIN_MODULE(Generator)


/**
 * @brief Default Arcana project template.
 *
 * This template represents the initial Arcana build file generated
 * when creating a new project. It defines:
 * - default profiles
 * - engine and threading configuration
 * - core variables and mappings
 * - example assertions
 * - skeleton public and private tasks
 *
 * The template is emitted verbatim either to stdout or to a file.
 */
static const char* ARCANA_TEMPLATE = R"TEMPLATE(
#!/usr/bin/arcana

using profiles Debug Release;
using default engine /bin/bash;
using threads 1;

@profile Debug;   FLAGS = -Wall -g3 -O0
@profile Release; FLAGS = -Wall -g0 -O2

COMPILER = gcc
INCLUDES = -Iincludes
SRCDIR   = src
OBJDIR   = src
TARGET   = app

@glob 
SOURCES  = {arc:SRCDIR}/*.c
OBJECTS  = {arc:OBJDIR}/*.o

map SOURCES -> OBJECTS;

assert "{arc:__os__}" eq "linux" -> "This project can only be build under linux, {arc:__os__} not admitted";

###########################
# PRIVATE TASKS
###########################


@echo
@cache track {arc:list:SOURCES} 
@multithread
task Compile() 
{
{arc:COMPILER} {arc:FLAGS} {arc:INCLUDES} -c {arc:list:SOURCES} -o {arc:list:OBJECTS}
}
    
@cache store {arc:list:SOURCES}  
task Link()
{        
{arc:COMPILER} {arc:FLAGS} {arc:inline:OBJECTS} -o {arc:TARGET}
}

###########################
# PUBLIC TASKS
###########################

@pub
@cache untrack {arc:list:SOURCES}
task Clean() 
{ 
rm -rf {arc:BUILDDIR}
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
 * @brief Generates an Arcana project template.
 *
 * Writes the default Arcana template either to a file or to stdout.
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
        MSG(ARCANA_TEMPLATE);
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

    out << ARCANA_TEMPLATE;
    return out.good();
}




END_MODULE(Generator)



#endif /* __ARCANA_GENERATOR_H__ */