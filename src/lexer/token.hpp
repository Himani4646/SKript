#pragma once

#include <cstddef>
#include <string>

namespace skript {

enum class TokenType {
    EndOfFile,
    Newline,
    Indent,
    Dedent,
    Identifier,
    Integer,
    Float,
    String,
    True,
    False,
    None,
    If,
    Else,
    While,
    Def,
    Return,
    Print,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    EqualEqual,
    BangEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Assign,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Comma,
    Colon,
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::size_t line;
    std::size_t column;
};

const char* token_type_name(TokenType type) noexcept;

} // namespace skript
