#pragma once

#include "ast/ast.hpp"

#include <stdexcept>
#include <vector>

namespace skript {

class ParserError : public std::runtime_error {
public:
    ParserError(std::string message, std::size_t line, std::size_t column);
    [[nodiscard]] std::size_t line() const noexcept { return line_; }
    [[nodiscard]] std::size_t column() const noexcept { return column_; }

private:
    std::size_t line_;
    std::size_t column_;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    [[nodiscard]] ast::Program parse();

private:
    [[nodiscard]] ast::StmtPtr statement();
    [[nodiscard]] ast::StmtPtr if_statement(Token keyword);
    [[nodiscard]] ast::StmtPtr while_statement(Token keyword);
    [[nodiscard]] ast::StmtPtr function_statement(Token keyword);
    [[nodiscard]] ast::StmtPtr return_statement(Token keyword);
    [[nodiscard]] ast::StmtPtr print_statement(Token keyword);
    [[nodiscard]] std::unique_ptr<ast::BlockStmt> suite(const Token& opener);
    [[nodiscard]] ast::ExprPtr expression();
    [[nodiscard]] ast::ExprPtr equality();
    [[nodiscard]] ast::ExprPtr comparison();
    [[nodiscard]] ast::ExprPtr term();
    [[nodiscard]] ast::ExprPtr factor();
    [[nodiscard]] ast::ExprPtr unary();
    [[nodiscard]] ast::ExprPtr call();
    [[nodiscard]] ast::ExprPtr primary();
    void consume_line_end(const Token& statement_start);
    void skip_newlines();
    [[nodiscard]] bool match(TokenType type);
    [[nodiscard]] bool check(TokenType type) const noexcept;
    const Token& advance() noexcept;
    [[nodiscard]] const Token& previous() const noexcept;
    [[nodiscard]] const Token& current() const noexcept;
    const Token& consume(TokenType type, const char* message);
    [[noreturn]] void error(const Token& token, const char* message) const;

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
};

} // namespace skript
