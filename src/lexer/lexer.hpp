#pragma once

#include "lexer/token.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace skript {

class LexerError : public std::runtime_error {
public:
    LexerError(std::string message, std::size_t line, std::size_t column);

    [[nodiscard]] std::size_t line() const noexcept { return line_; }
    [[nodiscard]] std::size_t column() const noexcept { return column_; }

private:
    std::size_t line_;
    std::size_t column_;
};

class Lexer {
public:
    explicit Lexer(std::string source);

    [[nodiscard]] std::vector<Token> tokenize();

private:
    [[nodiscard]] bool at_end() const noexcept;
    [[nodiscard]] char peek() const noexcept;
    [[nodiscard]] char peek_next() const noexcept;
    char advance() noexcept;
    bool match(char expected) noexcept;
    void scan_indentation();
    void skip_inline_whitespace();
    void skip_comment();
    void scan_identifier();
    void scan_number();
    void scan_string(char quote);
    void add_token(TokenType type);
    [[noreturn]] void error(const std::string& message) const;

    std::string source_;
    std::vector<Token> tokens_;
    std::size_t start_ = 0;
    std::size_t current_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    std::size_t token_line_ = 1;
    std::size_t token_column_ = 1;
    bool at_line_start_ = true;
    std::vector<std::size_t> indentation_{0};
};

} // namespace skript
