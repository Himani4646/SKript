#pragma once

#include "runtime/value.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace skript::runtime {

class Environment {
public:
    explicit Environment(std::shared_ptr<Environment> enclosing = nullptr);

    void define(const std::string& name, Value value);
    [[nodiscard]] Value get(const std::string& name) const;
    void assign(const std::string& name, Value value);

private:
    std::unordered_map<std::string, Value> values_;
    std::shared_ptr<Environment> enclosing_;
};

} // namespace skript::runtime
