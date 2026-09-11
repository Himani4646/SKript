#pragma once

#include "lexer/token.hpp"
#include "runtime/value.hpp"

#include <cstddef>
#include <vector>

namespace skript::compiler {

enum class OpCode {
    LoadConstant,
    BuildList,
    Builtin,
    LoadVariable,
    StoreVariable,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Negate,
    Compare,
    Jump,
    JumpIfFalse,
    Call,
    Return,
    Print,
    Pop,
};

struct Instruction {
    OpCode opcode;
    int operand = 0;
    Token location{TokenType::EndOfFile, "", 0, 0};
};

struct Chunk {
    std::vector<Instruction> code;
    std::vector<runtime::Value> constants;

    int add_constant(runtime::Value value);
};

} // namespace skript::compiler
