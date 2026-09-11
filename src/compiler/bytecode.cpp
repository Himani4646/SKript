#include "compiler/bytecode.hpp"

#include <utility>

namespace skript::compiler {

int Chunk::add_constant(runtime::Value value) {
    constants.push_back(std::move(value));
    return static_cast<int>(constants.size() - 1);
}

} // namespace skript::compiler
