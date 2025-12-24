#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
using namespace std;

// 文法规则定义
map<string, vector<vector<string>>> grammar = {
    {"program", {{"compoundstmt"}}},
    {"stmt", {{"ifstmt"}, {"whilestmt"}, {"assgstmt"}, {"compoundstmt"}}},
    {"compoundstmt", {{"{", "stmts", "}"}}},
    {"stmts", {{"stmt", "stmts"}, {"E"}}},
    {"ifstmt", {{"if", "(", "boolexpr", ")", "then", "stmt", "else", "stmt"}}},
    {"whilestmt", {{"while", "(", "boolexpr", ")", "stmt"}}},
    {"assgstmt", {{"ID", "=", "arithexpr", ";"}}},
    {"boolexpr", {{"arithexpr", "boolop", "arithexpr"}}},
    {"boolop", {{"<"}, {">"}, {"<="}, {">="}, {"=="}}},
    {"arithexpr", {{"multexpr", "arithexprprime"}}},
    {"arithexprprime", {{"+", "multexpr", "arithexprprime"}, {"-", "multexpr", "arithexprprime"}, {"E"}}},
    {"multexpr", {{"simpleexpr", "multexprprime"}}},
    {"multexprprime", {{"*", "simpleexpr", "multexprprime"}, {"/", "simpleexpr", "multexprprime"}, {"E"}}},
    {"simpleexpr", {{"ID"}, {"NUM"}, {"(", "arithexpr", ")"}}}
};

// LL(1)分析表
map<pair<string, string>, vector<string>> parseTable = {
    {{"program", "{"}, {"compoundstmt"}},
    
    {{"stmt", "{"}, {"compoundstmt"}},
    {{"stmt", "if"}, {"ifstmt"}},
    {{"stmt", "while"}, {"whilestmt"}},
    {{"stmt", "ID"}, {"assgstmt"}},
    
    {{"compoundstmt", "{"}, {"{", "stmts", "}"}},
    
    {{"stmts", "{"}, {"stmt", "stmts"}},
    {{"stmts", "if"}, {"stmt", "stmts"}},
    {{"stmts", "while"}, {"stmt", "stmts"}},
    {{"stmts", "ID"}, {"stmt", "stmts"}},
    {{"stmts", "}"}, {"E"}},
    
    {{"ifstmt", "if"}, {"if", "(", "boolexpr", ")", "then", "stmt", "else", "stmt"}},
    
    {{"whilestmt", "while"}, {"while", "(", "boolexpr", ")", "stmt"}},
    
    {{"assgstmt", "ID"}, {"ID", "=", "arithexpr", ";"}},
    
    {{"boolexpr", "("}, {"arithexpr", "boolop", "arithexpr"}},
    {{"boolexpr", "ID"}, {"arithexpr", "boolop", "arithexpr"}},
    {{"boolexpr", "NUM"}, {"arithexpr", "boolop", "arithexpr"}},
    
    {{"boolop", "<"}, {"<"}},
    {{"boolop", ">"}, {">"}},
    {{"boolop", "<="}, {"<="}},
    {{"boolop", ">="}, {">="}},
    {{"boolop", "=="}, {"=="}},
    
    {{"arithexpr", "("}, {"multexpr", "arithexprprime"}},
    {{"arithexpr", "ID"}, {"multexpr", "arithexprprime"}},
    {{"arithexpr", "NUM"}, {"multexpr", "arithexprprime"}},
    
    {{"arithexprprime", ")"}, {"E"}},
    {{"arithexprprime", ";"}, {"E"}},
    {{"arithexprprime", "<"}, {"E"}},
    {{"arithexprprime", ">"}, {"E"}},
    {{"arithexprprime", "<="}, {"E"}},
    {{"arithexprprime", ">="}, {"E"}},
    {{"arithexprprime", "=="}, {"E"}},
    {{"arithexprprime", "+"}, {"+", "multexpr", "arithexprprime"}},
    {{"arithexprprime", "-"}, {"-", "multexpr", "arithexprprime"}},
    
    {{"multexpr", "("}, {"simpleexpr", "multexprprime"}},
    {{"multexpr", "ID"}, {"simpleexpr", "multexprprime"}},
    {{"multexpr", "NUM"}, {"simpleexpr", "multexprprime"}},
    
    {{"multexprprime", ")"}, {"E"}},
    {{"multexprprime", ";"}, {"E"}},
    {{"multexprprime", "<"}, {"E"}},
    {{"multexprprime", ">"}, {"E"}},
    {{"multexprprime", "<="}, {"E"}},
    {{"multexprprime", ">="}, {"E"}},
    {{"multexprprime", "=="}, {"E"}},
    {{"multexprprime", "+"}, {"E"}},
    {{"multexprprime", "-"}, {"E"}},
    {{"multexprprime", "*"}, {"*", "simpleexpr", "multexprprime"}},
    {{"multexprprime", "/"}, {"/", "simpleexpr", "multexprprime"}},
    
    {{"simpleexpr", "("}, {"(", "arithexpr", ")"}},
    {{"simpleexpr", "ID"}, {"ID"}},
    {{"simpleexpr", "NUM"}, {"NUM"}}
};

// 终结符集合
set<string> terminals = {"{", "}", "if", "(", ")", "then", "else", "while", 
                        "ID", "=", ">", "<", ">=", "<=", "==", "+", "-", "*", 
                        "/", "NUM", ";", "$"};

// 非终结符集合
set<string> nonterminals = {"program", "stmt", "compoundstmt", "stmts", "ifstmt", 
                           "whilestmt", "assgstmt", "boolexpr", "boolop", 
                           "arithexpr", "arithexprprime", "multexpr", 
                           "multexprprime", "simpleexpr"};

struct Token {
    string value;
    int line;
    Token(string v="", int l=1): value(v), line(l) {}
};

vector<Token> tokenize(const string& src) {
    vector<Token> tokens;
    istringstream iss(src);
    string line;
    int lineNum = 1;
    bool lastLineHadTokens = false;
    
    while (getline(iss, line)) {
        istringstream lineStream(line);
        string token;
        bool lineHasTokens = false;
        
        while (lineStream >> token) {
            if (!token.empty()) {
                tokens.emplace_back(token, lineNum);
                lineHasTokens = true;
            }
        }
        
        if (lineHasTokens) {
            lineNum++;
            lastLineHadTokens = true;
        } else if (lastLineHadTokens) {
            // 空行，但不增加行号
            // 行号不变
            lastLineHadTokens = false;
        }
    }
    tokens.emplace_back("$", lineNum);
    return tokens;
}

class Parser {
    vector<Token> tokens;
    size_t pos = 0;
    bool semicolonMissing = false;
    int  missingLine = 0;
    bool hasError = false;
    int lastConsumedLine = 0;  // 记录上一个消耗的token的行号

public:
    Parser(const vector<Token>& t) : tokens(t) {}

    Token peek() const {
        return pos < tokens.size() ? tokens[pos] : Token("$",0);
    }
    
    void consume() { 
        if (pos < tokens.size()) {
            lastConsumedLine = tokens[pos].line;  // 记录行号
            ++pos; 
        }
    }

    // output==false 只检测错误，output==true 真正打印树
    bool parse(const string& symbol, int depth, bool output) {
        if (symbol == "E") {
            if (output) {
                cout << endl;
                for (int i = 0; i < depth; ++i) cout << "\t";
                cout << "E";
            }
            return true;
        }

        // 终结符
        if (terminals.count(symbol)) {
            if (output) {
                cout << endl;
                for (int i = 0; i < depth; ++i) cout << "\t";
                cout << symbol;
            }

            Token cur = peek();
            if (cur.value == symbol) {
                consume();
                return true;
            }

            // 专门处理缺少分号的情况
            if (symbol == ";") {
                if (!semicolonMissing) {
                    semicolonMissing = true;
                    // 使用上一个消耗的token的行号作为错误行号
                    missingLine = lastConsumedLine;
                    if (missingLine == 0) {
                        missingLine = cur.line;
                    }
                }
                // 假装匹配成功，不消耗任何token
                return true;
            }
            return false;
        }

        // 非终结符：先打印自己
        if (output) {
            if (depth > 0) {
                cout << endl;
            }
            for (int i = 0; i < depth; ++i) cout << "\t";
            cout << symbol;
        }

        string cur = peek().value;
        auto key = make_pair(symbol, cur);

        if (parseTable.count(key) == 0) {
            // ε产生式
            if ((symbol == "stmts" && cur == "}") ||
                (symbol == "arithexprprime" && (cur == ")" || cur == ";" || cur == "$" ||
                    cur == "<" || cur == ">" || cur == "<=" || cur == ">=" || cur == "==" || cur == "}")) ||
                (symbol == "multexprprime" && (cur == "+" || cur == "-" || cur == ")" ||
                    cur == ";" || cur == "$" || cur == "<" || cur == ">" ||
                    cur == "<=" || cur == ">=" || cur == "==" || cur == "}"))) {
                return parse("E", depth + 1, output);
            }
            
            // 如果是分号缺失的情况，尝试错误恢复
            if (semicolonMissing) {
                return false;
            }
            
            hasError = true;
            return false;
        }

        const vector<string>& production = parseTable[key];
        for (const string& s : production) {
            if (!parse(s, depth + 1, output)) {
                return false;
            }
        }
        return true;
    }

    // 对外接口
    bool hasMissingSemicolon() const { return semicolonMissing; }
    int  getMissingLine()     const { return missingLine; }
    void reset() { 
        pos = 0; 
        semicolonMissing = false; 
        missingLine = 0; 
        hasError = false;
        lastConsumedLine = 0;
    }
    bool hasSyntaxError() const { return hasError; }
};

void read_prog(string& prog) {
    char c;
    while (scanf("%c", &c) != EOF) prog += c;
}

void Analysis() {
    string prog;
    read_prog(prog);
    auto tokens = tokenize(prog);

    Parser p(tokens);

    //第一遍：检测是否缺分号 
    p.parse("program", 0, false);
    
    // 输出错误信息 
    if (p.hasMissingSemicolon()) {
        cout << "语法错误,第" << p.getMissingLine() << "行,缺少\";\"" << endl;
    }
    
    // 第二遍：输出语法树 
    p.reset();
    p.parse("program", 0, true);
}