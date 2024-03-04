//
// Created by kery on 2024/3/2.
//
#include "compiler.h"
#include <string.h>
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <assert.h>

// 如果exp的表达式的值为真，那么就执行buffer_write(buffer, c)和nextc()
// 即将c写入buffer，然后读取下一个字符
#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

struct token *read_next_token();

static struct lex_process *lex_process;
static struct token tmp_token;

/**
 * 从文件中读取下一个字符，但不获取
 * @return 下一个字符
 */
static char peekc() {
    return lex_process->function->peek_char(lex_process);
}

/**
 * 从文件中读取下一个字符
 * @return 下一个字符
 */
static char nextc() {
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.col++;
    if (c == '\n') {
        lex_process->pos.line++;
        lex_process->pos.col = 1;
    }
    return c;
}

/**
 * 将字符c推回到文件中
 * @param c 待推入的字符
 */
static void pushc(char c) {
    lex_process->function->push_char(lex_process, c);
}

/**
 * 获取在当前文件所处位置
 * @return 位置信息
 */
static struct pos lex_file_position() {
    return lex_process->pos;
}

/**
 * 创建一个token指针，将token结构体具象化
 * @param _token
 * @return
 */
struct token *token_create(struct token *_token) {
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    return &tmp_token;
}

/**
 * 获取token_vec的最后一个元素
 * @return token_vec的最后一个元素
 */
static struct token *lexer_last_token() {
    return vector_back_or_null(lex_process->token_vec);
}

/**
 * 处理空白字符
 * @return 继续处理下一个token
 */
static struct token *handle_whitespace() {
    struct token *last_token = lexer_last_token();
    if (last_token) {
        last_token->whitespace = true;
    }
    nextc();
    return read_next_token();
}

/**
 * 读取一个数字字符串
 * @return 字符串在buffer中的指针
 */
const char *read_number_str() {
    const char *num = NULL;
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

/**
 * 读取一个数字
 * @return
 */
unsigned long long read_number() {
    const char *s = read_number_str();
    return atoll(s);
}

/**
 * 创建一个数字token结构体
 * @param number
 * @return 创建好的token结构体
 */
struct token *token_make_number_for_value(unsigned long number) {
    return token_create(&(struct token) {
            .type = TOKEN_TYPE_NUMBER,
            .llnum = number
    });
}

/**
 * 创建一个数字token结构体
 * @return
 */
struct token *token_make_number() {
    return token_make_number_for_value(read_number());
}

/**
 * 创建一个字符串token结构体
 * @param start_delim 开始分割符
 * @param end_delim 结束分割符
 * @return
 */
static struct token *token_make_string(char start_delim, char end_delim) {
    struct buffer *buf = buffer_create();
    assert(start_delim == nextc());
    char c = nextc();
    for (; c != end_delim && c != EOF; c = nextc()) {
        if (c == '\\') {
            continue;
        }

        buffer_write(buf, c);
    }
    buffer_write(buf, 0x00);
    return token_create(&(struct token) {
            .type = TOKEN_TYPE_STRING,
            .sval = buffer_ptr(buf)
    });
}

/**
 * 判断运算符是否被当作单独运算符
 * @param op 运算符
 * @return 如果是列出的运算符之一，返回true，即只能按照单独运算符处理
 */
static bool op_treated_as_one(char op) {
    return op == '(' || op == ',' || op == '[' || op == '.' || op == '?' || op == '*';
}

/**
 * 判断运算符是否是单字符运算符
 * @param op
 * @return 如果是单字符运算符，返回true
 */
static bool is_single_operator(char op) {
    return op == '+' || op == '-' || op == '/' || op == '*' || \
           op == '<' || op == '>' || op == '=' || \
           op == '&' || op == '|' || op == '!' || \
           op == '~' || op == '^' || op == '%' || \
           op == '?' || op == '.' || op == ',' || \
           op == '(' || op == '[';

}

/**
 * 判断组合运算符是否是有效的运算符
 * @param op
 * @return
 */
bool op_valid(const char *op) {
    return S_EQ(op, "+") ||
           S_EQ(op, "-") ||
           S_EQ(op, "*") ||
           S_EQ(op, "/") ||
           S_EQ(op, "%") ||
           S_EQ(op, "!") ||
           S_EQ(op, "^") ||
           S_EQ(op, "&") ||
           S_EQ(op, "|") ||
           S_EQ(op, "~") ||
           S_EQ(op, ">") ||
           S_EQ(op, "<") ||
           S_EQ(op, "=") ||
           S_EQ(op, "==") ||
           S_EQ(op, "!=") ||
           S_EQ(op, "<=") ||
           S_EQ(op, ">=") ||
           S_EQ(op, "&&") ||
           S_EQ(op, "||") ||
           S_EQ(op, "++") ||
           S_EQ(op, "--") ||
           S_EQ(op, "+=") ||
           S_EQ(op, "-=") ||
           S_EQ(op, "*=") ||
           S_EQ(op, "/=") ||
           S_EQ(op, "%=") ||
           S_EQ(op, "<<") ||
           S_EQ(op, ">>") ||
           S_EQ(op, "->") ||
           S_EQ(op, ".") ||
           S_EQ(op, ",") ||
           S_EQ(op, "?") ||
           S_EQ(op, "...") ||
           S_EQ(op, "(") ||
           S_EQ(op, "[");
}

/**
 * 将内存中的最后一个字符送回到文件流中，并加入一个结束符
 * @param buffer 内存空间
 */
void read_op_flush_back_keep_first(struct buffer *buffer) {
    const char *data = buffer_ptr(buffer);
    int len = buffer->len;
    for (int i = len - 1; i > 1; i--) {
        if (data[i] == 0x00) {
            continue;
        }
        pushc(data[i]);
    }
}

/**
 * 从输入流中读取一个运算符并验证
 * @return
 */
const char *read_op() {
    bool single_operator = true;
    char op = nextc();
    struct buffer *buffer = buffer_create();
    buffer_write(buffer, op);

    if (!op_treated_as_one(op)) {
        // 双字符运算符，因此需要读取下一个字符
        op = peekc();
        if (is_single_operator(op)) {
            // 读取到了双字符运算符的第二个字符
            buffer_write(buffer, op);
            nextc();
            single_operator = false;
        }
    }
    buffer_write(buffer, 0x00);
    // 加入字符串结束符

    char *ptr = buffer_ptr(buffer);
    if (!single_operator)
        // 疑似双字符运算符
    {
        if (!op_valid(ptr))
            // 验证双字符的有效性，如果不是有效的双字符运算符，则先把第二个字符送回，再提前插入一个结束符
        {
            read_op_flush_back_keep_first(buffer);
            ptr[1] = 0x00;
            // 如果是多字符运算符，但又不在列表中，则提前插入一个结束符，表示这个操作符到此为止
        }
    } else if (!op_valid(ptr))
        // 如果是单字符运算符，但不在列表中
    {
        compiler_error(lex_process->compiler, "The operator %s is not valid\n", ptr);
    }
    return ptr;
}

/**
 * 读取一个新的表达式，一旦遇到一个左括号，就会增加一个表达式，然后建立括号的缓冲区
 */
static void lex_new_expression() {
    lex_process->current_expression_count++;
    if (lex_process->current_expression_count == 1) {
        lex_process->parentheses_buffer = buffer_create();
    }
}

/**
 * 结束一个表达式
 */
static void lex_finish_expression() {
    lex_process->current_expression_count--;
    if (lex_process->current_expression_count < 0) {
        compiler_error(lex_process->compiler, "Unexpected ')'\n");
    }
}

/**
 * 检测当前是否在一个表达式内部
 */
bool lex_is_in_expression() {
    return lex_process->current_expression_count > 0;
}

/**
 * 读取一个新的表达式
 * @return
 */
static struct token *token_make_operator_or_string() {
    char op = peekc();
    if (op == '<')
    {
        struct token* last_token = lexer_last_token();
        if (token_is_keyword(last_token, "include"))
        {
            return token_make_string('<', '>');
        }
    }

    struct token* token = token_create(&(struct token) {
            .type = TOKEN_TYPE_OPERATOR,
            .sval = read_op()
    });

    if (op == '(')
    // 如果是一个表达式
    {
        lex_new_expression();

    }
    return token;
}

/**
 * 创建一个符号token结构体
 * @return 符号token结构体
 */
static struct token *token_make_symbol() {
    char c = nextc();
    if (c == ')')
    {
        lex_finish_expression();
    }
    struct token* token = token_create(&(struct token) {
            .type = TOKEN_TYPE_SYMBOL,
            .cval = c
    });
    return token;
}

/**
 * 从文件中读取下一个token
 * @return 读取到的token
 */
struct token *read_next_token() {
    struct token *token = NULL;
    char c = peekc();
    switch (c) {
        NUMERIC_CASE:
            // 读取到了数字
            token = token_make_number();
            break;

        OPERATOR_CASE_EXCLUDING_DIVISION:
            // 读取到了字符串
            token = token_make_operator_or_string();
            break;

        SYMBOL_CASE:
            // 读取到了符号
            token = token_make_symbol();
            break;

        case '"':
            // 读取到了字符串开始
            token = token_make_string('"', '"');
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
int lex(struct lex_process *process) {
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token *token = read_next_token();
    while (token) {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }
    return LEXICAL_ANALYSIS_ALL_OK;
}