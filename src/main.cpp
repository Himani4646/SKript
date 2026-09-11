#include "compiler/compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "vm/virtual_machine.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

void execute_source(const std::string& source, skript::vm::VirtualMachine& machine, const bool echo_expressions) {
    const auto tokens = skript::Lexer(source).tokenize();
    const auto program = skript::Parser(tokens).parse();
    const auto bytecode = skript::compiler::Compiler().compile(program, echo_expressions);
    machine.execute(*bytecode);
}

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool starts_block(const std::string& line) {
    const auto comment = line.find('#');
    const auto code = trim(line.substr(0, comment));
    return !code.empty() && code.back() == ':';
}

int repl() {
    std::cout << "SKript 0.1 — type :quit to exit.\n";
    skript::vm::VirtualMachine machine(std::cout);
    std::string buffer;
    bool multiline = false;
    for (std::string line;;) {
        std::cout << (buffer.empty() ? ">>> " : "... ");
        if (!std::getline(std::cin, line)) break;
        if (buffer.empty() && trim(line) == ":quit") break;
        if (buffer.empty() && trim(line).empty()) continue;

        if (multiline && trim(line).empty()) {
            try {
                execute_source(buffer, machine, true);
            } catch (const std::exception& error) {
                std::cerr << error.what() << '\n';
            }
            buffer.clear();
            multiline = false;
            continue;
        }

        buffer += line + '\n';
        if (buffer.size() == line.size() + 1 && starts_block(line)) {
            multiline = true;
            continue;
        }
        if (!multiline) {
            try {
                execute_source(buffer, machine, true);
            } catch (const std::exception& error) {
                std::cerr << error.what() << '\n';
            }
            buffer.clear();
        }
    }
    if (!buffer.empty()) {
        try {
            execute_source(buffer, machine, true);
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
        }
    }
    return 0;
}

} // namespace

int main(const int argc, char* argv[]) {
    if (argc == 1) return repl();
    if (argc != 2) {
        std::cerr << "Usage: skript [program.skr]\n";
        return 64;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Could not open '" << argv[1] << "'.\n";
        return 66;
    }

    const std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    try {
        skript::vm::VirtualMachine machine(std::cout);
        execute_source(source, machine, false);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
