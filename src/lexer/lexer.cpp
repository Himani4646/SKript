#include "lexer/lexer.hpp"

#include <cctype>
#include <unordered_map>
#include <utility>

namespace skript {
namespace {

const std::unordered_map<std::string, TokenType> kKeywords = {
    {"if", TokenType::If},       {"else", TokenType::Else},
    {"while", TokenType::While}, {"def", TokenType::Def},
    {"return", TokenType::Return}, {"print", TokenType::Print},
    {"True", TokenType::True},   {"False", TokenType::False},
    {"None", TokenType::None},
};

bool is_alpha(const char character) {
    return std::isalpha(static_cast<unsigned char>(character)) != 0 || character == '_';
}

bool is_alphanumeric(const char character) {
    return is_alpha(character) || std::isdigit(static_cast<unsigned char>(character)) != 0;
}

} // namespace

LexerError::LexerError(std::string message, const std::size_t line, const std::size_t column)
    : std::runtime_error("Lexer error at " + std::to_string(line) + ":" +
                         std::to_string(column) + ": " + message),
      line_(line), column_(column) {}

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    while (!at_end()) {
        if (at_line_start_) {
            scan_indentation();
            if (at_end()) break;
            if (peek() == '\n') {
                advance();
                continue;
            }
            if (peek() == '#') {
                skip_comment();
                continue;
            }
            at_line_start_ = false;
        }

        skip_inline_whitespace();
        if (at_end()) break;
        if (peek() == '#') {
            skip_comment();
            continue;
        }

        start_ = current_;
        token_line_ = line_;
        token_column_ = column_;
        const char character = advance();

        if (character == '\n') {
            add_token(TokenType::Newline);
            at_line_start_ = true;
        } else if (is_alpha(character)) {
            scan_identifier();
        } else if (std::isdigit(static_cast<unsigned char>(character)) != 0) {
            scan_number();
        } else {
            switch (character) {
            case '(': add_token(TokenType::LeftParen); break;
            case ')': add_token(TokenType::RightParen); break;
            case '{': add_token(TokenType::LeftBrace); break;
            case '}': add_token(TokenType::RightBrace); break;
            case '[': add_token(TokenType::LeftBracket); break;
            case ']': add_token(TokenType::RightBracket); break;
            case ',': add_token(TokenType::Comma); break;
            case ':': add_token(TokenType::Colon); break;
            case '+': add_token(TokenType::Plus); break;
            case '-': add_token(TokenType::Minus); break;
            case '*': add_token(TokenType::Star); break;
            case '/': add_token(TokenType::Slash); break;
            case '%': add_token(TokenType::Percent); break;
            case '=': add_token(match('=') ? TokenType::EqualEqual : TokenType::Assign); break;
            case '!':
                if (!match('=')) error("expected '=' after '!'");
                add_token(TokenType::BangEqual);
                break;
            case '<': add_token(match('=') ? TokenType::LessEqual : TokenType::Less); break;
            case '>': add_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
            case '\'':
            case '"': scan_string(character); break;
            default: error(std::string("unexpected character '") + character + "'");
            }
        }
    }

    if (!tokens_.empty() && tokens_.back().type != TokenType::Newline) {
        tokens_.push_back({TokenType::Newline, "", line_, column_});
    }
    while (indentation_.size() > 1) {
        indentation_.pop_back();
        tokens_.push_back({TokenType::Dedent, "", line_, 1});
    }
    tokens_.push_back({TokenType::EndOfFile, "", line_, column_});
    return tokens_;
}

bool Lexer::at_end() const noexcept { return current_ >= source_.size(); }
char Lexer::peek() const noexcept { return at_end() ? '\0' : source_[current_]; }
char Lexer::peek_next() const noexcept {
    return current_ + 1 >= source_.size() ? '\0' : source_[current_ + 1];
}

char Lexer::advance() noexcept {
    const char character = source_[current_++];
    if (character == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return character;
}

bool Lexer::match(const char expected) noexcept {
    if (at_end() || source_[current_] != expected) return false;
    advance();
    return true;
}

void Lexer::scan_indentation() {
    std::size_t width = 0;
    while (!at_end() && (peek() == ' ' || peek() == '\t')) {
        if (peek() == '\t') error("tabs are not supported for indentation");
        advance();
        ++width;
    }
    if (at_end() || peek() == '\n' || peek() == '#') return;

    const std::size_t current_indent = indentation_.back();
    if (width > current_indent) {
        indentation_.push_back(width);
        tokens_.push_back({TokenType::Indent, "", line_, 1});
    } else if (width < current_indent) {
        while (indentation_.size() > 1 && width < indentation_.back()) {
            indentation_.pop_back();
            tokens_.push_back({TokenType::Dedent, "", line_, 1});
        }
        if (width != indentation_.back()) error("inconsistent indentation");
    }
}

void Lexer::skip_inline_whitespace() {
    while (!at_end() && (peek() == ' ' || peek() == '\t' || peek() == '\r')) advance();
}

void Lexer::skip_comment() {
    while (!at_end() && peek() != '\n') advance();
}

void Lexer::scan_identifier() {
    while (is_alphanumeric(peek())) advance();
    const std::string text = source_.substr(start_, current_ - start_);
    const auto keyword = kKeywords.find(text);
    add_token(keyword == kKeywords.end() ? TokenType::Identifier : keyword->second);
}

void Lexer::scan_number() {
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0) advance();
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next())) != 0) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek())) != 0) advance();
        add_token(TokenType::Float);
        return;
    }
    add_token(TokenType::Integer);
}

void Lexer::scan_string(const char quote) {
    std::string value;
    while (!at_end() && peek() != quote) {
        if (peek() == '\n') error("unterminated string");
        if (peek() == '\\') {
            advance();
            if (at_end()) error("unterminated string");
            switch (advance()) {
            case 'n': value += '\n'; break;
            case 't': value += '\t'; break;
            case '\\': value += '\\'; break;
            case '\'': value += '\''; break;
            case '"': value += '"'; break;
            default: error("unsupported escape sequence");
            }
        } else {
            value += advance();
        }
    }
    if (at_end()) error("unterminated string");
    advance();
    tokens_.push_back({TokenType::String, std::move(value), token_line_, token_column_});
}

void Lexer::add_token(const TokenType type) {
    tokens_.push_back({type, source_.substr(start_, current_ - start_), token_line_, token_column_});
}

[[noreturn]] void Lexer::error(const std::string& message) const {
    throw LexerError(message, token_line_, token_column_);
}

} // namespace skript
