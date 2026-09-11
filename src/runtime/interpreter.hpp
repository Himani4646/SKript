#pragma once

#include "ast/ast.hpp"
#include "runtime/environment.hpp"

#include <iosfwd>
#include <memory>
#include <stdexcept>

namespace skript::runtime {

class RuntimeError : public std::runtime_error {
public:
    RuntimeError(std::string message, std::size_t line, std::size_t column);
    [[nodiscard]] std::size_t line() const noexcept { return line_; }
    [[nodiscard]] std::size_t column() const noexcept { return column_; }

private:
    std::size_t line_;
    std::size_t column_;
};

class Interpreter {
public:
    explicit Interpreter(std::ostream& output);

    void interpret(const ast::Program& program);
    [[nodiscard]] Value global(const std::string& name) const;

private:
    class ReturnSignal;

    void execute(const ast::Stmt& statement);
    void execute_block(const ast::BlockStmt& block, std::shared_ptr<Environment> environment);
    [[nodiscard]] Value evaluate(const ast::Expr& expression);
    [[nodiscard]] Value call(const ast::CallExpr& expression);
    [[nodiscard]] Value call_function(const FunctionPtr& function, const std::vector<Value>& arguments,
                                      const Token& location);
    [[noreturn]] void error(const Token& token, const std::string& message) const;

    std::shared_ptr<Environment> globals_;
    std::shared_ptr<Environment> environment_;
    std::ostream& output_;
};

} // namespace skript::runtime
