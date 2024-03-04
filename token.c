//
// Description: 包含了token的相关信息和操作
// Created by kery on 2024/3/5.
//

#include "compiler.h"

/**
 * @brief 检测该token是否是一个关键字
 * @param token
 * @param keyword
 * @return
 */
bool token_is_keyword(struct token* token, const char* value)
{
    return token->type == TOKEN_TYPE_KEYWORD && S_EQ(token->sval, value);
}
