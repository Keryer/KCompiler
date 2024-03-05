//
// Created by kery on 2024/2/24.
//
#include "compiler.h"
#include <stdarg.h>
#include <stdlib.h>
/**
 * @brief 在此处三个函数指针分别被赋值为三个具体的函数，之后可以用这三个函数指针来调用这三个函数
 */
struct lex_process_functions compiler_lex_functions = {
        .next_char = compile_process_next_char,
        .peek_char = compile_process_peek_char,
        .push_char = compile_process_push_char
};

/**
 * 输出编译时产生的错误信息
 * @param compiler 产生错误的编译进程
 * @param msg 错误信息内容
 * @param ...
 */
void compiler_error(struct compile_process* compiler, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line, compiler->pos.col, compiler->pos.filename);
    exit(-1);
}

/**
 * 输出编译时产生的警告信息
 * @param compiler 产生警告的编译进程
 * @param msg 警告信息内容
 * @param ...
 */
void compiler_warning(struct compile_process* compiler, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line, compiler->pos.col, compiler->pos.filename);
}

/**
 *  编译文件的起始函数
 *
 *  @param file_name 待编译的文件名
 *  @param out_filename 编译结果输出文件名
 *  @param flags 编译选项
 *  @return 编译结果
 */
int compile_file(const char* file_name, const char* out_filename, int flags)
{
    struct compile_process* process = compile_process_create(file_name, out_filename, flags);
    if(!process)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }
    // lexical analysis
    // 传入了一个指针结构体，相当于传入了三个函数
    struct lex_process* lex_process = lex_process_create(process, &compiler_lex_functions, NULL);
    if (!lex_process)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }
    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }
    process->token_vec = lex_process->token_vec;

    //parsing

    //code generation

    return COMPILER_FILE_COMPILED_OK;
}