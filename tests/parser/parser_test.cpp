#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <gtest/gtest.h>

using namespace skript;

TEST(ParserTest, AppliesArithmeticPrecedence) {
    const auto program = Parser(Lexer("result = 1 + 2 * 3\n").tokenize()).parse();
    ASSERT_EQ(program.statements.size(), 1U);
    const auto* assignment = dynamic_cast<const ast::AssignmentStmt*>(program.statements[0].get());
    ASSERT_NE(assignment, nullptr);
    const auto* plus = dynamic_cast<const ast::BinaryExpr*>(assignment->value.get());
    ASSERT_NE(plus, nullptr);
    EXPECT_EQ(plus->op.type, TokenType::Plus);
    const auto* multiply = dynamic_cast<const ast::BinaryExpr*>(plus->right.get());
    ASSERT_NE(multiply, nullptr);
    EXPECT_EQ(multiply->op.type, TokenType::Star);
}

TEST(ParserTest, ParsesFunctionWithReturnAndCall) {
    const auto program = Parser(Lexer("def add(a, b):\n    return a + b\nprint(add(2, 3))\n").tokenize()).parse();
    ASSERT_EQ(program.statements.size(), 2U);
    const auto* function = dynamic_cast<const ast::FunctionStmt*>(program.statements[0].get());
    ASSERT_NE(function, nullptr);
    EXPECT_EQ(function->name.lexeme, "add");
    ASSERT_EQ(function->parameters.size(), 2U);
    ASSERT_EQ(function->body->statements.size(), 1U);
    EXPECT_NE(dynamic_cast<const ast::ReturnStmt*>(function->body->statements[0].get()), nullptr);
    const auto* print = dynamic_cast<const ast::PrintStmt*>(program.statements[1].get());
    ASSERT_NE(print, nullptr);
    EXPECT_NE(dynamic_cast<const ast::CallExpr*>(print->expression.get()), nullptr);
}

TEST(ParserTest, ParsesIfElseAndWhileBlocks) {
    const auto program = Parser(Lexer("if x > 5:\n    print(\"large\")\nelse:\n    while x < 10:\n        x = x + 1\n").tokenize()).parse();
    ASSERT_EQ(program.statements.size(), 1U);
    const auto* conditional = dynamic_cast<const ast::IfStmt*>(program.statements[0].get());
    ASSERT_NE(conditional, nullptr);
    ASSERT_EQ(conditional->then_branch->statements.size(), 1U);
    ASSERT_NE(conditional->else_branch, nullptr);
    ASSERT_EQ(conditional->else_branch->statements.size(), 1U);
    EXPECT_NE(dynamic_cast<const ast::WhileStmt*>(conditional->else_branch->statements[0].get()), nullptr);
}

TEST(ParserTest, ReportsMissingColon) {
    try {
        static_cast<void>(Parser(Lexer("if x > 0\n    print(x)\n").tokenize()).parse());
        FAIL() << "Expected ParserError";
    } catch (const ParserError& error) {
        EXPECT_EQ(error.line(), 1U);
    }
}

TEST(ParserTest, ParsesListLiterals) {
    const auto program = Parser(Lexer("items = [1, 2 + 3]\n").tokenize()).parse();
    const auto* assignment = dynamic_cast<const ast::AssignmentStmt*>(program.statements[0].get());
    ASSERT_NE(assignment, nullptr);
    const auto* list = dynamic_cast<const ast::ListExpr*>(assignment->value.get());
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->elements.size(), 2U);
}
