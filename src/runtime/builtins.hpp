#pragma once

#include "runtime/value.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace skript::runtime {

enum class Builtin { Len, Type, Range };

[[nodiscard]] std::optional<Builtin> find_builtin(std::string_view name) noexcept;
[[nodiscard]] Value invoke_builtin(Builtin builtin, const std::vector<Value>& arguments);

} // namespace skript::runtime
