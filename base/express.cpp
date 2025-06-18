#include <stack>
#include "express.h"

bool Expression::GetDoubleParameter(double& out, std::string str)
{
    size_t notNumberPos = str.find_first_not_of("0123456789.e+-");
    std::string strBase = str.substr(0, notNumberPos);
    std::string unit = notNumberPos == std::string::npos ? "" : str.substr(notNumberPos);

    double base;
    try {
        base = stod(strBase);
    } catch (const std::invalid_argument& e) {
        return false;
    }

    auto it = parameterUnitTable.find(unit);
    if (it == parameterUnitTable.end()) {
        return false;
    }

    out = base * it->second;
    return true;
}

bool Expression::isOperate(std::string& str, size_t index)
{
    // Split时判断是否为操作符
    char ch = str[index];
    if (ch == '(' || ch == ')' || ch == '*' || ch == '/') {
        return true;
    }
    if (ch == '+' || ch == '-') {
        if (index == 0) {
            // 在开头则为±号
            return false;
        }
        char preCh = str[index - 1];
        if (preCh == '(') {
            // 前一个为'('或操作符则为±号
            return false;
        }
        // 数e±数 则为±号
        if (index >= 2 &&
            (str[index - 2] >= '0' && str[index - 2] <= '9') &&
            (str[index - 1] == 'e' || str[index - 1] == 'E') &&
            index + 1 < str.size() &&
            (str[index + 1] >= '0' && str[index + 1] <= '9')) {
            // std::cout << std::endl;
            // std::cout << "index=" << index << std::endl;
            // std::cout << "str[index-2]=" << str[index - 2] << std::endl;
            // std::cout << "str[index-1]=" << str[index - 1] << std::endl;
            // std::cout << "str[index+2]=" << str[index+1] << std::endl;
            return false;
        }
        return true;
    }
    return false;
}

std::variant<double, std::string> Expression::CalculateExpression(const std::string& expression)
{
    std::stack<double> value;
    std::stack<char> operate;
    std::map<std::string, std::string>::iterator it;
    double value1, value2;

    for (ExpressionPartition element : _vectorExpression) {
        if (const double* valuePtr = std::get_if<double>(&element)) {
            value.push(*valuePtr);
        } else if (const char* opPtr = std::get_if<char>(&element)) {
            if (*opPtr == '(') {
                operate.push('(');
                continue;
            } else if (*opPtr == '*' || *opPtr == '/') {
                operate.push(*opPtr);
                continue;
            } else if (*opPtr == '+' || *opPtr == '-') {
                operate.push(*opPtr);
                continue;
            }

            if (*opPtr == ')') {
                while (!operate.empty() && operate.top() != '(') {
                    value2 = value.top();
                    value.pop();
                    value1 = value.top();
                    value.pop();

                    if (operate.top() == '+') {
                        value.push(value1 + value2);
                    } else if (operate.top() == '-') {
                        value.push(value1 - value2);
                    }
                    operate.pop();
                }

                if (!operate.empty()) {
                    operate.pop(); // 去掉栈顶(
                } else {
                    return expression; // 没有对应的(
                }
            } else {
                return expression;
            }
        }

        if (operate.empty() || operate.top() == '(') {
            // 栈为空或者栈顶为左括号
            continue;
        }

        while (!operate.empty() && (operate.top() == '*' || operate.top() == '/')) {
            if (value.size() < 2) {
                return expression;
            }

            value2 = value.top();
            value.pop();
            value1 = value.top();
            value.pop();

            if (operate.top() == '*') {
                value.push(value1 * value2);
            } else if (operate.top() == '/') {
                value.push(value1 / value2);
            } else {
                return expression;
            }

            operate.pop();
        }
    }

    if (!operate.empty()) {
        return expression;
    }

    if (value.size() == 1) {
        return value.top();
    }

    return expression;
}

bool Expression::SplitExpressionToElements(std::string str)
{
    _vectorExpression.clear();
    if (str.empty()) {
        return true;
    }

    std::string numStr; // string which means number
    double value;

    // add() on the outermost layer for convenient calculation
    _vectorExpression.emplace_back('(');

    auto CheckParam = [&]() {
        if (!numStr.empty()) {
            auto itLocal = _paramLocal.find(numStr);
            bool getNum = false;

            if (itLocal != _paramLocal.end()) {
                // 局部param
                const PARAMETER_VALUE& parameterValue = itLocal->second;
                if (const double* dPtr = std::get_if<double>(&parameterValue)) {
                    _vectorExpression.emplace_back(*dPtr);
                    getNum = true;
                } else if (const std::string* strPtr = std::get_if<std::string>(&parameterValue)) {
                    numStr = *strPtr;
                }
            }

            if (!getNum) {
                auto itGlobal = _paramGlobal.find(numStr);
                if (itGlobal != _paramGlobal.end()) {
                    // 全局param
                    const PARAMETER_VALUE& parameterValue = itGlobal->second;
                    if (const double* dPtr = std::get_if<double>(&parameterValue)) {
                        _vectorExpression.emplace_back(*dPtr);
                        getNum = true;
                    } else if (const std::string* strPtr = std::get_if<std::string>(&parameterValue)) {
                        numStr = *strPtr;
                    }
                }
            }

            if (!getNum) {
                if (!GetDoubleParameter(value, numStr)) {
                    return false;
                } else {
                    _vectorExpression.emplace_back(value);
                }
            }

            numStr = "";
        }
        return true;
    };

    int length = str.size();
    for (int i = 0; i < length; ++i) {
        char ch = str[i];
        if (isOperate(str, i)) {
            if (!CheckParam()) {
                return false;
            }
            _vectorExpression.emplace_back(ch);
        } else {
            numStr += ch;
        }
    }

    if (!CheckParam()) {
        // 最后一个字符
        return false;
    }

    // 最外层加上括号
    _vectorExpression.push_back(')');

    return true;
}

PARAMETER_VALUE Expression::SolveExpression(
    const std::string& expression,
    const std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& dictParamLocal,
    const std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& dictParamGlobal)
{
    _paramLocal = dictParamLocal;
    _paramGlobal = dictParamGlobal;

    if (!SplitExpressionToElements(expression)) {
        // if can't devide
        return expression; // it is better to use string operation, but now return expression without calculate.
    }

    PARAMETER_VALUE result = CalculateExpression(expression);

    if (const double* dPtr = std::get_if<double>(&result)) {
        return *dPtr;
    }

    const std::string* strPtr = std::get_if<std::string>(&result);
    return *strPtr;
}

#include <iostream>

void TestParseParameter()
{
    std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> mpLocal;
    mpLocal["a"] = "5.5k";
    mpLocal["b"] = "2";
    mpLocal["c"] = "3.5k";

    std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> mpGlobal;
    mpGlobal["a"] = "4.5k";
    mpGlobal["d"] = "1500";
    mpGlobal["e"] = "1.2";

    Expression exp;
    PARAMETER_VALUE result;

    std::string expression1 = "3e-008";
    result = exp.SolveExpression(expression1, mpLocal, mpGlobal);
    std::cout << expression1 << std::endl;
    std::cout << std::get<double>(result) << std::endl;
    std::cout << std::endl << "over" << std::endl;

    std::string expression2 = "((a-1k)*b-(c+d*1.2))+1.2e3";
    result = exp.SolveExpression(expression2, mpLocal, mpGlobal);
    std::cout << expression2 << std::endl;
    std::cout << std::get<double>(result) << std::endl;
    std::cout << std::endl << "over" << std::endl;
}
