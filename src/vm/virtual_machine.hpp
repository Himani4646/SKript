#pragma once

#include "compiler/bytecode.hpp"

#include <iosfwd>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace skript::vm {

class VmError : public std::runtime_error {
public:
    VmError(std::string message, std::size_t line, std::size_t column);
};

class VirtualMachine {
public:
    explicit VirtualMachine(std::ostream& output);

    void execute(const compiler::Chunk& chunk);

private:
    struct Frame {
        const compiler::Chunk* chunk;
        std::size_t instruction = 0;
        std::unordered_map<std::string, runtime::Value> locals;
    };

    [[nodiscard]] runtime::Value pop(const compiler::Instruction& instruction);
    void push(runtime::Value value);
    [[nodiscard]] runtime::Value variable(const compiler::Instruction& instruction) const;
    void store(const compiler::Instruction& instruction);
    void binary(const compiler::Instruction& instruction);
    void compare(const compiler::Instruction& instruction);
    void call(const compiler::Instruction& instruction);
    [[noreturn]] void error(const compiler::Instruction& instruction, const std::string& message) const;

    std::ostream& output_;
    std::vector<runtime::Value> stack_;
    std::vector<Frame> frames_;
    std::unordered_map<std::string, runtime::Value> globals_;
};

} // namespace skript::vm
