#include "runtime/interpreter.hpp"
#include "runtime/builtins.hpp"

#include <cmath>
#include <ostream>
#include <utility>

namespace skript::runtime {
namespace {

bool equal(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) return left.as_number() == right.as_number();
    if (left.type() != right.type()) return false;
    switch (left.type()) {
    case Value::Type::None: return true;
    case Value::Type::String: return left.as_string() == right.as_string();
    case Value::Type::Boolean: return left.as_boolean() == right.as_boolean();
    case Value::Type::Function: return left.as_function() == right.as_function();
    case Value::Type::List: return left.as_list() == right.as_list();
    case Value::Type::Integer:
    case Value::Type::Float: return left.as_number() == right.as_number();
    }
    return false;
}

} // namespace

class Interpreter::ReturnSignal {
public:
    explicit ReturnSignal(Value value) : value(std::move(value)) {}
    Value value;
};

RuntimeError::RuntimeError(std::string message, const std::size_t line, const std::size_t column)
    : std::runtime_error("Runtime error at " + std::to_string(line) + ":" + std::to_string(column) +
                         ": " + message), line_(line), column_(column) {}

Interpreter::Interpreter(std::ostream& output)
    : globals_(std::make_shared<Environment>()), environment_(globals_), output_(output) {}

void Interpreter::interpret(const ast::Program& program) {
    try {
        for (const auto& statement : program.statements) execute(*statement);
    } catch (const ReturnSignal&) {
        throw std::runtime_error("'return' outside a function");
    }
}

Value Interpreter::global(const std::string& name) const { return globals_->get(name); }

void Interpreter::execute(const ast::Stmt& statement) {
    if (const auto* expression = dynamic_cast<const ast::ExpressionStmt*>(&statement)) {
        static_cast<void>(evaluate(*expression->expression));
    } else if (const auto* assignment = dynamic_cast<const ast::AssignmentStmt*>(&statement)) {
        environment_->define(assignment->name.lexeme, evaluate(*assignment->value));
    } else if (const auto* print = dynamic_cast<const ast::PrintStmt*>(&statement)) {
        output_ << evaluate(*print->expression).to_string() << '\n';
    } else if (const auto* conditional = dynamic_cast<const ast::IfStmt*>(&statement)) {
        if (evaluate(*conditional->condition).is_truthy()) {
            for (const auto& child : conditional->then_branch->statements) execute(*child);
        } else if (conditional->else_branch) {
            for (const auto& child : conditional->else_branch->statements) execute(*child);
        }
    } else if (const auto* loop = dynamic_cast<const ast::WhileStmt*>(&statement)) {
        while (evaluate(*loop->condition).is_truthy()) {
            for (const auto& child : loop->body->statements) execute(*child);
        }
    } else if (const auto* function = dynamic_cast<const ast::FunctionStmt*>(&statement)) {
        auto callable = std::make_shared<Function>();
        callable->declaration = function;
        callable->closure = environment_;
        environment_->define(function->name.lexeme, Value(std::move(callable)));
    } else if (const auto* returned = dynamic_cast<const ast::ReturnStmt*>(&statement)) {
        throw ReturnSignal(returned->value ? evaluate(*returned->value) : Value());
    } else if (const auto* block = dynamic_cast<const ast::BlockStmt*>(&statement)) {
        for (const auto& child : block->statements) execute(*child);
    } else {
        error(statement.location, "unsupported statement");
    }
}

void Interpreter::execute_block(const ast::BlockStmt& block, std::shared_ptr<Environment> environment) {
    const auto previous = environment_;
    environment_ = std::move(environment);
    try {
        for (const auto& statement : block.statements) execute(*statement);
    } catch (...) {
        environment_ = previous;
        throw;
    }
    environment_ = previous;
}

Value Interpreter::evaluate(const ast::Expr& expression) {
    if (const auto* literal = dynamic_cast<const ast::LiteralExpr*>(&expression)) {
        switch (literal->location.type) {
        case TokenType::Integer: return Value(static_cast<std::int64_t>(std::stoll(literal->value)));
        case TokenType::Float: return Value(std::stod(literal->value));
        case TokenType::String: return Value(literal->value);
        case TokenType::True: return Value(true);
        case TokenType::False: return Value(false);
        case TokenType::None: return Value();
        default: error(literal->location, "invalid literal");
        }
    }
    if (const auto* variable = dynamic_cast<const ast::VariableExpr*>(&expression)) {
        try {
            return environment_->get(variable->name.lexeme);
        } catch (const std::runtime_error& runtime_error) {
            error(variable->name, runtime_error.what());
        }
    }
    if (const auto* list = dynamic_cast<const ast::ListExpr*>(&expression)) {
        auto object = std::make_shared<List>();
        object->elements.reserve(list->elements.size());
        for (const auto& element : list->elements) object->elements.push_back(evaluate(*element));
        return Value(std::move(object));
    }
    if (const auto* unary = dynamic_cast<const ast::UnaryExpr*>(&expression)) {
        const Value right = evaluate(*unary->right);
        if (!right.is_number()) error(unary->op, "unary operator requires a number");
        return unary->op.type == TokenType::Minus
            ? (right.type() == Value::Type::Integer ? Value(-right.as_integer()) : Value(-right.as_float()))
            : right;
    }
    if (const auto* binary = dynamic_cast<const ast::BinaryExpr*>(&expression)) {
        const Value left = evaluate(*binary->left);
        const Value right = evaluate(*binary->right);
        const Token& op = binary->op;
        if (op.type == TokenType::EqualEqual) return Value(equal(left, right));
        if (op.type == TokenType::BangEqual) return Value(!equal(left, right));
        if (op.type == TokenType::Plus && left.type() == Value::Type::String && right.type() == Value::Type::String) {
            return Value(left.as_string() + right.as_string());
        }
        if (!left.is_number() || !right.is_number()) error(op, "operator requires compatible numeric operands");
        const bool integers = left.type() == Value::Type::Integer && right.type() == Value::Type::Integer;
        if (op.type == TokenType::Plus) return integers ? Value(left.as_integer() + right.as_integer()) : Value(left.as_number() + right.as_number());
        if (op.type == TokenType::Minus) return integers ? Value(left.as_integer() - right.as_integer()) : Value(left.as_number() - right.as_number());
        if (op.type == TokenType::Star) return integers ? Value(left.as_integer() * right.as_integer()) : Value(left.as_number() * right.as_number());
        if (op.type == TokenType::Slash) {
            if (right.as_number() == 0.0) error(op, "division by zero");
            return Value(left.as_number() / right.as_number());
        }
        if (op.type == TokenType::Percent) {
            if (!integers) error(op, "'%' requires integer operands");
            if (right.as_integer() == 0) error(op, "division by zero");
            return Value(left.as_integer() % right.as_integer());
        }
        if (op.type == TokenType::Less) return Value(left.as_number() < right.as_number());
        if (op.type == TokenType::LessEqual) return Value(left.as_number() <= right.as_number());
        if (op.type == TokenType::Greater) return Value(left.as_number() > right.as_number());
        if (op.type == TokenType::GreaterEqual) return Value(left.as_number() >= right.as_number());
        error(op, "unsupported binary operator");
    }
    if (const auto* call_expression = dynamic_cast<const ast::CallExpr*>(&expression)) return call(*call_expression);
    error(expression.location, "unsupported expression");
}

Value Interpreter::call(const ast::CallExpr& expression) {
    if (const auto* name = dynamic_cast<const ast::VariableExpr*>(expression.callee.get())) {
        if (const auto builtin = find_builtin(name->name.lexeme)) {
            std::vector<Value> arguments;
            arguments.reserve(expression.arguments.size());
            for (const auto& argument : expression.arguments) arguments.push_back(evaluate(*argument));
            try {
                return invoke_builtin(*builtin, arguments);
            } catch (const std::runtime_error& runtime_error) {
                error(name->name, runtime_error.what());
            }
        }
    }
    const Value callee = evaluate(*expression.callee);
    if (callee.type() != Value::Type::Function) error(expression.location, "can only call functions");
    std::vector<Value> arguments;
    arguments.reserve(expression.arguments.size());
    for (const auto& argument : expression.arguments) arguments.push_back(evaluate(*argument));
    return call_function(callee.as_function(), arguments, expression.location);
}

Value Interpreter::call_function(const FunctionPtr& function, const std::vector<Value>& arguments, const Token& location) {
    if (arguments.size() != function->declaration->parameters.size()) error(location, "incorrect number of function arguments");
    auto local = std::make_shared<Environment>(function->closure);
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        local->define(function->declaration->parameters[index].lexeme, arguments[index]);
    }
    try {
        execute_block(*function->declaration->body, std::move(local));
    } catch (const ReturnSignal& returned) {
        return returned.value;
    }
    return Value();
}

[[noreturn]] void Interpreter::error(const Token& token, const std::string& message) const {
    throw RuntimeError(message, token.line, token.column);
}

} // namespace skript::runtime
