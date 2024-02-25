//
// Created by kery on 2024/2/24.
//
#include "compiler.h"
int compile_file(const char* file_name, const char* out_filename, int flags)
{
    struct compile_process* process = compile_process_create(file_name, out_filename, flags);
    if(!process)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }
    //lexical analysis

    //parsing

    //code generation

    return COMPILER_FILE_COMPILED_OK;
}