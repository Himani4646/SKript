#pragma once

#include "lexer/token.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace skript::ast {

struct Expr {
    explicit Expr(Token location) : location(std::move(location)) {}
    virtual ~Expr() = default;
    Token location;
};

using ExprPtr = std::unique_ptr<Expr>;

struct LiteralExpr final : Expr {
    LiteralExpr(Token location, std::string value) : Expr(std::move(location)), value(std::move(value)) {}
    std::string value;
};

struct VariableExpr final : Expr {
    explicit VariableExpr(Token name) : Expr(name), name(std::move(name)) {}
    Token name;
};

struct UnaryExpr final : Expr {
    UnaryExpr(Token op, ExprPtr right) : Expr(op), op(std::move(op)), right(std::move(right)) {}
    Token op;
    ExprPtr right;
};

struct BinaryExpr final : Expr {
    BinaryExpr(ExprPtr left, Token op, ExprPtr right)
        : Expr(op), left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    ExprPtr left;
    Token op;
    ExprPtr right;
};

struct CallExpr final : Expr {
    CallExpr(ExprPtr callee, Token parenthesis, std::vector<ExprPtr> arguments)
        : Expr(parenthesis), callee(std::move(callee)), arguments(std::move(arguments)) {}
    ExprPtr callee;
    std::vector<ExprPtr> arguments;
};

struct ListExpr final : Expr {
    ListExpr(Token bracket, std::vector<ExprPtr> elements)
        : Expr(std::move(bracket)), elements(std::move(elements)) {}
    std::vector<ExprPtr> elements;
};

struct Stmt {
    explicit Stmt(Token location) : location(std::move(location)) {}
    virtual ~Stmt() = default;
    Token location;
};

using StmtPtr = std::unique_ptr<Stmt>;

struct ExpressionStmt final : Stmt {
    explicit ExpressionStmt(ExprPtr expression) : Stmt(expression->location), expression(std::move(expression)) {}
    ExprPtr expression;
};

struct AssignmentStmt final : Stmt {
    AssignmentStmt(Token name, ExprPtr value) : Stmt(name), name(std::move(name)), value(std::move(value)) {}
    Token name;
    ExprPtr value;
};

struct PrintStmt final : Stmt {
    PrintStmt(Token keyword, ExprPtr expression) : Stmt(keyword), expression(std::move(expression)) {}
    ExprPtr expression;
};

struct BlockStmt final : Stmt {
    BlockStmt(Token location, std::vector<StmtPtr> statements)
        : Stmt(std::move(location)), statements(std::move(statements)) {}
    std::vector<StmtPtr> statements;
};

struct IfStmt final : Stmt {
    IfStmt(Token keyword, ExprPtr condition, std::unique_ptr<BlockStmt> then_branch,
           std::unique_ptr<BlockStmt> else_branch)
        : Stmt(keyword), condition(std::move(condition)), then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}
    ExprPtr condition;
    std::unique_ptr<BlockStmt> then_branch;
    std::unique_ptr<BlockStmt> else_branch;
};

struct WhileStmt final : Stmt {
    WhileStmt(Token keyword, ExprPtr condition, std::unique_ptr<BlockStmt> body)
        : Stmt(keyword), condition(std::move(condition)), body(std::move(body)) {}
    ExprPtr condition;
    std::unique_ptr<BlockStmt> body;
};

struct FunctionStmt final : Stmt {
    FunctionStmt(Token name, std::vector<Token> parameters, std::unique_ptr<BlockStmt> body)
        : Stmt(name), name(std::move(name)), parameters(std::move(parameters)), body(std::move(body)) {}
    Token name;
    std::vector<Token> parameters;
    std::unique_ptr<BlockStmt> body;
};

struct ReturnStmt final : Stmt {
    ReturnStmt(Token keyword, ExprPtr value) : Stmt(keyword), value(std::move(value)) {}
    ExprPtr value;
};

struct Program {
    std::vector<StmtPtr> statements;
};

} // namespace skript::ast
