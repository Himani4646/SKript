#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace skript::ast { struct FunctionStmt; }
namespace skript::compiler { struct Chunk; }

namespace skript::runtime {

class Environment;
struct Function;
struct List;
using FunctionPtr = std::shared_ptr<Function>;
using ListPtr = std::shared_ptr<List>;

class Value {
public:
    enum class Type { None, Integer, Float, String, Boolean, Function, List };

    Value() = default;
    Value(std::int64_t value) : data_(value) {}
    Value(double value) : data_(value) {}
    Value(std::string value) : data_(std::move(value)) {}
    Value(const char* value) : data_(std::string(value)) {}
    Value(bool value) : data_(value) {}
    Value(FunctionPtr value) : data_(std::move(value)) {}
    Value(ListPtr value) : data_(std::move(value)) {}

    [[nodiscard]] Type type() const noexcept;
    [[nodiscard]] const std::string& as_string() const;
    [[nodiscard]] std::int64_t as_integer() const;
    [[nodiscard]] double as_float() const;
    [[nodiscard]] bool as_boolean() const;
    [[nodiscard]] const FunctionPtr& as_function() const;
    [[nodiscard]] const ListPtr& as_list() const;
    [[nodiscard]] bool is_number() const noexcept;
    [[nodiscard]] double as_number() const;
    [[nodiscard]] bool is_truthy() const;
    [[nodiscard]] std::string to_string() const;

private:
    std::variant<std::monostate, std::int64_t, double, std::string, bool, FunctionPtr, ListPtr> data_;
};

struct Function {
    const ast::FunctionStmt* declaration = nullptr;
    std::shared_ptr<Environment> closure;
    std::shared_ptr<const compiler::Chunk> bytecode;
    std::vector<std::string> parameters;
};

struct List {
    std::vector<Value> elements;
};

} // namespace skript::runtime
