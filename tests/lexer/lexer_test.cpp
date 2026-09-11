#include "lexer/lexer.hpp"

#include <gtest/gtest.h>

using namespace skript;

namespace {

std::vector<Token> content_tokens(std::vector<Token> tokens) {
    std::vector<Token> result;
    for (auto& token : tokens) {
        if (token.type != TokenType::Newline && token.type != TokenType::Indent && token.type != TokenType::Dedent) {
            result.push_back(std::move(token));
        }
    }
    return result;
}

} // namespace

TEST(LexerTest, TokenizesAssignmentAndPrintCall) {
    const auto tokens = content_tokens(Lexer("x = 10 + 5\nprint(x)").tokenize());
    const std::vector<TokenType> types = {
        TokenType::Identifier, TokenType::Assign, TokenType::Integer, TokenType::Plus,
        TokenType::Integer, TokenType::Print, TokenType::LeftParen, TokenType::Identifier,
        TokenType::RightParen, TokenType::EndOfFile,
    };

    ASSERT_EQ(tokens.size(), types.size());
    for (std::size_t index = 0; index < types.size(); ++index) EXPECT_EQ(tokens[index].type, types[index]);
    EXPECT_EQ(tokens[0].lexeme, "x");
    EXPECT_EQ(tokens[2].lexeme, "10");
    EXPECT_EQ(tokens[5].line, 2U);
    EXPECT_EQ(tokens[5].column, 1U);
}

TEST(LexerTest, TokenizesKeywordsLiteralsAndComparisonOperators) {
    const auto tokens = content_tokens(Lexer("if True and False: x <= 3.14 # comment\nNone != \"hi\\n\"").tokenize());
    const std::vector<TokenType> types = {
        TokenType::If, TokenType::True, TokenType::Identifier, TokenType::False, TokenType::Colon,
        TokenType::Identifier, TokenType::LessEqual, TokenType::Float, TokenType::None,
        TokenType::BangEqual, TokenType::String, TokenType::EndOfFile,
    };

    ASSERT_EQ(tokens.size(), types.size());
    for (std::size_t index = 0; index < types.size(); ++index) EXPECT_EQ(tokens[index].type, types[index]);
    EXPECT_EQ(tokens[10].lexeme, "hi\n");
}

TEST(LexerTest, TokenizesEverySupportedSymbol) {
    const auto tokens = content_tokens(Lexer("() {} [],: +-*/% == != < > <= >= =").tokenize());
    const std::vector<TokenType> types = {
        TokenType::LeftParen, TokenType::RightParen, TokenType::LeftBrace, TokenType::RightBrace,
        TokenType::LeftBracket, TokenType::RightBracket, TokenType::Comma, TokenType::Colon, TokenType::Plus, TokenType::Minus, TokenType::Star,
        TokenType::Slash, TokenType::Percent, TokenType::EqualEqual, TokenType::BangEqual,
        TokenType::Less, TokenType::Greater, TokenType::LessEqual, TokenType::GreaterEqual,
        TokenType::Assign, TokenType::EndOfFile,
    };
    ASSERT_EQ(tokens.size(), types.size());
    for (std::size_t index = 0; index < types.size(); ++index) EXPECT_EQ(tokens[index].type, types[index]);
}

TEST(LexerTest, RejectsInvalidCharactersWithLocation) {
    try {
        static_cast<void>(Lexer("x = @").tokenize());
        FAIL() << "Expected LexerError";
    } catch (const LexerError& error) {
        EXPECT_EQ(error.line(), 1U);
        EXPECT_EQ(error.column(), 5U);
    }
}

TEST(LexerTest, RejectsUnterminatedString) {
    EXPECT_THROW(static_cast<void>(Lexer("print(\"oops)").tokenize()), LexerError);
}

TEST(LexerTest, EmitsIndentationTokensForBlocks) {
    const auto tokens = Lexer("if x:\n    print(x)\nprint(0)\n").tokenize();
    const std::vector<TokenType> types = {
        TokenType::If, TokenType::Identifier, TokenType::Colon, TokenType::Newline,
        TokenType::Indent, TokenType::Print, TokenType::LeftParen, TokenType::Identifier,
        TokenType::RightParen, TokenType::Newline, TokenType::Dedent, TokenType::Print,
        TokenType::LeftParen, TokenType::Integer, TokenType::RightParen, TokenType::Newline,
        TokenType::EndOfFile,
    };
    ASSERT_EQ(tokens.size(), types.size());
    for (std::size_t index = 0; index < types.size(); ++index) EXPECT_EQ(tokens[index].type, types[index]);
}
