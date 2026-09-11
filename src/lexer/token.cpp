#include "lexer/token.hpp"

namespace skript {

const char* token_type_name(const TokenType type) noexcept {
    switch (type) {
    case TokenType::EndOfFile: return "EOF";
    case TokenType::Newline: return "NEWLINE";
    case TokenType::Indent: return "INDENT";
    case TokenType::Dedent: return "DEDENT";
    case TokenType::Identifier: return "IDENTIFIER";
    case TokenType::Integer: return "INTEGER";
    case TokenType::Float: return "FLOAT";
    case TokenType::String: return "STRING";
    case TokenType::True: return "TRUE";
    case TokenType::False: return "FALSE";
    case TokenType::None: return "NONE";
    case TokenType::If: return "IF";
    case TokenType::Else: return "ELSE";
    case TokenType::While: return "WHILE";
    case TokenType::Def: return "DEF";
    case TokenType::Return: return "RETURN";
    case TokenType::Print: return "PRINT";
    case TokenType::Plus: return "PLUS";
    case TokenType::Minus: return "MINUS";
    case TokenType::Star: return "STAR";
    case TokenType::Slash: return "SLASH";
    case TokenType::Percent: return "PERCENT";
    case TokenType::EqualEqual: return "EQUAL_EQUAL";
    case TokenType::BangEqual: return "BANG_EQUAL";
    case TokenType::Less: return "LESS";
    case TokenType::Greater: return "GREATER";
    case TokenType::LessEqual: return "LESS_EQUAL";
    case TokenType::GreaterEqual: return "GREATER_EQUAL";
    case TokenType::Assign: return "ASSIGN";
    case TokenType::LeftParen: return "LPAREN";
    case TokenType::RightParen: return "RPAREN";
    case TokenType::LeftBrace: return "LBRACE";
    case TokenType::RightBrace: return "RBRACE";
    case TokenType::LeftBracket: return "LBRACKET";
    case TokenType::RightBracket: return "RBRACKET";
    case TokenType::Comma: return "COMMA";
    case TokenType::Colon: return "COLON";
    }
    return "UNKNOWN";
}

} // namespace skript
