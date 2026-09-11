#include "runtime/value.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace skript::runtime {

Value::Type Value::type() const noexcept {
    switch (data_.index()) {
    case 0: return Type::None;
    case 1: return Type::Integer;
    case 2: return Type::Float;
    case 3: return Type::String;
    case 4: return Type::Boolean;
    case 5: return Type::Function;
    default: return Type::List;
    }
}

const std::string& Value::as_string() const { return std::get<std::string>(data_); }
std::int64_t Value::as_integer() const { return std::get<std::int64_t>(data_); }
double Value::as_float() const { return std::get<double>(data_); }
bool Value::as_boolean() const { return std::get<bool>(data_); }
const FunctionPtr& Value::as_function() const { return std::get<FunctionPtr>(data_); }
const ListPtr& Value::as_list() const { return std::get<ListPtr>(data_); }
bool Value::is_number() const noexcept { return type() == Type::Integer || type() == Type::Float; }
double Value::as_number() const { return type() == Type::Integer ? static_cast<double>(as_integer()) : as_float(); }

bool Value::is_truthy() const {
    switch (type()) {
    case Type::None: return false;
    case Type::Boolean: return as_boolean();
    case Type::Integer: return as_integer() != 0;
    case Type::Float: return as_float() != 0.0;
    case Type::String: return !as_string().empty();
    case Type::Function: return true;
    case Type::List: return !as_list()->elements.empty();
    }
    return false;
}

std::string Value::to_string() const {
    switch (type()) {
    case Type::None: return "None";
    case Type::Integer: return std::to_string(as_integer());
    case Type::Float: {
        std::ostringstream output;
        output << std::setprecision(15) << as_float();
        return output.str();
    }
    case Type::String: return as_string();
    case Type::Boolean: return as_boolean() ? "True" : "False";
    case Type::Function: return "<function>";
    case Type::List: {
        std::string result = "[";
        for (std::size_t index = 0; index < as_list()->elements.size(); ++index) {
            if (index != 0) result += ", ";
            result += as_list()->elements[index].to_string();
        }
        return result + "]";
    }
    }
    throw std::logic_error("unknown value type");
}

} // namespace skript::runtime
