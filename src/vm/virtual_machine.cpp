#include "vm/virtual_machine.hpp"
#include "runtime/builtins.hpp"

#include <ostream>
#include <utility>

namespace skript::vm {
namespace {

bool equal(const runtime::Value& left, const runtime::Value& right) {
    if (left.is_number() && right.is_number()) return left.as_number() == right.as_number();
    if (left.type() != right.type()) return false;
    switch (left.type()) {
    case runtime::Value::Type::None: return true;
    case runtime::Value::Type::String: return left.as_string() == right.as_string();
    case runtime::Value::Type::Boolean: return left.as_boolean() == right.as_boolean();
    case runtime::Value::Type::Function: return left.as_function() == right.as_function();
    case runtime::Value::Type::List: return left.as_list() == right.as_list();
    case runtime::Value::Type::Integer:
    case runtime::Value::Type::Float: return left.as_number() == right.as_number();
    }
    return false;
}

} // namespace

VmError::VmError(std::string message, const std::size_t line, const std::size_t column)
    : std::runtime_error("VM error at " + std::to_string(line) + ":" + std::to_string(column) + ": " + message) {}

VirtualMachine::VirtualMachine(std::ostream& output) : output_(output) {}

void VirtualMachine::execute(const compiler::Chunk& chunk) {
    stack_.clear();
    frames_.clear();
    frames_.push_back({&chunk, 0, {}});
    while (!frames_.empty()) {
        auto& frame = frames_.back();
        if (frame.instruction >= frame.chunk->code.size()) {
            frames_.pop_back();
            continue;
        }
        const compiler::Instruction instruction = frame.chunk->code[frame.instruction++];
        switch (instruction.opcode) {
        case compiler::OpCode::LoadConstant: push(frame.chunk->constants.at(instruction.operand)); break;
        case compiler::OpCode::BuildList: {
            if (instruction.operand < 0 || stack_.size() < static_cast<std::size_t>(instruction.operand)) error(instruction, "stack underflow");
            auto list = std::make_shared<runtime::List>();
            list->elements.resize(static_cast<std::size_t>(instruction.operand));
            for (std::size_t index = list->elements.size(); index > 0; --index) list->elements[index - 1] = pop(instruction);
            push(runtime::Value(std::move(list)));
            break;
        }
        case compiler::OpCode::Builtin: {
            const int argument_count = instruction.operand & 0xFFFF;
            const auto builtin = static_cast<runtime::Builtin>(instruction.operand >> 16);
            if (argument_count < 0 || stack_.size() < static_cast<std::size_t>(argument_count)) error(instruction, "stack underflow");
            std::vector<runtime::Value> arguments(static_cast<std::size_t>(argument_count));
            for (std::size_t index = arguments.size(); index > 0; --index) arguments[index - 1] = pop(instruction);
            try {
                push(runtime::invoke_builtin(builtin, arguments));
            } catch (const std::runtime_error& runtime_error) {
                error(instruction, runtime_error.what());
            }
            break;
        }
        case compiler::OpCode::LoadVariable: push(variable(instruction)); break;
        case compiler::OpCode::StoreVariable: store(instruction); break;
        case compiler::OpCode::Add: case compiler::OpCode::Subtract: case compiler::OpCode::Multiply:
        case compiler::OpCode::Divide: case compiler::OpCode::Modulo: binary(instruction); break;
        case compiler::OpCode::Negate: {
            const auto value = pop(instruction);
            if (!value.is_number()) error(instruction, "unary '-' requires a number");
            push(value.type() == runtime::Value::Type::Integer ? runtime::Value(-value.as_integer()) : runtime::Value(-value.as_float()));
            break;
        }
        case compiler::OpCode::Compare: compare(instruction); break;
        case compiler::OpCode::Jump: frame.instruction = static_cast<std::size_t>(instruction.operand); break;
        case compiler::OpCode::JumpIfFalse:
            if (!pop(instruction).is_truthy()) frame.instruction = static_cast<std::size_t>(instruction.operand);
            break;
        case compiler::OpCode::Call: call(instruction); break;
        case compiler::OpCode::Return: {
            const auto result = pop(instruction);
            if (frames_.size() == 1) error(instruction, "'return' outside a function");
            frames_.pop_back();
            push(result);
            break;
        }
        case compiler::OpCode::Print: output_ << pop(instruction).to_string() << '\n'; break;
        case compiler::OpCode::Pop: static_cast<void>(pop(instruction)); break;
        }
    }
}

runtime::Value VirtualMachine::pop(const compiler::Instruction& instruction) {
    if (stack_.empty()) error(instruction, "stack underflow");
    const auto value = stack_.back();
    stack_.pop_back();
    return value;
}
void VirtualMachine::push(runtime::Value value) { stack_.push_back(std::move(value)); }

runtime::Value VirtualMachine::variable(const compiler::Instruction& instruction) const {
    const auto& name = frames_.back().chunk->constants.at(instruction.operand).as_string();
    const auto local = frames_.back().locals.find(name);
    if (local != frames_.back().locals.end()) return local->second;
    const auto global = globals_.find(name);
    if (global != globals_.end()) return global->second;
    error(instruction, "undefined variable '" + name + "'");
}

void VirtualMachine::store(const compiler::Instruction& instruction) {
    const auto& name = frames_.back().chunk->constants.at(instruction.operand).as_string();
    const auto value = pop(instruction);
    if (frames_.size() == 1) globals_[name] = value;
    else frames_.back().locals[name] = value;
}

void VirtualMachine::binary(const compiler::Instruction& instruction) {
    const auto right = pop(instruction);
    const auto left = pop(instruction);
    if (instruction.opcode == compiler::OpCode::Add && left.type() == runtime::Value::Type::String && right.type() == runtime::Value::Type::String) {
        push(runtime::Value(left.as_string() + right.as_string()));
        return;
    }
    if (!left.is_number() || !right.is_number()) error(instruction, "operator requires compatible numeric operands");
    const bool integers = left.type() == runtime::Value::Type::Integer && right.type() == runtime::Value::Type::Integer;
    switch (instruction.opcode) {
    case compiler::OpCode::Add: push(integers ? runtime::Value(left.as_integer() + right.as_integer()) : runtime::Value(left.as_number() + right.as_number())); break;
    case compiler::OpCode::Subtract: push(integers ? runtime::Value(left.as_integer() - right.as_integer()) : runtime::Value(left.as_number() - right.as_number())); break;
    case compiler::OpCode::Multiply: push(integers ? runtime::Value(left.as_integer() * right.as_integer()) : runtime::Value(left.as_number() * right.as_number())); break;
    case compiler::OpCode::Divide:
        if (right.as_number() == 0.0) error(instruction, "division by zero");
        push(runtime::Value(left.as_number() / right.as_number()));
        break;
    case compiler::OpCode::Modulo:
        if (!integers) error(instruction, "'%' requires integer operands");
        if (right.as_integer() == 0) error(instruction, "division by zero");
        push(runtime::Value(left.as_integer() % right.as_integer()));
        break;
    default: error(instruction, "invalid arithmetic instruction");
    }
}

void VirtualMachine::compare(const compiler::Instruction& instruction) {
    const auto right = pop(instruction);
    const auto left = pop(instruction);
    const auto operation = static_cast<TokenType>(instruction.operand);
    if (operation == TokenType::EqualEqual) { push(runtime::Value(equal(left, right))); return; }
    if (operation == TokenType::BangEqual) { push(runtime::Value(!equal(left, right))); return; }
    if (!left.is_number() || !right.is_number()) error(instruction, "comparison requires numeric operands");
    switch (operation) {
    case TokenType::Less: push(runtime::Value(left.as_number() < right.as_number())); break;
    case TokenType::LessEqual: push(runtime::Value(left.as_number() <= right.as_number())); break;
    case TokenType::Greater: push(runtime::Value(left.as_number() > right.as_number())); break;
    case TokenType::GreaterEqual: push(runtime::Value(left.as_number() >= right.as_number())); break;
    default: error(instruction, "invalid comparison instruction");
    }
}

void VirtualMachine::call(const compiler::Instruction& instruction) {
    if (instruction.operand < 0 || stack_.size() < static_cast<std::size_t>(instruction.operand) + 1) error(instruction, "stack underflow");
    std::vector<runtime::Value> arguments(static_cast<std::size_t>(instruction.operand));
    for (std::size_t index = arguments.size(); index > 0; --index) arguments[index - 1] = pop(instruction);
    const auto callee = pop(instruction);
    if (callee.type() != runtime::Value::Type::Function || !callee.as_function()->bytecode) error(instruction, "can only call compiled functions");
    const auto& function = callee.as_function();
    if (arguments.size() != function->parameters.size()) error(instruction, "incorrect number of function arguments");
    Frame frame{function->bytecode.get(), 0, {}};
    for (std::size_t index = 0; index < arguments.size(); ++index) frame.locals.emplace(function->parameters[index], arguments[index]);
    frames_.push_back(std::move(frame));
}

[[noreturn]] void VirtualMachine::error(const compiler::Instruction& instruction, const std::string& message) const {
    throw VmError(message, instruction.location.line, instruction.location.column);
}

} // namespace skript::vm
