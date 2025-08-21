//---------------------------------------------------------------------------

#pragma hdrstop

#include "Unit2.h"
#include <cctype>
#include <stack>
#include <algorithm>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#include <stack>
#include <vector>
#include <map>
#include <limits>
#include <math.h>
















static std::map<String, double> gVars;                      // user variables (x, y, Ans, etc.)
static std::map<String, double> gConsts = {                 // constants (lowercase)
	{L"pi", M_PI},
	{L"e",  M_E}
};

static int precedence(const String& op) {
	if (op == L"NEG") return 4;
	if (op == L"^")   return 3;
	if (op == L"*")   return 2;
	if (op == L"/")   return 2;
	if (op == L"+")   return 1;
	if (op == L"-")   return 1;
	if (op == L"=")   return -1; // lowest
	return 0;
}

static bool isRightAssoc(const String& op) {
	return (op == L"^" || op == L"NEG");
}

// ---- small helpers ----
static double popNum(std::stack<double>& st) {
	if (st.empty()) throw Exception(L"Internal: stack underflow");
	double v = st.top(); st.pop();
	return v;
}

static double getVar(const String& name) {
	auto it = gVars.find(name);
	if (it != gVars.end()) return it->second;
	throw Exception(L"Unknown variable: " + name);
}

static double applyOp(const String& op, double a, double b) {
    if (op == L"+") return a + b;
    if (op == L"-") return a - b;
	if (op == L"*") return a * b;
    if (op == L"/") return a / b;
    if (op == L"^") return pow(a, b);
    throw Exception(L"Unknown operator: " + op);
}

static double applyFunc(const String& f, double a) {
    String lf = f.LowerCase();
    if (lf == L"sin")  return sin(a);
    if (lf == L"cos")  return cos(a);
    if (lf == L"tan")  return tan(a);
    if (lf == L"sqrt") return sqrt(a);
	if (lf == L"ln")   return log(a);
    if (lf == L"log")  return log10(a);
    if (lf == L"exp")  return exp(a);
	throw Exception(L"Unknown function: " + f);
}

// Evaluate RPN produced by ToRPN.
// Uses a second stack of names so we can distinguish lvalues (assignment) from rvalues.
static double EvaluateRPN(const std::vector<Token>& rpn) {
    std::stack<double> nums;
    std::stack<String> names; // parallel stack for when we push a Name

	auto resolveIfName = [&](double v) -> double {
        if (std::isnan(v)) {
            if (names.empty()) throw Exception(L"Name stack empty");
            String nm = names.top(); names.pop();
            // constants are pushed numerically, so reaching here means a variable reference
            return getVar(nm);
		}
        return v;
    };

    for (const auto& t : rpn) {
        switch (t.type) {
			case Token::Type::Number:
                nums.push(t.value);
                break;

            case Token::Type::Name: {
                // constants → push number; variables → push NaN + record name
				String lname = t.text.LowerCase();
                auto itc = gConsts.find(lname);
                if (itc != gConsts.end()) {
                    nums.push(itc->second);
                } else {
                    names.push(t.text); // keep original name casing
					nums.push(std::numeric_limits<double>::quiet_NaN());
                }
                break;
			}

            case Token::Type::Func: {
				double a = popNum(nums);
                a = resolveIfName(a);
                nums.push(applyFunc(t.text, a));
                break;
            }

			case Token::Type::Op: {
                if (t.text == L"NEG") {
                    double a = popNum(nums);
                    a = resolveIfName(a);
                    nums.push(-a);
                } else {
					double b = popNum(nums);
                    double a = popNum(nums);
                    b = resolveIfName(b);
					a = resolveIfName(a);
                    nums.push(applyOp(t.text, a, b));
                }
				break;
            }

            case Token::Type::Assign: {
                double right = popNum(nums);
                right = resolveIfName(right);

                double left = popNum(nums);
                if (!std::isnan(left) || names.empty())
                    throw Exception(L"Left side of '=' must be a variable name");

                String varName = names.top(); names.pop();
				gVars[varName] = right;
                // also store Ans as last result
                gVars[L"Ans"] = right;
                nums.push(right);
                break;
			}

            default:
                throw Exception(L"Bad token in RPN");
        }
    }

	if (nums.size() != 1) throw Exception(L"Malformed expression");
    double res = nums.top();
    if (std::isnan(res)) res = resolveIfName(res); // just in case
    // store Ans for convenience even for plain expressions
	gVars[L"Ans"] = res;
	return res;
}

double EvalText(const String& input) {
	auto toks = Tokenize(input);
	auto rpn  = ToRPN(toks);
    return EvaluateRPN(rpn);
}

void ClearCasVars() {
	gVars.clear();
}





// the start of the tokenize funcion/method
std::vector<Token> Tokenize(const String& input) {
// sets tokens into vector
	std::vector<Token> toks;
	String s = input.Trim();
	int i = 1; // VCL String is 1-based

	auto isAlpha = [](wchar_t c){ return std::iswalpha(c) || c == L'_'; };
	auto isAlnum = [&](wchar_t c){ return std::iswalnum(c) || c == L'_'; };

	while (i <= s.Length()) {
		wchar_t c = s[i];
		if (std::iswspace(c)) { i++; continue; }

		// Number (digits + optional dot)
        if (std::iswdigit(c) || c == L'.') {
			int j = i; bool seenDot = (c == L'.');
			j++;
			while (j <= s.Length()) {
                wchar_t d = s[j];
                if (std::iswdigit(d)) { j++; continue; }
				if (d == L'.' && !seenDot) { seenDot = true; j++; continue; }
                break;
            }
            String piece = s.SubString(i, j - i);
			Token t; t.type = Token::Type::Number; t.text = piece; t.value = piece.ToDouble();
            toks.push_back(t);
            i = j;
            continue;
        }

        // Name (function/variable/constant)
        if (isAlpha(c)) {
            int j = i + 1;
            while (j <= s.Length() && isAlnum(s[j])) j++;
            String name = s.SubString(i, j - i);
			Token t; t.type = Token::Type::Name; t.text = name;
			toks.push_back(t);
			i = j;
            continue;
		}

        // Single-char tokens
        if (c == L'(') { toks.push_back({Token::Type::LParen, L"("}); i++; continue; }
		if (c == L')') { toks.push_back({Token::Type::RParen, L")"}); i++; continue; }
		if (c == L',') { toks.push_back({Token::Type::Comma,  L","}); i++; continue; }
		if (c == L'=') { toks.push_back({Token::Type::Assign, L"="}); i++; continue; }

		// Operators + - * / ^
		if (c == L'+' || c == L'-' || c == L'*' || c == L'/' || c == L'^') {
			String op; op += c;
			toks.push_back({Token::Type::Op, op});
			i++; continue;
		}

		throw Exception(L"Unexpected character: " + String(c));
	}
	return toks;
}



























// start of the toRPN method
std::vector<Token> ToRPN(const std::vector<Token>& in) {
	std::vector<Token> out;
	std::stack<Token> ops;

	auto isValueLike = [](const Token& t){
        return t.type == Token::Type::Number || t.type == Token::Type::Name || t.type == Token::Type::RParen;
    };

    for (size_t i = 0; i < in.size(); ++i) {
        const Token& t = in[i];

        if (t.type == Token::Type::Number || t.type == Token::Type::Name) {
			// Peek: if Name followed by '(' → it’s a function
			bool isFunc = false;
            if (t.type == Token::Type::Name && i + 1 < in.size() && in[i+1].type == Token::Type::LParen) {
                Token f = t; f.type = Token::Type::Func;
				ops.push(f);      // function goes to op stack
                continue;         // DO NOT emit now
            }
            out.push_back(t);     // plain number or variable name → out
        }
		else if (t.type == Token::Type::LParen) {
			ops.push(t);
        }
        else if (t.type == Token::Type::RParen) {
            // Pop until '('
            while (!ops.empty() && ops.top().type != Token::Type::LParen) {
				out.push_back(ops.top()); ops.pop();
			}
            if (ops.empty()) throw Exception(L"Mismatched parenthesis");
            ops.pop(); // pop '('

            // If a function is on top now, pop it to output
			if (!ops.empty() && ops.top().type == Token::Type::Func) {
                out.push_back(ops.top()); ops.pop();
			}
        }
        else if (t.type == Token::Type::Comma) {
            // Function argument separator: pop until '('
			while (!ops.empty() && ops.top().type != Token::Type::LParen) {
                out.push_back(ops.top()); ops.pop();
            }
			if (ops.empty()) throw Exception(L"Misplaced comma");
        }
        else if (t.type == Token::Type::Assign) {
			// Treat '=' as the lowest-precedence operator
            Token assign = t; assign.text = "=";
            while (!ops.empty() && ops.top().type == Token::Type::Op) {
                out.push_back(ops.top()); ops.pop();
			}
            ops.push(assign);
		}
        else if (t.type == Token::Type::Op) {
            // Decide if '-' is unary
            bool unaryMinus = false;
            if (t.text == "-") {
				if (i == 0) unaryMinus = true;
                else {
                    const Token& prev = in[i-1];
                    if (prev.type == Token::Type::Op ||
                        prev.type == Token::Type::LParen ||
						prev.type == Token::Type::Comma ||
						prev.type == Token::Type::Assign) {
						unaryMinus = true;
                    }
                }
            }

            Token op = t;
            if (unaryMinus) op.text = "NEG"; // special unary operator

			int p = precedence(op.text);
			bool rightAssoc = isRightAssoc(op.text);

			while (!ops.empty()) {
				const Token& top = ops.top();
				if (top.type != Token::Type::Op) break;

                int pt = precedence(top.text);
				bool topHigher =
                    (rightAssoc ? (p < pt) : (p <= pt));

                if (topHigher) { out.push_back(top); ops.pop(); }
                else break;
            }

            op.type = Token::Type::Op;
            ops.push(op);
		}
		else if (t.type == Token::Type::Func) {
            // Shouldn’t occur here (we convert Name→Func above)
			ops.push(t);
        }
        else {
            throw Exception(L"Unexpected token in shunting-yard");
        }
    }

    // Drain the stack
    while (!ops.empty()) {
        if (ops.top().type == Token::Type::LParen || ops.top().type == Token::Type::RParen)
            throw Exception(L"Mismatched parenthesis");
		out.push_back(ops.top()); ops.pop();
    }

    return out;
}
// ---- constants and variables ----

