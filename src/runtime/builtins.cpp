#include "runtime/builtins.hpp"

#include <stdexcept>

namespace skript::runtime {

std::optional<Builtin> find_builtin(const std::string_view name) noexcept {
    if (name == "len") return Builtin::Len;
    if (name == "type") return Builtin::Type;
    if (name == "range") return Builtin::Range;
    return std::nullopt;
}

Value invoke_builtin(const Builtin builtin, const std::vector<Value>& arguments) {
    switch (builtin) {
    case Builtin::Len:
        if (arguments.size() != 1) throw std::runtime_error("len() expects exactly one argument");
        if (arguments[0].type() == Value::Type::String) return Value(static_cast<std::int64_t>(arguments[0].as_string().size()));
        if (arguments[0].type() == Value::Type::List) return Value(static_cast<std::int64_t>(arguments[0].as_list()->elements.size()));
        throw std::runtime_error("len() expects a string or list");
    case Builtin::Type:
        if (arguments.size() != 1) throw std::runtime_error("type() expects exactly one argument");
        switch (arguments[0].type()) {
        case Value::Type::None: return Value("NoneType");
        case Value::Type::Integer: return Value("int");
        case Value::Type::Float: return Value("float");
        case Value::Type::String: return Value("str");
        case Value::Type::Boolean: return Value("bool");
        case Value::Type::Function: return Value("function");
        case Value::Type::List: return Value("list");
        }
        break;
    case Builtin::Range: {
        if (arguments.empty() || arguments.size() > 3) throw std::runtime_error("range() expects one to three arguments");
        for (const auto& argument : arguments) {
            if (argument.type() != Value::Type::Integer) throw std::runtime_error("range() expects integer arguments");
        }
        std::int64_t start = 0;
        std::int64_t stop = arguments[0].as_integer();
        std::int64_t step = 1;
        if (arguments.size() >= 2) {
            start = stop;
            stop = arguments[1].as_integer();
        }
        if (arguments.size() == 3) step = arguments[2].as_integer();
        if (step == 0) throw std::runtime_error("range() step cannot be zero");
        auto list = std::make_shared<List>();
        for (std::int64_t value = start; step > 0 ? value < stop : value > stop; value += step) {
            list->elements.emplace_back(value);
        }
        return Value(std::move(list));
    }
    }
    throw std::logic_error("unknown builtin");
}

} // namespace skript::runtime
