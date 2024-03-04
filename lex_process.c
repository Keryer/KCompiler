//
// Created by kery on 2024/2/25.
//
#include "compiler.h"
#include "helpers/vector.h"
#include <stdlib.h>

/**
 * @brief 词法分析过程的函数指针
 * @param compiler 词法分析过程
 * @param functions 词法分析过程的函数结构体，里面包括所需要的函数的指针
 * @param private 词法分析过程的私有数据
 * @return 词法分析过程
 */
struct lex_process* lex_process_create(struct compile_process* compiler, struct lex_process_functions* functions, void* private)
{
    struct lex_process* process = calloc(1, sizeof(struct lex_process));
    process->function = functions;
    process->token_vec = vector_create(sizeof(struct token));
    process->compiler = compiler;
    process->private = private;
    process->pos.line = 1;
    process->pos.col = 1;
    return process;
}

/**
 * @brief 释放词法分析过程
 * @param process 词法分析过程
 */
void lex_process_free(struct lex_process* process)
{
    vector_free(process->token_vec);
    free(process);
}

/**
 * @brief 获取词法分析过程的私有数据
 * @param process 词法分析过程
 * @return 词法分析过程的私有数据
 */
void* lex_process_private(struct lex_process* process)
{
    return process->private;
}

/**
 * @brief 设置词法分析过程的私有数据
 * @param process 词法分析过程
 * @return 词法分析过程的私有数据
 */
struct vector* lex_process_tokens(struct lex_process* process)
{
    return process->token_vec;
}

