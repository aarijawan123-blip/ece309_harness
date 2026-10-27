#include "tools/calculator.h"

#include <stack>

using Kind = CalcToken::Kind;

static int precedence(Kind op) {
    if (op == Kind::Times || op == Kind::Divide) return 2;
    if (op == Kind::Plus || op == Kind::Minus) return 1;
    return 0;
}

static bool apply(std::stack<double>& values, std::stack<Kind>& ops,
                  std::string* error) {
    if (ops.empty() || values.size() < 2) {
        *error = "missing operand or operator";
        return false;
    }
    Kind op = ops.top();
    ops.pop();
    // The right operand is on top, which matters for subtraction and division.
    double right = values.top();
    values.pop();
    double left = values.top();
    values.pop();
    switch (op) {
        case Kind::Plus: values.push(left + right); break;
        case Kind::Minus: values.push(left - right); break;
        case Kind::Times: values.push(left * right); break;
        case Kind::Divide:
            if (right == 0.0) {
                *error = "division by zero";
                return false;
            }
            values.push(left / right);
            break;
        default:
            *error = "invalid operator or unmatched parenthesis";
            return false;
    }
    return true;
}

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out,
                   std::string* error) {
    std::stack<double> values;
    std::stack<Kind> ops;
    bool expect_operand = true;
    for (const auto& token : tokens) {
        Kind kind = token.kind;
        if (kind == Kind::Number && expect_operand) {
            values.push(token.value);
            expect_operand = false;
        } else if (kind == Kind::LParen && expect_operand) {
            ops.push(kind);
        } else if (kind == Kind::RParen && !expect_operand) {
            while (!ops.empty() && ops.top() != Kind::LParen) {
                if (!apply(values, ops, error)) return false;
            }
            if (ops.empty()) {
                *error = "unmatched closing parenthesis";
                return false;
            }
            ops.pop();
        } else if (precedence(kind) != 0 && !expect_operand) {
            // Equal precedence is applied now to keep evaluation left to right.
            while (!ops.empty() && precedence(ops.top()) >= precedence(kind)) {
                if (!apply(values, ops, error)) return false;
            }
            ops.push(kind);
            expect_operand = true;
        } else {
            *error = "unexpected token";
            return false;
        }
    }
    if (expect_operand) {
        *error = "missing operand";
        return false;
    }
    while (!ops.empty()) {
        if (!apply(values, ops, error)) return false;
    }
    if (values.size() != 1) {
        *error = "expected one result";
        return false;
    }
    *out = values.top();
    error->clear();
    return true;
}
