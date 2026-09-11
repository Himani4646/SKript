#include "runtime/environment.hpp"

#include <stdexcept>
#include <utility>

namespace skript::runtime {

Environment::Environment(std::shared_ptr<Environment> enclosing) : enclosing_(std::move(enclosing)) {}
void Environment::define(const std::string& name, Value value) { values_[name] = std::move(value); }

Value Environment::get(const std::string& name) const {
    const auto value = values_.find(name);
    if (value != values_.end()) return value->second;
    if (enclosing_) return enclosing_->get(name);
    throw std::runtime_error("undefined variable '" + name + "'");
}

void Environment::assign(const std::string& name, Value value) {
    const auto existing = values_.find(name);
    if (existing != values_.end()) {
        existing->second = std::move(value);
        return;
    }
    if (enclosing_) {
        enclosing_->assign(name, std::move(value));
        return;
    }
    values_[name] = std::move(value);
}

} // namespace skript::runtime
