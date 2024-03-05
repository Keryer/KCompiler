//
// Created by kery on 2024/3/2.
//
#include "compiler.h"
#include <string.h>
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <assert.h>
#include <ctype.h>

// 如果exp的表达式的值为真，那么就执行buffer_write(buffer, c)和nextc()
// 即将c写入buffer，然后读取下一个字符
#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

struct token *read_next_token();

bool lex_is_in_expression();

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
    if (lex_is_in_expression()) {
        buffer_write(lex_process->parentheses_buffer, c);
    }
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


static char assert_next_char(char c) {
    char next_c = nextc();
    assert(c == next_c);
    return next_c;
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
    if (lex_is_in_expression()) {
        tmp_token.between_brackets = buffer_ptr(lex_process->parentheses_buffer);
    }
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

int lexer_number_type(char c) {
    int res = NUMBER_TYPE_NORMAl;
    if (c == 'L') {
        res = NUMBER_TYPE_LONG;
    } else if (c == 'f') {
        res = NUMBER_TYPE_FLOAT;
    } else if (c == 'l') {
        res = NUMBER_TYPE_LONG;
    } else if (c == 'd') {
        res = NUMBER_TYPE_DOUBLE;
    }
    return res;
}


/**
 * 创建一个数字token结构体
 * @param number
 * @return 创建好的token结构体
 */
struct token *token_make_number_for_value(unsigned long number) {
    int number_type = lexer_number_type(peekc());
    if (number_type != NUMBER_TYPE_NORMAl) {
        nextc();
    }
    return token_create(&(struct token) {
            .type = TOKEN_TYPE_NUMBER,
            .llnum = number,
            .num.type = number_type
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


bool is_keyword(const char *str) {
    return S_EQ(str, "unsigned") ||
           S_EQ(str, "signed") ||
           S_EQ(str, "char") ||
           S_EQ(str, "short") ||
           S_EQ(str, "int") ||
           S_EQ(str, "long") ||
           S_EQ(str, "float") ||
           S_EQ(str, "double") ||
           S_EQ(str, "void") ||
           S_EQ(str, "struct") ||
           S_EQ(str, "enum") ||
           S_EQ(str, "union") ||
           S_EQ(str, "typedef") ||
           S_EQ(str, "const") ||
           S_EQ(str, "volatile") ||
           S_EQ(str, "extern") ||
           S_EQ(str, "static") ||
           S_EQ(str, "__ignore_typecheck") ||
           S_EQ(str, "return") ||
           S_EQ(str, "include") ||
           S_EQ(str, "if") ||
           S_EQ(str, "else") ||
           S_EQ(str, "while") ||
           S_EQ(str, "for") ||
           S_EQ(str, "do") ||
           S_EQ(str, "break") ||
           S_EQ(str, "continue") ||
           S_EQ(str, "switch") ||
           S_EQ(str, "case") ||
           S_EQ(str, "default") ||
           S_EQ(str, "goto") ||
           S_EQ(str, "auto") ||
           S_EQ(str, "register") ||
           S_EQ(str, "restrict") ||
           S_EQ(str, "inline") ||
           S_EQ(str, "virtual") ||
           S_EQ(str, "explicit") ||
           S_EQ(str, "friend") ||
           S_EQ(str, "constexpr") ||
           S_EQ(str, "mutable") ||
           S_EQ(str, "operator") ||
           S_EQ(str, "this") ||
           S_EQ(str, "sizeof") ||
           S_EQ(str, "alignof") ||
           S_EQ(str, "decltype") ||
           S_EQ(str, "nullptr") ||
           S_EQ(str, "true") ||
           S_EQ(str, "false") ||
           S_EQ(str, "bool");
}

/**
 * 读取一个新的表达式
 * @return
 */
static struct token *token_make_operator_or_string() {
    char op = peekc();
    if (op == '<') {
        struct token *last_token = lexer_last_token();
        if (token_is_keyword(last_token, "include")) {
            return token_make_string('<', '>');
        }
    }

    struct token *token = token_create(&(struct token) {
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


struct token *token_make_one_line_comment() {
    struct buffer *buffer = buffer_create();
    char c;
    LEX_GETC_IF(buffer, c, c != '\n' && c != EOF);
    buffer_write(buffer, 0x00);
    return token_create(&(struct token) {
            .type = TOKEN_TYPE_COMMENT,
            .sval = buffer_ptr(buffer)
    });
}


struct token *token_make_multiline_comment() {
    struct buffer *buffer = buffer_create();
    char c = 0;
    while (true) {
        LEX_GETC_IF(buffer, c, c != '*' && c != EOF);
        if (c == EOF) {
            compiler_error(lex_process->compiler, "You did not close this multiline comment.\n");
        } else if (c == '*') {
            // 跳过星号
            nextc();
            if (peekc() == '/') {
                nextc();
                break;
            }
        }
    }
    return token_create(&(struct token) {
            .type = TOKEN_TYPE_COMMENT,
            .sval = buffer_ptr(buffer)
    });
}


struct token *handle_comment() {
    char c = peekc();
    if (c == '/') {
        nextc();
        if (peekc() == '/') {
            return token_make_one_line_comment();
        } else if (peekc() == '*') {
            nextc();
            return token_make_multiline_comment();
        }
        pushc('/');
        return token_make_operator_or_string();
    }
    return NULL;
}

/**
 * 创建一个符号token结构体
 * @return 符号token结构体
 */
static struct token *token_make_symbol() {
    char c = nextc();
    if (c == ')') {
        lex_finish_expression();
    }
    struct token *token = token_create(&(struct token) {
            .type = TOKEN_TYPE_SYMBOL,
            .cval = c
    });
    return token;
}


static struct token *token_make_identifier_or_keyword() {
    struct buffer *buffer = buffer_create();
    char c;
    LEX_GETC_IF(buffer, c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
    buffer_write(buffer, 0x00);
    if (is_keyword(buffer_ptr(buffer))) {
        // 关键字检测
        return token_create(&(struct token) {
                .type = TOKEN_TYPE_KEYWORD,
                .sval = buffer_ptr(buffer)
        });
    }
    return token_create(&(struct token) {
            .type = TOKEN_TYPE_IDENTIFIER,
            .sval = buffer_ptr(buffer)
    });
}

/**
 * 检测标识符是否是以字母或_开头
 * @return
 */
struct token *read_special_token() {
    char c = peekc();
    if (isalpha(c) || c == '_') {
        return token_make_identifier_or_keyword();
    }
    return NULL;
}

/**
 * 创建一个换行符token结构体
 * @return
 */
struct token *token_make_newline() {
    nextc();
    return token_create(&(struct token) {
            .type = TOKEN_TYPE_NEWLINE
    });
}

char lex_get_escaped_char(char c) {
    char co = 0;
    switch (c) {
        case 'n':
            co = '\n';
            break;
        case 't':
            co = '\t';
            break;
        case 'r':
            co = '\r';
            break;
        case '\\':
            co = '\\';
            break;
        case '\'':
            co = '\'';
            break;
    }
    return co;
}

/**
 * 弹出一个token
 */
void lexer_pop_token() {
    vector_pop(lex_process->token_vec);
}


bool is_hex_char(char c) {
    c = tolower(c);
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

/**
 * 检测字符是否是十六进制字符，并返回这个字符串
 * @return
 */
const char *read_hex_number_str() {
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, is_hex_char(c));
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}


/**
 * 创建一个特殊的十六进制数字token结构体
 * @return
 */

struct token *token_make_special_number_hexadecimal() {
    // Skip x
    nextc();

    unsigned long number = 0;
    const char *number_str = read_hex_number_str();
    number = strtol(number_str, 0, 16);
    return token_make_number_for_value(number);
}


const char *read_bin_number_str() {
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, c == '0' || c == '1');
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

void lexer_validate_binary_string(const char *str) {
    size_t len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] != '0' && str[i] != '1') {
            compiler_error(lex_process->compiler, "Invalid binary string\n");
        }
    }
}


struct token *token_make_special_number_binary() {
    // Skip b
    nextc();

    unsigned long number = 0;
    const char *number_str = read_number_str();
    lexer_validate_binary_string(number_str);
    number = strtol(number_str, 0, 2);
    return token_make_number_for_value(number);
}


struct token *token_make_special_number() {
    struct token *token = NULL;
    struct token *last_token = lexer_last_token();
    if (!last_token || !(last_token->type == TOKEN_TYPE_NUMBER && last_token->llnum == 0)) {
        return token_make_identifier_or_keyword();
    }
    lexer_pop_token();
    char c = peekc();
    if (c == 'x') {
        token = token_make_special_number_hexadecimal();
    } else if (c == 'b') {
        token = token_make_special_number_binary();
    }
    return token;
}

/**
 * 创建一个引号token结构体
 * @return
 */
struct token *token_make_quote() {
    assert_next_char('\'');
    char c = nextc();
    if (c == '\\') {
        c = nextc();
        c = lex_get_escaped_char(c);
    }
    if (nextc() != '\'') {
        compiler_error(lex_process->compiler, "You opened a quote, but did not close it.\n");
    }

    return token_create(&(struct token) {
            .type = TOKEN_TYPE_NUMBER,
            .cval = c
    });
}

/**
 * 从文件中读取下一个token
 * @return 读取到的token
 */
struct token *read_next_token() {
    struct token *token = NULL;
    char c = peekc();
    token = handle_comment();
    if (token) {
        return token;
    }
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
        case 'b':
        case 'x':
            token = token_make_special_number();
            break;
        case '"':
            // 读取到了字符串开始
            token = token_make_string('"', '"');
            break;
        case '\'':
            token = token_make_quote();
            break;

        case ' ':
        case '\t':
            // 略过空格和制表符
            token = handle_whitespace();
            break;
        case '\n':
            token = token_make_newline();
            break;
        case EOF:
            //完成了词法分析
            break;

        default:
            token = read_special_token();
            if (!token) { compiler_error(lex_process->compiler, "Unexpected token\n"); }
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
    // 括号缓冲区
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token *token = read_next_token();
    while (token) {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }
    return LEXICAL_ANALYSIS_ALL_OK;
}

char lexer_string_buffer_next_char(struct lex_process *process) {
    struct buffer *buf = lex_process_private(process);
    return buffer_read(buf);
}

char lexer_string_buffer_peek_char(struct lex_process *process) {
    struct buffer *buf = lex_process_private(process);
    return buffer_peek(buf);
}

void lexer_string_buffer_push_char(struct lex_process *process, char c) {
    struct buffer *buf = lex_process_private(process);
    buffer_write(buf, c);
}

struct lex_process_functions lexer_string_buffer_functions = {
        .next_char = lexer_string_buffer_next_char,
        .peek_char = lexer_string_buffer_peek_char,
        .push_char = lexer_string_buffer_push_char
};


struct lex_process *token_build_for_string(struct compile_process *compiler, const char *str) {
    struct buffer *buffer = buffer_create();
    buffer_printf(buffer, str);
    struct lex_process *lex_process = lex_process_create(compiler, &lexer_string_buffer_functions, buffer);
    if (!lex_process) {
        return NULL;
    }
    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK) {
        return NULL;
    }
    return lex_process;
}