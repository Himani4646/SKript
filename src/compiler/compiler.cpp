#include "compiler/compiler.hpp"
#include "runtime/builtins.hpp"

#include <utility>

namespace skript::compiler {

CompileError::CompileError(std::string message, const std::size_t line, const std::size_t column)
    : std::runtime_error("Compile error at " + std::to_string(line) + ":" + std::to_string(column) +
                         ": " + message) {}

std::shared_ptr<Chunk> Compiler::compile(const ast::Program& program, const bool echo_expressions) {
    echo_expressions_ = echo_expressions;
    for (const auto& item : program.statements) statement(*item);
    return chunk_;
}

void Compiler::statement(const ast::Stmt& item) {
    if (const auto* expression_statement = dynamic_cast<const ast::ExpressionStmt*>(&item)) {
        expression(*expression_statement->expression);
        emit(echo_expressions_ ? OpCode::Print : OpCode::Pop, item.location);
    } else if (const auto* assignment = dynamic_cast<const ast::AssignmentStmt*>(&item)) {
        expression(*assignment->value);
        variable(OpCode::StoreVariable, assignment->name);
    } else if (const auto* print = dynamic_cast<const ast::PrintStmt*>(&item)) {
        expression(*print->expression);
        emit(OpCode::Print, item.location);
    } else if (const auto* conditional = dynamic_cast<const ast::IfStmt*>(&item)) {
        expression(*conditional->condition);
        const auto false_jump = emit_jump(OpCode::JumpIfFalse, conditional->location);
        block(*conditional->then_branch);
        if (conditional->else_branch) {
            const auto end_jump = emit_jump(OpCode::Jump, conditional->location);
            patch_jump(false_jump, chunk_->code.size());
            block(*conditional->else_branch);
            patch_jump(end_jump, chunk_->code.size());
        } else {
            patch_jump(false_jump, chunk_->code.size());
        }
    } else if (const auto* loop = dynamic_cast<const ast::WhileStmt*>(&item)) {
        const std::size_t loop_start = chunk_->code.size();
        expression(*loop->condition);
        const auto exit_jump = emit_jump(OpCode::JumpIfFalse, loop->location);
        block(*loop->body);
        emit(OpCode::Jump, loop->location, static_cast<int>(loop_start));
        patch_jump(exit_jump, chunk_->code.size());
    } else if (const auto* function = dynamic_cast<const ast::FunctionStmt*>(&item)) {
        auto callable = std::make_shared<runtime::Function>();
        callable->bytecode = compile_function(*function);
        for (const auto& parameter : function->parameters) callable->parameters.push_back(parameter.lexeme);
        emit(OpCode::LoadConstant, function->name, chunk_->add_constant(runtime::Value(std::move(callable))));
        variable(OpCode::StoreVariable, function->name);
    } else if (const auto* returned = dynamic_cast<const ast::ReturnStmt*>(&item)) {
        if (returned->value) expression(*returned->value);
        else emit(OpCode::LoadConstant, item.location, chunk_->add_constant(runtime::Value()));
        emit(OpCode::Return, item.location);
    } else if (const auto* suite = dynamic_cast<const ast::BlockStmt*>(&item)) {
        block(*suite);
    } else {
        error(item.location, "unsupported statement");
    }
}

void Compiler::block(const ast::BlockStmt& suite) {
    for (const auto& item : suite.statements) statement(*item);
}

void Compiler::expression(const ast::Expr& item) {
    if (const auto* value = dynamic_cast<const ast::LiteralExpr*>(&item)) {
        emit(OpCode::LoadConstant, item.location, chunk_->add_constant(literal(*value)));
    } else if (const auto* variable_expression = dynamic_cast<const ast::VariableExpr*>(&item)) {
        variable(OpCode::LoadVariable, variable_expression->name);
    } else if (const auto* list = dynamic_cast<const ast::ListExpr*>(&item)) {
        for (const auto& element : list->elements) expression(*element);
        emit(OpCode::BuildList, item.location, static_cast<int>(list->elements.size()));
    } else if (const auto* unary = dynamic_cast<const ast::UnaryExpr*>(&item)) {
        expression(*unary->right);
        if (unary->op.type == TokenType::Minus) emit(OpCode::Negate, unary->op);
    } else if (const auto* binary = dynamic_cast<const ast::BinaryExpr*>(&item)) {
        expression(*binary->left);
        expression(*binary->right);
        switch (binary->op.type) {
        case TokenType::Plus: emit(OpCode::Add, binary->op); break;
        case TokenType::Minus: emit(OpCode::Subtract, binary->op); break;
        case TokenType::Star: emit(OpCode::Multiply, binary->op); break;
        case TokenType::Slash: emit(OpCode::Divide, binary->op); break;
        case TokenType::Percent: emit(OpCode::Modulo, binary->op); break;
        default: emit(OpCode::Compare, binary->op, static_cast<int>(binary->op.type)); break;
        }
    } else if (const auto* call = dynamic_cast<const ast::CallExpr*>(&item)) {
        const auto* name = dynamic_cast<const ast::VariableExpr*>(call->callee.get());
        const auto builtin = name ? runtime::find_builtin(name->name.lexeme) : std::nullopt;
        if (!builtin) expression(*call->callee);
        for (const auto& argument : call->arguments) expression(*argument);
        if (builtin) {
            const int packed = (static_cast<int>(*builtin) << 16) | static_cast<int>(call->arguments.size());
            emit(OpCode::Builtin, item.location, packed);
        } else {
            emit(OpCode::Call, item.location, static_cast<int>(call->arguments.size()));
        }
    } else {
        error(item.location, "unsupported expression");
    }
}

void Compiler::emit(const OpCode opcode, const Token& location, const int operand) {
    chunk_->code.push_back({opcode, operand, location});
}
std::size_t Compiler::emit_jump(const OpCode opcode, const Token& location) {
    emit(opcode, location, -1);
    return chunk_->code.size() - 1;
}
void Compiler::patch_jump(const std::size_t instruction, const std::size_t destination) {
    chunk_->code[instruction].operand = static_cast<int>(destination);
}
void Compiler::variable(const OpCode opcode, const Token& token) {
    emit(opcode, token, chunk_->add_constant(runtime::Value(token.lexeme)));
}
runtime::Value Compiler::literal(const ast::LiteralExpr& expression) const {
    switch (expression.location.type) {
    case TokenType::Integer: return runtime::Value(static_cast<std::int64_t>(std::stoll(expression.value)));
    case TokenType::Float: return runtime::Value(std::stod(expression.value));
    case TokenType::String: return runtime::Value(expression.value);
    case TokenType::True: return runtime::Value(true);
    case TokenType::False: return runtime::Value(false);
    case TokenType::None: return runtime::Value();
    default: error(expression.location, "invalid literal");
    }
}
std::shared_ptr<Chunk> Compiler::compile_function(const ast::FunctionStmt& function) {
    Compiler nested;
    nested.block(*function.body);
    nested.emit(OpCode::LoadConstant, function.name, nested.chunk_->add_constant(runtime::Value()));
    nested.emit(OpCode::Return, function.name);
    return nested.chunk_;
}
[[noreturn]] void Compiler::error(const Token& token, const std::string& message) const {
    throw CompileError(message, token.line, token.column);
}

} // namespace skript::compiler
