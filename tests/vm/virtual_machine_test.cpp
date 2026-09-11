#include "compiler/compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "vm/virtual_machine.hpp"

#include <gtest/gtest.h>

#include <sstream>

using namespace skript;

namespace {

std::string run(const std::string& source) {
    const auto program = Parser(Lexer(source).tokenize()).parse();
    const auto bytecode = compiler::Compiler().compile(program);
    std::ostringstream output;
    vm::VirtualMachine machine(output);
    machine.execute(*bytecode);
    return output.str();
}

} // namespace

TEST(VirtualMachineTest, RunsArithmeticBytecode) {
    EXPECT_EQ(run("x = 5 + 3\nprint(x)\n"), "8\n");
}

TEST(VirtualMachineTest, RunsBranchesAndLoops) {
    const std::string source =
        "x = 0\n"
        "while x < 3:\n"
        "    x = x + 1\n"
        "if x == 3:\n"
        "    print(\"done\")\n"
        "else:\n"
        "    print(\"wrong\")\n";
    EXPECT_EQ(run(source), "done\n");
}

TEST(VirtualMachineTest, RunsRecursiveFunctions) {
    const std::string source =
        "def factorial(n):\n"
        "    if n <= 1:\n"
        "        return 1\n"
        "    return n * factorial(n - 1)\n"
        "print(factorial(5))\n";
    EXPECT_EQ(run(source), "120\n");
}

TEST(VirtualMachineTest, RejectsUnknownVariables) {
    const auto program = Parser(Lexer("print(missing)\n").tokenize()).parse();
    const auto bytecode = compiler::Compiler().compile(program);
    std::ostringstream output;
    vm::VirtualMachine machine(output);
    EXPECT_THROW(machine.execute(*bytecode), vm::VmError);
}

TEST(VirtualMachineTest, BuildsListObjects) {
    EXPECT_EQ(run("items = [1, 2 + 3, \"six\"]\nprint(items)\n"), "[1, 5, six]\n");
}

TEST(VirtualMachineTest, RunsStandardLibraryBuiltins) {
    EXPECT_EQ(run("items = range(2, 7, 2)\nprint(items)\nprint(len(items))\nprint(type(items))\nprint(len(\"hello\"))\n"),
              "[2, 4, 6]\n3\nlist\n5\n");
}

TEST(VirtualMachineTest, EchoesTopLevelExpressionsForTheRepl) {
    const auto program = Parser(Lexer("2 + 3\n").tokenize()).parse();
    const auto bytecode = compiler::Compiler().compile(program, true);
    std::ostringstream output;
    vm::VirtualMachine machine(output);
    machine.execute(*bytecode);
    EXPECT_EQ(output.str(), "5\n");
}
