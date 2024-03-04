//
// Created by kery on 2024/2/24.
//
#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"

/**
 * @brief 创建一个编译过程，检测输入文件是否存在，如果存在则打开文件
 * @param filename 输入文件名
 * @param out_filename 输出文件名
 * @param flags 编译选项
 * @return 编译过程
 */
struct compile_process* compile_process_create(const char* filename, const char* out_filename, int flags)
{
    FILE* file = fopen(filename, "r");
    if(!file)
    {
        return NULL;
    }
    FILE* out_file = NULL;
    if(out_filename)
    {
        out_file = fopen(out_filename, "w");
        if(!out_file)
        {
            return NULL;
        }
    }
    // 为编译过程分配内存
    struct compile_process* process = calloc(1, sizeof(struct compile_process));
    process->flags = flags;
    process->cfile.fp = file;
    process->ofile = out_file;
    return process;
}

/**
 * @brief 读取当前词法分析过程正在处理的文件的下一个字符
 * @param lex_process 词法分析过程
 * @return 下一个字符
 */
char compile_process_next_char(struct lex_process* lex_process)
{
    struct compile_process* compiler = lex_process->compiler;
    compiler->pos.col++;
    char c = getc(compiler->cfile.fp);
    if(c == '\n')
    {
        compiler->pos.line++;
        compiler->pos.col = 1;
    }

    return c;
}

/**
 * @brief 读取当前词法分析过程正在处理的文件的下一个字符，在获取他之后再放回文件流
 * @param lex_process 词法分析过程
 * @return 下一个字符
 */
char compile_process_peek_char(struct lex_process* lex_process)
{
    struct compile_process* compiler = lex_process->compiler;
    char c = getc(compiler->cfile.fp);
    ungetc(c, compiler->cfile.fp);
    return c;
}

/**
 * @brief 将一个字符推回到文件流中
 * @param lex_process 词法分析过程
 * @param c 待推回的字符
 */
void compile_process_push_char(struct lex_process* lex_process, char c)
{
    struct compile_process* compiler = lex_process->compiler;
    ungetc(c, compiler->cfile.fp);
}