//---------------------------------------------------------------------------

#ifndef Unit2H
#define Unit2H
//---------------------------------------------------------------------------
#endif

// Unit2.h
#pragma once
#include <vector>
#include <System.hpp>

struct Token {
    enum class Type { Number, Name, Op, LParen, RParen, Comma, Assign, Func } type;
    String text;
    double value{}; // only used for Number
};

std::vector<Token> Tokenize(const String& input);
std::vector<Token> ToRPN(const std::vector<Token>& in);

// Evaluate a full expression string (tokenize -> RPN -> evaluate).
double EvalText(const String& input); // throws Exception on errors

// Optional helper if you want to clear variables later (CE-all).
void ClearCasVars();
