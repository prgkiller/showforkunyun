#pragma once

#include <vector>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include "../config/config.h"

typedef std::string PARAMETER_NAME;
typedef std::variant<double, std::string> PARAMETER_VALUE;

static const std::map<std::string, double> parameterUnitTable = {
    {"", 1}, // no Unit
    {"g", 1e9},
    {"k", 1e3},
    {"c", 1e-2},
    {"m", 1e-3},
    {"u", 1e-6},
    {"n", 1e-9},
    {"p", 1e-12},
    {"f", 1e-15},
    {"a", 1e-18},
    {"G", 1e9},
    {"K", 1e3},
    {"C", 1e-2},
    {"M", 1e-3},
    {"U", 1e-6},
    {"N", 1e-9},
    {"P", 1e-12},
    {"F", 1e-15},
    {"A", 1e-18},
};

struct StringCaseInsensitiveHash { // almost copy code
    std::size_t operator()(const std::string& key) const {
        const Config& config = Config::GetInstance();
        if (config.caseInsensitive) {
            return std::hash<std::string>()(key);
        }
        std::size_t hashval = 0;
        for (char c : key) {
            hashval ^= std::tolower(c) + 0x9e3779b9 + (hashval << 6) + (hashval >> 2);
        }
        return hashval;
    }
};

struct StringCaseInsensitiveEqual { // almost copy code
    bool operator()(const std::string& str1, const std::string& str2) const {
        const Config& config = Config::GetInstance();

        if (config.caseInsensitive) {
            return str1 == str2;
        }
        return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(),
            [](char aChar, char bChar) {
                return std::tolower(aChar) == std::tolower(bChar);
            });
    }
};

class Expression{
public:
    PARAMETER_VALUE SolveExpression(const std::string& expression,
        const std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& dictParamLocal,
        const std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual>& dictParamGlobal); // 计算表达式的最外层函数
private:
    bool isOperate(std::string& str, size_t index);
    bool GetDoubleParameter(double& out, std::string str); // 将字符串(可能含单位)转化成double
    PARAMETER_VALUE CalculateExpression(const std::string& expression);
    bool SplitExpressionToElements(std::string str); // 把字符串转化成各数据和运算符
private:
    typedef std::variant<char, double> ExpressionPartition;

    std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> _paramLocal; // 子电路区域的参数
    std::unordered_map<PARAMETER_NAME, std::variant<double, std::string>, StringCaseInsensitiveHash, StringCaseInsensitiveEqual> _paramGlobal; // 全局参数
    std::vector<ExpressionPartition> _vectorExpression; // 被拆分成运算符号和数
};

void TestParseParameter();