#include "parser/parser.hpp"

#include <utility>

namespace skript {

ParserError::ParserError(std::string message, const std::size_t line, const std::size_t column)
    : std::runtime_error("Syntax error at " + std::to_string(line) + ":" + std::to_string(column) +
                         ": " + message), line_(line), column_(column) {}

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

ast::Program Parser::parse() {
    ast::Program program;
    skip_newlines();
    while (!check(TokenType::EndOfFile)) {
        program.statements.push_back(statement());
        skip_newlines();
    }
    return program;
}

ast::StmtPtr Parser::statement() {
    if (match(TokenType::If)) return if_statement(previous());
    if (match(TokenType::While)) return while_statement(previous());
    if (match(TokenType::Def)) return function_statement(previous());
    if (match(TokenType::Return)) return return_statement(previous());
    if (match(TokenType::Print)) return print_statement(previous());
    if (check(TokenType::Identifier) && current_ + 1 < tokens_.size() && tokens_[current_ + 1].type == TokenType::Assign) {
        const Token name = advance();
        advance();
        auto value = expression();
        consume_line_end(name);
        return std::make_unique<ast::AssignmentStmt>(name, std::move(value));
    }
    auto value = expression();
    consume_line_end(value->location);
    return std::make_unique<ast::ExpressionStmt>(std::move(value));
}

ast::StmtPtr Parser::if_statement(Token keyword) {
    auto condition = expression();
    const Token& colon = consume(TokenType::Colon, "expected ':' after if condition");
    auto then_branch = suite(colon);
    std::unique_ptr<ast::BlockStmt> else_branch;
    if (match(TokenType::Else)) {
        const Token& else_colon = consume(TokenType::Colon, "expected ':' after else");
        else_branch = suite(else_colon);
    }
    return std::make_unique<ast::IfStmt>(std::move(keyword), std::move(condition), std::move(then_branch), std::move(else_branch));
}

ast::StmtPtr Parser::while_statement(Token keyword) {
    auto condition = expression();
    const Token& colon = consume(TokenType::Colon, "expected ':' after while condition");
    return std::make_unique<ast::WhileStmt>(std::move(keyword), std::move(condition), suite(colon));
}

ast::StmtPtr Parser::function_statement(Token keyword) {
    static_cast<void>(keyword);
    const Token name = consume(TokenType::Identifier, "expected function name after 'def'");
    consume(TokenType::LeftParen, "expected '(' after function name");
    std::vector<Token> parameters;
    if (!check(TokenType::RightParen)) {
        do { parameters.push_back(consume(TokenType::Identifier, "expected parameter name")); } while (match(TokenType::Comma));
    }
    consume(TokenType::RightParen, "expected ')' after function parameters");
    const Token& colon = consume(TokenType::Colon, "expected ':' after function declaration");
    return std::make_unique<ast::FunctionStmt>(std::move(name), std::move(parameters), suite(colon));
}

ast::StmtPtr Parser::return_statement(Token keyword) {
    ast::ExprPtr value;
    if (!check(TokenType::Newline) && !check(TokenType::Dedent) && !check(TokenType::EndOfFile)) value = expression();
    consume_line_end(keyword);
    return std::make_unique<ast::ReturnStmt>(std::move(keyword), std::move(value));
}

ast::StmtPtr Parser::print_statement(Token keyword) {
    consume(TokenType::LeftParen, "expected '(' after 'print'");
    auto value = expression();
    consume(TokenType::RightParen, "expected ')' after print argument");
    consume_line_end(keyword);
    return std::make_unique<ast::PrintStmt>(std::move(keyword), std::move(value));
}

std::unique_ptr<ast::BlockStmt> Parser::suite(const Token& opener) {
    consume(TokenType::Newline, "expected newline after ':'");
    consume(TokenType::Indent, "expected an indented block");
    std::vector<ast::StmtPtr> statements;
    skip_newlines();
    while (!check(TokenType::Dedent) && !check(TokenType::EndOfFile)) {
        statements.push_back(statement());
        skip_newlines();
    }
    consume(TokenType::Dedent, "expected end of indented block");
    return std::make_unique<ast::BlockStmt>(opener, std::move(statements));
}

ast::ExprPtr Parser::expression() { return equality(); }
ast::ExprPtr Parser::equality() {
    auto expr = comparison();
    while (match(TokenType::EqualEqual) || match(TokenType::BangEqual)) {
        const Token op = previous();
        expr = std::make_unique<ast::BinaryExpr>(std::move(expr), op, comparison());
    }
    return expr;
}
ast::ExprPtr Parser::comparison() {
    auto expr = term();
    while (match(TokenType::Less) || match(TokenType::LessEqual) || match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
        const Token op = previous();
        expr = std::make_unique<ast::BinaryExpr>(std::move(expr), op, term());
    }
    return expr;
}
ast::ExprPtr Parser::term() {
    auto expr = factor();
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        const Token op = previous();
        expr = std::make_unique<ast::BinaryExpr>(std::move(expr), op, factor());
    }
    return expr;
}
ast::ExprPtr Parser::factor() {
    auto expr = unary();
    while (match(TokenType::Star) || match(TokenType::Slash) || match(TokenType::Percent)) {
        const Token op = previous();
        expr = std::make_unique<ast::BinaryExpr>(std::move(expr), op, unary());
    }
    return expr;
}
ast::ExprPtr Parser::unary() {
    if (match(TokenType::Minus) || match(TokenType::Plus)) {
        const Token op = previous();
        return std::make_unique<ast::UnaryExpr>(op, unary());
    }
    return call();
}
ast::ExprPtr Parser::call() {
    auto expr = primary();
    while (match(TokenType::LeftParen)) {
        const Token parenthesis = previous();
        std::vector<ast::ExprPtr> arguments;
        if (!check(TokenType::RightParen)) {
            do { arguments.push_back(expression()); } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "expected ')' after call arguments");
        expr = std::make_unique<ast::CallExpr>(std::move(expr), parenthesis, std::move(arguments));
    }
    return expr;
}
ast::ExprPtr Parser::primary() {
    if (match(TokenType::Integer) || match(TokenType::Float) || match(TokenType::String) ||
        match(TokenType::True) || match(TokenType::False) || match(TokenType::None)) {
        const Token token = previous();
        return std::make_unique<ast::LiteralExpr>(token, token.lexeme);
    }
    if (match(TokenType::Identifier)) return std::make_unique<ast::VariableExpr>(previous());
    if (match(TokenType::LeftBracket)) {
        const Token bracket = previous();
        std::vector<ast::ExprPtr> elements;
        if (!check(TokenType::RightBracket)) {
            do { elements.push_back(expression()); } while (match(TokenType::Comma));
        }
        consume(TokenType::RightBracket, "expected ']' after list elements");
        return std::make_unique<ast::ListExpr>(bracket, std::move(elements));
    }
    if (match(TokenType::LeftParen)) {
        auto expr = expression();
        consume(TokenType::RightParen, "expected ')' after expression");
        return expr;
    }
    error(current(), "expected expression");
}

void Parser::consume_line_end(const Token& statement_start) {
    if (!match(TokenType::Newline) && !check(TokenType::Dedent) && !check(TokenType::EndOfFile)) {
        error(current(), "expected newline after statement");
    }
    static_cast<void>(statement_start);
}
void Parser::skip_newlines() { while (match(TokenType::Newline)) {} }
bool Parser::match(const TokenType type) { if (!check(type)) return false; advance(); return true; }
bool Parser::check(const TokenType type) const noexcept { return current().type == type; }
const Token& Parser::advance() noexcept { if (!check(TokenType::EndOfFile)) ++current_; return previous(); }
const Token& Parser::previous() const noexcept { return tokens_[current_ - 1]; }
const Token& Parser::current() const noexcept { return tokens_[current_]; }
const Token& Parser::consume(const TokenType type, const char* message) { if (check(type)) return advance(); error(current(), message); }
[[noreturn]] void Parser::error(const Token& token, const char* message) const { throw ParserError(message, token.line, token.column); }

} // namespace skript
