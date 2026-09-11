#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"

#include <gtest/gtest.h>

#include <sstream>

using namespace skript;

namespace {

std::string run(const std::string& source) {
    std::ostringstream output;
    runtime::Interpreter interpreter(output);
    interpreter.interpret(Parser(Lexer(source).tokenize()).parse());
    return output.str();
}

} // namespace

TEST(InterpreterTest, EvaluatesArithmeticAndAssignments) {
    EXPECT_EQ(run("x = 10 + 5 * 2\nprint(x)\n"), "20\n");
    EXPECT_EQ(run("print(7 / 2)\nprint(7 % 2)\n"), "3.5\n1\n");
}

TEST(InterpreterTest, ExecutesConditionalsAndLoops) {
    EXPECT_EQ(run("x = 0\nwhile x < 3:\n    x = x + 1\nif x == 3:\n    print(\"done\")\nelse:\n    print(\"wrong\")\n"), "done\n");
}

TEST(InterpreterTest, CallsRecursiveFunctions) {
    const std::string source =
        "def factorial(n):\n"
        "    if n <= 1:\n"
        "        return 1\n"
        "    return n * factorial(n - 1)\n"
        "print(factorial(5))\n";
    EXPECT_EQ(run(source), "120\n");
}

TEST(InterpreterTest, CapturesEnclosingScope) {
    const std::string source =
        "base = 4\n"
        "def add_base(value):\n"
        "    return value + base\n"
        "print(add_base(6))\n";
    EXPECT_EQ(run(source), "10\n");
}

TEST(InterpreterTest, ReportsRuntimeErrors) {
    try {
        static_cast<void>(run("print(missing)\n"));
        FAIL() << "Expected RuntimeError";
    } catch (const runtime::RuntimeError& error) {
        EXPECT_EQ(error.line(), 1U);
    }
}

TEST(InterpreterTest, CreatesAndPrintsLists) {
    EXPECT_EQ(run("items = [1, 2 + 3, \"six\"]\nprint(items)\n"), "[1, 5, six]\n");
}

TEST(InterpreterTest, RunsStandardLibraryBuiltins) {
    EXPECT_EQ(run("items = range(2, 7, 2)\nprint(items)\nprint(len(items))\nprint(type(items))\nprint(len(\"hello\"))\n"),
              "[2, 4, 6]\n3\nlist\n5\n");
}
