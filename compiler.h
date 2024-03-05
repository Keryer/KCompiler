//
// Created by kery on 2024/2/24.
//

#ifndef KCOMPILER_COMPILER_H
#define KCOMPILER_COMPILER_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define S_EQ(str, str2) \
        (str && str2 && (strcmp(str, str2) == 0))
/**
 * 位置结构体
 * line: 行
 * col: 列
 * filename: 文件名
 */
struct pos
{
    int line;
    int col;
    const char* filename;
};

/**
 * 对文件中遇到的数字的枚举
 */
#define NUMERIC_CASE \
    case '0':        \
    case '1':        \
    case '2':        \
    case '3':        \
    case '4':        \
    case '5':        \
    case '6':        \
    case '7':        \
    case '8':        \
    case '9'

#define OPERATOR_CASE_EXCLUDING_DIVISION \
    case '+':                            \
    case '-':                            \
    case '*':                            \
    case '%':                            \
    case '=':                            \
    case '!':                            \
    case '~':                            \
    case '&':                            \
    case '|':                            \
    case '^':                            \
    case '<':                            \
    case '>':                            \
    case '(':                            \
    case '[':                            \
    case ',':                            \
    case '.':                            \
    case '?'


#define SYMBOL_CASE \
    case '{':       \
    case ';':       \
    case ':':       \
    case ']':       \
    case '}':       \
    case ')':       \
    case '#':       \
    case '\\'

/**
 * 词法分析的结果枚举
 * LEXICAL_ANALYSIS_ALL_OK: 词法分析成功
 * LEXICAL_ANALYSIS_INPUT_ERROR: 词法分析失败
 */
enum
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_INPUT_ERROR
};

/**
 * token的类型枚举
 * TOKEN_TYPE_IDENTIFIER: 标识符
 * TOKEN_TYPE_KEYWORD: 关键字
 * TOKEN_TYPE_OPERATOR: 操作符
 * TOKEN_TYPE_SYMBOL: 符号
 * TOKEN_TYPE_NUMBER: 数字
 * TOKEN_TYPE_STRING: 字符串
 * TOKEN_TYPE_COMMENT: 注释
 * TOKEN_TYPE_NEWLINE: 换行符
 */
enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

enum
{
    NUMBER_TYPE_NORMAl,
    NUMBER_TYPE_FLOAT,
    NUMBER_TYPE_LONG,
    NUMBER_TYPE_DOUBLE
};

/**
 * token结构体
 * type: token的类型
 * flag: token的标志
 * pos: token的位置
 * cval: token的字符值
 * sval: token的字符串值
 * inum: token的整数值
 * lnum: token的长整数值
 * llnum: token的长长整数值
 * any: token的一个任意指针
 * whitespace: token之间的空格
 * between_brackets: 一个指向括号之间的字符串的指针，如果token在一个括号之间，则指向左括号之后的位置
 * e.g. 对于(10+20+30)，每个token的该指针都指向“1”所在的位置
 */
struct token
{
    int type;
    int flag;
    struct pos pos;
    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void* any;
    };


    struct token_number
    {
        int type;
    } num;
    // token之间的空格
    bool whitespace;

    const char* between_brackets;
};


struct lex_process;
/**
 * @brief 转发声明，词法分析进程函数指针，LEX_PROCESS_xxx为函数指针类型，process为该指针指向的函数的参数。
 * 注意，此时只确定了函数指针类型，并确定了每种指针类型的参数结构，还需要依据此类型建立出具体的函数指针。
 * @param lex_process 词法分析进程
 */
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process* process);     //函数指针
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process* process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process* process, char c);

/**
 * @brief 词法分析进程函数指针结构体，在此处声明了三个函数指针，分别属于上面所说的三种指针类型
 * next_char: 下一个字符
 * peek_char: 查看下一个字符
 * push_char: 推回一个字符
 */
struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

/**
 * @brief 词法分析进程结构体，保存了词法分析过程中的一些信息
 * pos: 位置
 * token_vec: 存储token向量
 * compiler: 指向编译过程的指针
 * current_expression_count: 当前表达式的数量，即有几层括号
 * parentheses_buffer: 括号缓冲区
 * function: 函数指针结构体指针
 * private: 指向一些只有使用者可以理解的私人数据
 */
struct lex_process
{
    struct pos pos;
    struct vector* token_vec;
    struct compile_process* compiler;

    int current_expression_count;
    struct buffer* parentheses_buffer;
    struct lex_process_functions* function;

    //指向一些lexer无法理解的私人数据
    void* private;
};

/**
 * 编译过程结果类型枚举
 * COMPILER_FAILED_WITH_ERRORS: 编译失败
 * COMPILER_FILE_COMPILED_OK: 文件编译成功
 */
enum
{
    COMPILER_FAILED_WITH_ERRORS,
    COMPILER_FILE_COMPILED_OK
};

/**
 * 编译过程结构体
 * flags: 编译选项
 * pos: 位置
 * cfile: 输入文件
 * ofile: 输出文件
 */
struct compile_process
{
    //编译选项
    int flags;

    struct pos pos;
    /**
     * 输入文件结构体
     * fp: 输入文件指针
     * abs_path: 文件的绝对路径
     */
    struct compile_process_input_file
    {
        FILE* fp;
        const char* abs_path;
    } cfile;

    struct vector* token_vec;
    // 词法分析得到的token向量
    FILE* ofile;
};

/***********************************************************************************************************************
 * 函数声明
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * 编译过程函数声明
 **********************************************************************************************************************/
int compile_file(const char* file_name, const char* out_filename, int flags);
struct compile_process* compile_process_create(const char* filename, const char* out_filename, int flags);

/***********************************************************************************************************************
 * 字符操作函数声明
 **********************************************************************************************************************/
char compile_process_next_char(struct lex_process* lex_process);
char compile_process_peek_char(struct lex_process* lex_process);
void compile_process_push_char(struct lex_process* lex_process, char c);

/***********************************************************************************************************************
 * 编译结果函数声明
 **********************************************************************************************************************/
void compiler_error(struct compile_process* compiler, const char* msg, ...);
void compiler_warning(struct compile_process* compiler, const char* msg, ...);

/***********************************************************************************************************************
 * 词法分析函数声明
 **********************************************************************************************************************/
struct lex_process* lex_process_create(struct compile_process* compiler, struct lex_process_functions* functions, void* private);
void lex_process_free(struct lex_process* process);
void* lex_process_private(struct lex_process* process);
struct vector* lex_process_tokens(struct lex_process* process);
int lex(struct lex_process* process);

/***********************************************************************************************************************
 * token函数声明
 **********************************************************************************************************************/
/**
 * @brief 为输入的字符串构建token
 * @param compiler
 * @param str
 * @return struct lex_process*
 */
struct lex_process* token_build_for_string(struct compile_process* compiler, const char* str);

bool token_is_keyword(struct token* token, const char* value);

#endif //KCOMPILER_COMPILER_H
