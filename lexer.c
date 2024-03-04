//
// Created by kery on 2024/3/2.
//
#include "compiler.h"
#include <string.h>
#include "helpers/vector.h"
#include "helpers/buffer.h"

// 如果exp的表达式的值为真，那么就执行buffer_write(buffer, c)和nextc()
// 即将c写入buffer，然后读取下一个字符
#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

struct token* read_next_token();

static struct lex_process* lex_process;
static struct token tmp_token;

/**
 * 从文件中读取下一个字符，但不获取
 * @return 下一个字符
 */
static char peekc()
{
    return lex_process->function->peek_char(lex_process);
}

/**
 * 从文件中读取下一个字符
 * @return 下一个字符
 */
static char nextc()
{
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.col++;
    if (c == '\n')
    {
        lex_process->pos.line++;
        lex_process->pos.col = 1;
    }
    return c;
}

/**
 * 将字符c推回到文件中
 * @param c 待推入的字符
 */
static void pushc(char c)
{
    lex_process->function->push_char(lex_process, c);
}

/**
 * 获取在当前文件所处位置
 * @return 位置信息
 */
static struct pos lex_file_position()
{
    return lex_process->pos;
}

/**
 * 创建一个token指针，将token结构体具象化
 * @param _token
 * @return
 */
struct token* token_create(struct token* _token)
{
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    return &tmp_token;
}

/**
 * 获取token_vec的最后一个元素
 * @return token_vec的最后一个元素
 */
static struct token* lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

/**
 * 处理空白字符
 * @return 继续处理下一个token
 */
static struct token* handle_whitespace()
{
    struct token* last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }
    nextc();
    return read_next_token();
}

/**
 * 读取一个数字字符串
 * @return 字符串在buffer中的指针
 */
const char* read_number_str()
{
    const char* num = NULL;
    struct buffer* buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

/**
 * 读取一个数字
 * @return
 */
unsigned long long read_number()
{
    const char* s = read_number_str();
    return atoll(s);
}

/**
 * 创建一个数字token结构体
 * @param number
 * @return 创建好的token结构体
 */
struct token* token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){
        .type = TOKEN_TYPE_NUMBER,
        .llnum = number
    });
}

/**
 * 创建一个数字token结构体
 * @return
 */
struct token* token_make_number()
{
    return token_make_number_for_value(read_number());
}

/**
 * 从文件中读取下一个token
 * @return 读取到的token
 */
struct token* read_next_token()
{
    struct token* token = NULL;
    char c = peekc();
    switch (c) {
        NUMERIC_CASE:
            token = token_make_number();
            break;

        case ' ':
        case '\t':
            // 略过空格和制表符
            token = handle_whitespace();
            break;

        case EOF:
            //完成了词法分析
            break;
        default:
            compiler_error(lex_process->compiler, "Unexpected token\n");
    }
    return token;
}

/**
 * 词法分析
 * @param process
 * @return
 */
int lex(struct lex_process* process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token* token = read_next_token();
    while (token)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }
    return LEXICAL_ANALYSIS_ALL_OK;
}