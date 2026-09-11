#pragma once

#include "ast/ast.hpp"
#include "compiler/bytecode.hpp"

#include <memory>
#include <stdexcept>

namespace skript::compiler {

class CompileError : public std::runtime_error {
public:
    CompileError(std::string message, std::size_t line, std::size_t column);
};

class Compiler {
public:
    [[nodiscard]] std::shared_ptr<Chunk> compile(const ast::Program& program, bool echo_expressions = false);

private:
    void statement(const ast::Stmt& statement);
    void block(const ast::BlockStmt& block);
    void expression(const ast::Expr& expression);
    void emit(OpCode opcode, const Token& location, int operand = 0);
    [[nodiscard]] std::size_t emit_jump(OpCode opcode, const Token& location);
    void patch_jump(std::size_t instruction, std::size_t destination);
    void variable(OpCode opcode, const Token& token);
    [[nodiscard]] runtime::Value literal(const ast::LiteralExpr& expression) const;
    [[nodiscard]] std::shared_ptr<Chunk> compile_function(const ast::FunctionStmt& function);
    [[noreturn]] void error(const Token& token, const std::string& message) const;

    std::shared_ptr<Chunk> chunk_ = std::make_shared<Chunk>();
    bool echo_expressions_ = false;
};

} // namespace skript::compiler
