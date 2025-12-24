#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <streambuf>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "LR parser.h"

using namespace std;

// 枚举token存在的类型
map<string, short> typeNameMap = {
    {"auto",1}, {"break",2}, {"case",3}, {"char",4}, {"const",5},
    {"continue",6}, {"default",7}, {"do",8}, {"double",9}, {"else",10},
    {"enum",11}, {"extern",12}, {"float",13}, {"for",14}, {"goto",15},
    {"if",16}, {"int",17}, {"long",18}, {"register",19}, {"return",20},
    {"short",21}, {"signed",22}, {"sizeof",23}, {"static",24}, {"struct",25},
    {"switch",26}, {"typedef",27}, {"union",28}, {"unsigned",29}, {"void",30},
    {"volatile",31}, {"while",32}, {"-",33}, {"--",34}, {"-=",35},
    {"->",36}, {"!",37}, {"!=",38}, {"%",39}, {"%=",40},
    {"&",41}, {"&&",42}, {"&=",43}, {"(",44}, {")",45},
    {"*",46}, {"*=",47}, {",",48}, {".",49}, {"/",50},
    {"/=",51}, {":",52}, {";",53}, {"?",54}, {"[",55},
    {"]",56}, {"^",57}, {"^=",58}, {"{",59}, {"|",60},
    {"||",61}, {"|=",62}, {"}",63}, {"~",64}, {"+",65},
    {"++",66}, {"+=",67}, {"<",68}, {"<<",69}, {"<<=",70},
    {"<=",71}, {"=",72}, {"==",73}, {">",74}, {">=",75},
    {">>",76}, {">>=",77}, {"\"",78}, {"Comment",79}, {"Constant",80},
    {"Identifier",81}
};

// 关键字数组
vector<string> Keywords = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", 
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "int", 
    "long", "register", "return", "short", "signed", "sizeof", "static", 
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
};

// 字符类型枚举
enum CharType {
    Alphabet, Number, Symbol, Slash, Blank, Quote, Endl, Star, Dot, Eof
};

// Token数据结构
struct TokenInfo {
    int id;
    string lexeme;
    string typeName;
    int typeId;
};

// 全局变量
string src;
int pos = 0;
int tokenCount = 0;
short quoteStatus = 0;

// 查看下一个字符的类型 
CharType peekChar() {
    if (pos >= src.length()) return Eof;
    char ch = src[pos];
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_') return Alphabet;
    if (ch >= '0' && ch <= '9') return Number;
    switch (ch) {
        case ' ': case '\t': return Blank;
        case '\n': return Endl;
        case '.': return Dot;
        case '*': return Star;
        case '/': return Slash;
        case '"': return Quote;
        default: return Symbol;
    }
}

// 获取下一个字符
char getChar() {
    if (pos >= src.length()) return '\0';
    return src[pos++];
}

// 获取关键字或标识符
TokenInfo getIdentifierOrKeyword() {
    string lexeme = "";
    while (pos < src.length()) {
        CharType ct = peekChar();
        if (ct == Alphabet || ct == Number) {
            lexeme += getChar();
        } else {
            break;
        }
    }
    
    // 检查是否为关键字
    bool isKeyword = false;
    for (const auto& keyword : Keywords) {
        if (keyword == lexeme) {
            isKeyword = true;
            break;
        }
    }
    
    TokenInfo token;
    token.id = tokenCount;
    token.lexeme = lexeme;
    
    if (isKeyword) {
        token.typeName = "Keyword";
        token.typeId = typeNameMap[lexeme];
    } else {
        token.typeName = "Identifier";
        token.typeId = 81;
    }
    
    return token;
}

// 获取数字常量
TokenInfo getNumber() {
    string lexeme = "";
    bool hasDot = false;
    
    while (pos < src.length()) {
        CharType ct = peekChar();
        if (ct == Number) {
            lexeme += getChar();
        } else if (ct == Dot && !hasDot) {
            lexeme += getChar();
            hasDot = true;
        } else {
            break;
        }
    }
    
    TokenInfo token;
    token.id = tokenCount;
    token.lexeme = lexeme;
    token.typeName = "Constant";
    token.typeId = 80;
    
    return token;
}

// 获取运算符
TokenInfo getOperator() {
    string lexeme = "";
    lexeme += getChar();
    
    // 检查是否为多字符运算符
    if (pos < src.length()) {
        string twoChar = lexeme + src[pos];
        if (typeNameMap.find(twoChar) != typeNameMap.end()) {
            // 检查三个字符的运算符
            if (pos + 1 < src.length()) {
                string threeChar = twoChar + src[pos + 1];
                if (typeNameMap.find(threeChar) != typeNameMap.end()) {
                    lexeme = threeChar;
                    pos += 2;
                } else {
                    // 两个字符的运算符
                    lexeme = twoChar;
                    pos++;
                }
            } else {
                // 两个字符的运算符
                lexeme = twoChar;
                pos++;
            }
        }
    }
    
    TokenInfo token;
    token.id = tokenCount;
    token.lexeme = lexeme;
    
    // 查找类型ID
    if (typeNameMap.find(lexeme) != typeNameMap.end()) {
        token.typeId = typeNameMap[lexeme];
        token.typeName = "Operator";
    } else {
        // 如果找不到映射，设为未知运算符
        token.typeId = 0;
        token.typeName = "Operator";
    }
    
    return token;
}

// 处理注释或除法运算符
TokenInfo getCommentOrOperator() {
    // 消耗当前的 '/'
    string lexeme = "/";
    getChar();

    if (pos >= src.length()) {
        TokenInfo token;
        token.id = tokenCount;
        token.lexeme = lexeme;
        token.typeName = "Operator";
        token.typeId = typeNameMap[lexeme];
        return token;
    }

    char nextChar = src[pos];

    // 单行注释：//...
    if (nextChar == '/') {
        lexeme += '/';
        pos++; // 消耗第二个 '/'

        // 读取直到行尾（不包含换行符）
        while (pos < src.length() && src[pos] != '\n' && src[pos] != '\r') {
            lexeme += src[pos];
            pos++;
        }

        TokenInfo token;
        token.id = tokenCount;
        token.lexeme = lexeme;
        token.typeName = "Comment";
        token.typeId = 79;
        return token;
    }
    // 多行注释：/* ... */
    else if (nextChar == '*') {
        lexeme += '*';
        pos++; // 消耗 '*'

        while (pos < src.length()) {
            if (src[pos] == '*' && pos + 1 < src.length() && src[pos + 1] == '/') {
                lexeme += "*/";
                pos += 2; // 消耗 '*/'
                break;
            }
            lexeme += src[pos];
            pos++;
        }

        TokenInfo token;
        token.id = tokenCount;
        token.lexeme = lexeme;
        token.typeName = "Comment";
        token.typeId = 79;
        return token;
    }
    // 除法赋值运算符：/=
    else if (nextChar == '=') {
        lexeme += '=';
        pos++; // 消耗 '='

        TokenInfo token;
        token.id = tokenCount;
        token.lexeme = lexeme;
        token.typeName = "Operator";
        token.typeId = typeNameMap[lexeme];
        return token;
    }
    // 普通除法运算符：/
    else {
        TokenInfo token;
        token.id = tokenCount;
        token.lexeme = lexeme;
        token.typeName = "Operator";
        token.typeId = typeNameMap[lexeme];
        return token;
    }
}

// 处理引号内的内容
TokenInfo getQuoteContent() {
    TokenInfo token;
    token.id = tokenCount;
    if (quoteStatus == 0) {
        token.lexeme = "\"";
        token.typeName = "Operator";
        token.typeId = 78;
        pos++;
        quoteStatus = 1;
    } else if (quoteStatus == 1) {
        token.lexeme = "";
        while (pos < src.length() && src[pos] != '"') {
            token.lexeme += src[pos];
            pos++;
        }
        token.typeName = "Identifier";
        token.typeId = 81;
        quoteStatus = 2;
    } else {
        token.lexeme = "\"";
        token.typeName = "Operator";
        token.typeId = 78;
        pos++;
        quoteStatus = 0;
    }
    return token;
}

// 简单的JSON生成函数
string generateJSON(const vector<TokenInfo>& tokens) {
    stringstream json;
    json << "{\"tokens\":[";
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        json << "{";
        json << "\"id\":" << token.id << ",";
        
        // 对lexeme进行JSON转义
        string escapedLexeme;
        for (char c : token.lexeme) {
            switch (c) {
                case '"': escapedLexeme += "\\\""; break;
                case '\\': escapedLexeme += "\\\\"; break;
                case '\b': escapedLexeme += "\\b"; break;
                case '\f': escapedLexeme += "\\f"; break;
                case '\n': escapedLexeme += "\\n"; break;
                case '\r': escapedLexeme += "\\r"; break;
                case '\t': escapedLexeme += "\\t"; break;
                default:
                    // 对于控制字符，使用Unicode转义
                    if (static_cast<unsigned char>(c) < 32) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        escapedLexeme += buf;
                    } else {
                        escapedLexeme += c;
                    }
                    break;
            }
        }
        
        json << "\"lexeme\":\"" << escapedLexeme << "\",";
        json << "\"typeName\":\"" << token.typeName << "\",";
        json << "\"typeId\":" << token.typeId;
        json << "}";
        if (i < tokens.size() - 1) json << ",";
    }
    
    json << "],\"stats\":{";
    json << "\"total\":" << tokens.size() << ",";
    json << "\"keywords\":" << count_if(tokens.begin(), tokens.end(), 
        [](const TokenInfo& t) { return t.typeName == "Keyword"; }) << ",";
    json << "\"identifiers\":" << count_if(tokens.begin(), tokens.end(), 
        [](const TokenInfo& t) { return t.typeName == "Identifier"; }) << ",";
    json << "\"constants\":" << count_if(tokens.begin(), tokens.end(), 
        [](const TokenInfo& t) { return t.typeName == "Constant"; }) << ",";
    json << "\"operators\":" << count_if(tokens.begin(), tokens.end(), 
        [](const TokenInfo& t) { return t.typeName == "Operator"; }) << ",";
    json << "\"comments\":" << count_if(tokens.begin(), tokens.end(), 
        [](const TokenInfo& t) { return t.typeName == "Comment"; });
    json << "}}";
    
    return json.str();
}


string analyzeCode(const string& code) {
    src = code;
    pos = 0;
    tokenCount = 0;
    quoteStatus = 0;
    vector<TokenInfo> tokens;
    while (pos < src.length()) {
        while (pos < src.length()) {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { pos++; }
            else { break; }
        }
        if (pos >= src.length()) break;
        tokenCount++;
        char currentChar = src[pos];
        TokenInfo token;
        if (quoteStatus > 0) {
            token = getQuoteContent();
        } else if (isalpha(currentChar) || currentChar == '_') {
            token = getIdentifierOrKeyword();
        } else if (isdigit(currentChar)) {
            token = getNumber();
        } else if (currentChar == '/') {
            token = getCommentOrOperator();
        } else if (currentChar == '"') {
            token = getQuoteContent();
        } else {
            token = getOperator();
        }
        tokens.push_back(token);
    }
    return generateJSON(tokens);
}

// LL(1) parser implementation
struct LLToken {
    string value;
    int line;
    LLToken(string v = "", int l = 1) : value(v), line(l) {}
};

static map<string, vector<vector<string>>> LLGrammar = {
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

static map<pair<string, string>, vector<string>> LLParseTable = {
    { make_pair(string("program"), string("{")), vector<string>{"compoundstmt"} },
    { make_pair(string("stmt"), string("{")), vector<string>{"compoundstmt"} },
    { make_pair(string("stmt"), string("if")), vector<string>{"ifstmt"} },
    { make_pair(string("stmt"), string("while")), vector<string>{"whilestmt"} },
    { make_pair(string("stmt"), string("ID")), vector<string>{"assgstmt"} },
    { make_pair(string("compoundstmt"), string("{")), vector<string>{"{", "stmts", "}"} },
    { make_pair(string("stmts"), string("{")), vector<string>{"stmt", "stmts"} },
    { make_pair(string("stmts"), string("if")), vector<string>{"stmt", "stmts"} },
    { make_pair(string("stmts"), string("while")), vector<string>{"stmt", "stmts"} },
    { make_pair(string("stmts"), string("ID")), vector<string>{"stmt", "stmts"} },
    { make_pair(string("stmts"), string("}")), vector<string>{"E"} },
    { make_pair(string("ifstmt"), string("if")), vector<string>{"if", "(", "boolexpr", ")", "then", "stmt", "else", "stmt"} },
    { make_pair(string("whilestmt"), string("while")), vector<string>{"while", "(", "boolexpr", ")", "stmt"} },
    { make_pair(string("assgstmt"), string("ID")), vector<string>{"ID", "=", "arithexpr", ";"} },
    { make_pair(string("boolexpr"), string("(")), vector<string>{"arithexpr", "boolop", "arithexpr"} },
    { make_pair(string("boolexpr"), string("ID")), vector<string>{"arithexpr", "boolop", "arithexpr"} },
    { make_pair(string("boolexpr"), string("NUM")), vector<string>{"arithexpr", "boolop", "arithexpr"} },
    { make_pair(string("boolop"), string("<")), vector<string>{"<"} },
    { make_pair(string("boolop"), string(">")), vector<string>{">"} },
    { make_pair(string("boolop"), string("<=")), vector<string>{"<="} },
    { make_pair(string("boolop"), string(">=")), vector<string>{">="} },
    { make_pair(string("boolop"), string("==")), vector<string>{"=="} },
    { make_pair(string("arithexpr"), string("(")), vector<string>{"multexpr", "arithexprprime"} },
    { make_pair(string("arithexpr"), string("ID")), vector<string>{"multexpr", "arithexprprime"} },
    { make_pair(string("arithexpr"), string("NUM")), vector<string>{"multexpr", "arithexprprime"} },
    { make_pair(string("arithexprprime"), string(")")), vector<string>{"E"} },
    { make_pair(string("arithexprprime"), string(";")), vector<string>{"E"} },
    { make_pair(string("arithexprprime"), string("<")), vector<string>{"E"} },
    { make_pair(string("arithexprprime"), string(">")), vector<string>{"E"} },
    { make_pair(string("arithexprprime"), string("<=")), vector<string>{"E"} },
    { make_pair(string("arithexprprime"), string(">=")), vector<string>{"E"} },
    { make_pair(string("arithexprprime"), string("==")), vector<string>{"E"} },
    { make_pair(string("arithexprprime"), string("+")), vector<string>{"+", "multexpr", "arithexprprime"} },
    { make_pair(string("arithexprprime"), string("-")), vector<string>{"-", "multexpr", "arithexprprime"} },
    { make_pair(string("multexpr"), string("(")), vector<string>{"simpleexpr", "multexprprime"} },
    { make_pair(string("multexpr"), string("ID")), vector<string>{"simpleexpr", "multexprprime"} },
    { make_pair(string("multexpr"), string("NUM")), vector<string>{"simpleexpr", "multexprprime"} },
    { make_pair(string("multexprprime"), string(")")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string(";")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string("<")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string(">")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string("<=")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string(">=")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string("==")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string("+")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string("-")), vector<string>{"E"} },
    { make_pair(string("multexprprime"), string("*")), vector<string>{"*", "simpleexpr", "multexprprime"} },
    { make_pair(string("multexprprime"), string("/")), vector<string>{"/", "simpleexpr", "multexprprime"} },
    { make_pair(string("simpleexpr"), string("(")), vector<string>{"(", "arithexpr", ")"} },
    { make_pair(string("simpleexpr"), string("ID")), vector<string>{"ID"} },
    { make_pair(string("simpleexpr"), string("NUM")), vector<string>{"NUM"} }
};

static set<string> LLTerminals = {"{", "}", "if", "(", ")", "then", "else", "while",
    "ID", "=", ">", "<", ">=", "<=", "==", "+", "-",
    "*", "/", "NUM", ";", "$"};

vector<LLToken> llTokenize(const string& prog) {
    vector<LLToken> tokens;
    istringstream iss(prog);
    string line;
    int lineNum = 1;
    bool lastLineHadTokens = false;
    while (getline(iss, line)) {
        istringstream ls(line);
        string tk;
        bool lineHasTokens = false;
        while (ls >> tk) {
            if (!tk.empty()) {
                tokens.emplace_back(tk, lineNum);
                lineHasTokens = true;
            }
        }
        if (lineHasTokens) { lineNum++; lastLineHadTokens = true; }
        else if (lastLineHadTokens) { lastLineHadTokens = false; }
    }
    tokens.emplace_back("$", lineNum);
    return tokens;
}

struct LLParseResult {
    string tree;
    bool missingSemicolon = false;
    int missingLine = 0;
    bool syntaxError = false;
};

class LLParser {
    vector<LLToken> tokens;
    size_t p = 0;
public:
    bool semicolonMissing = false;
    int missingLine = 0;
    bool hasError = false;
    int lastConsumedLine = 0;
    string tree;

    LLParser(const vector<LLToken>& t) : tokens(t) {}

    LLToken peek() const { return p < tokens.size() ? tokens[p] : LLToken("$", 0); }
    void consume() { if (p < tokens.size()) { lastConsumedLine = tokens[p].line; ++p; } }

    void append(int depth, const string& s) {
        for (int i = 0; i < depth; ++i) tree += "\t";
        tree += s;
        tree += "\n";
    }

    bool parse(const string& symbol, int depth, bool output) {
        if (symbol == "E") { if (output) append(depth, "E"); return true; }
        if (LLTerminals.count(symbol)) {
            if (output) append(depth, symbol);
            LLToken cur = peek();
            if (cur.value == symbol) { consume(); return true; }
            if (symbol == ";") {
                if (!semicolonMissing) {
                    semicolonMissing = true;
                    missingLine = lastConsumedLine;
                    if (missingLine == 0) missingLine = cur.line;
                }
                return true;
            }
            return false;
        }
        if (output) append(depth, symbol);
        string cur = peek().value;
        auto key = make_pair(symbol, cur);
        if (LLParseTable.count(key) == 0) {
            if ((symbol == "stmts" && cur == "}") ||
                (symbol == "arithexprprime" && (cur == ")" || cur == ";" || cur == "$" ||
                    cur == "<" || cur == ">" || cur == "<=" || cur == ">=" || cur == "==" || cur == "}")) ||
                (symbol == "multexprprime" && (cur == "+" || cur == "-" || cur == ")" ||
                    cur == ";" || cur == "$" || cur == "<" || cur == ">" || cur == "<=" || cur == ">=" || cur == "==" || cur == "}"))) {
                return parse("E", depth + 1, output);
            }
            if (semicolonMissing) return false;
            hasError = true;
            return false;
        }
        const vector<string>& prod = LLParseTable[key];
        for (const string& s : prod) {
            if (!parse(s, depth + 1, output)) return false;
        }
        return true;
    }

    void reset() { p = 0; semicolonMissing = false; missingLine = 0; hasError = false; lastConsumedLine = 0; tree.clear(); }
};


vector<string> split(string const& s) {
    vector<string> result;
    istringstream stm(s);
    string token, line;
    while (getline(stm, line)) {
        if (!line.empty()) {  // 只有非空行才处理
            istringstream linestm(line);
            while (linestm >> token) {
                result.push_back(token);
            }
            if (token != "$") {
                result.push_back("<endl>");
            }
        }
    }
    return result;
}

struct Identifier {
    string name;
    bool isReal;
    double value;
    Identifier(string n = "", bool real = false, double v = 0) : name(n), isReal(real), value(v) {}
};

map<string, Identifier> IDMap;

void conversionError(int lineNum) {
    printf("error message:line %d,realnum can not be translated into int type\n", lineNum);
}

void divisionByZeroError(int lineNum) {
    printf("error message:line %d,division by zero\n", lineNum);
}

double executeAssign(Identifier* target, Identifier* left, Identifier* right, char op, int lineNum) {
    double result = 0;
    switch (op) {
        case '+': result = left->value + right->value; break;
        case '-': result = left->value - right->value; break;
        case '*': result = left->value * right->value; break;
        case '/':
            if (right->value == 0) {
                divisionByZeroError(lineNum);
                return 0;
            }
            result = left->value / right->value; 
            break;
    }
    target->value = result;
    return result;
}

bool evalBool(Identifier* left, Identifier* right, string cmp) {
    if (cmp == ">") return left->value > right->value;
    if (cmp == ">=") return left->value >= right->value;
    if (cmp == "<") return left->value < right->value;
    if (cmp == "<=") return left->value <= right->value;
    return left->value == right->value;
}

class Translator {
    vector<string> tokens;
    int pos = 0, lineNum = 1;
    bool hasError = false;
    
    void forward(int count) {
        for (int i = 0; i < count;) {
            if (pos < tokens.size() && tokens[pos] == "<endl>") {
                lineNum++;  // 遇到<endl>表示一行结束，行号增加
                pos++;
            } else {
                i++;
                pos++;
            }
        }
        // 跳过连续的<endl>
        while (pos < tokens.size() && tokens[pos] == "<endl>") {
            lineNum++;
            pos++;
        }
    }
    
    Identifier* getOrCreateId(string name, bool isReal = false) {
        if (IDMap.find(name) == IDMap.end()) {
            IDMap[name] = Identifier(name, isReal, 0);
        }
        return &IDMap[name];
    }
    
    Identifier* getValue(string token) {
        if (token[0] >= '0' && token[0] <= '9') {
            return new Identifier("temp", token.find('.') != string::npos, stod(token));
        }
        return getOrCreateId(token);
    }
    
    void handleDeclare() {
        string type = tokens[pos];
        string name = tokens[pos + 1];
        string valueStr = tokens[pos + 3];
        
        if (type == "int") {
            if (valueStr.find('.') != string::npos) {
                conversionError(lineNum);
                hasError = true;
            }
            IDMap[name] = Identifier(name, false, stod(valueStr));
        } else {
            IDMap[name] = Identifier(name, true, stod(valueStr));
        }
        forward(4);
    }
    
    void handleAssign() {
        string targetName = tokens[pos];
        Identifier* target = getOrCreateId(targetName);
        
        string leftStr = tokens[pos + 2];
        string rightStr = tokens[pos + 4];
        char op = tokens[pos + 3][0];
        
        Identifier* left = getValue(leftStr);
        Identifier* right = getValue(rightStr);
        
        executeAssign(target, left, right, op, lineNum);
        
        forward(5);
        
        if (pos < tokens.size() && (tokens[pos] == "+" || tokens[pos] == "-" || 
            tokens[pos] == "*" || tokens[pos] == "/")) {
            char op2 = tokens[pos][0];
            forward(1);
            
            if (pos < tokens.size()) {
                string nextRight = tokens[pos];
                Identifier* right2 = getValue(nextRight);
                Identifier* left2 = new Identifier("temp", target->isReal, target->value);
                
                executeAssign(target, left2, right2, op2, lineNum);
                forward(1);
            }
        }
    }
    
    void handleIf() {
        forward(2); // 跳过 "if" 和 "("
        
        // 解析条件
        string leftName = tokens[pos];
        string cmp = tokens[pos + 1];
        string rightName = tokens[pos + 2];
        forward(3); // 跳过左操作数、比较符、右操作数
        
        Identifier* left = getOrCreateId(leftName);
        Identifier* right = getOrCreateId(rightName);
        bool condition = evalBool(left, right, cmp);
        
        forward(2); // 跳过 ")" 和 "then"
        
        if (condition) {
            // 执行then分支的赋值
            handleAssign(); // 执行后pos指向分号
            // 跳过then分支的分号
            forward(1);
            // 跳过else分支（包括else关键字和整个else赋值语句）
            while (pos < tokens.size() && tokens[pos] != "else") {
                forward(1);
            }
            if (pos < tokens.size() && tokens[pos] == "else") {
                forward(1); // 跳过else
                // 跳过else赋值语句：id = id op id ;
                forward(5); // 跳过id, =, id, op, id
                forward(1); // 跳过分号
            }
        } else {
            // 跳过then分支的赋值语句
            while (pos < tokens.size() && tokens[pos] != ";") {
                forward(1);
            }
            forward(1); // 跳过分号
            // 执行else分支
            if (pos < tokens.size() && tokens[pos] == "else") {
                forward(1); // 跳过else
                handleAssign(); // 执行else赋值
            }
        }
    }
    
    void printResult() {
    for (auto& p : IDMap) {
        if (p.first != "temp") {
            if (p.second.isReal) {
                printf("%s: %g\n", p.first.c_str(), p.second.value);
            } else {
                printf("%s: %d\n", p.first.c_str(), (int)p.second.value);
            }
        }
    }
}

public:
    Translator(string& prog) {
        prog += " $";
        tokens = split(prog);
    }
    
    void translate() {
        while (pos < tokens.size()) {
            string token = tokens[pos];
            if (token == "$") {
                if (!hasError) printResult();
                break;
            }
            if (token == "<endl>") { 
                lineNum++; 
                pos++; 
                continue; 
            }
            if (token == ";" || token == "{" || token == "}") { 
                pos++; 
                continue; 
            }
            
            if (token == "real" || token == "int") {
                handleDeclare();
            }
            else if (token == "if") {
                handleIf();
            }
            else {
                handleAssign();
            }
        }
    }
};

string llParseToJSON(const string& code) {
    auto tokens = llTokenize(code);
    LLParser parser(tokens);
    parser.parse("program", 0, false);
    bool miss = parser.semicolonMissing;
    int line = parser.missingLine;
    parser.reset();
    parser.parse("program", 0, true);
    string t = parser.tree;
    stringstream json;
    // Build JSON safely (escape tree text)
    string escaped;
    for (char c : t) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    json << "{\"tree\":\"" << escaped << "\",";
    json << "\"missingSemicolon\":" << (miss ? "true" : "false") << ",";
    json << "\"missingLine\":" << line << ",";
    json << "\"syntaxError\":" << (parser.hasError ? "true" : "false") << "}";
    return json.str();
}

string lrParseToJSON(const string& code) {
    stringstream errss;
    streambuf* oldbuf = cout.rdbuf(errss.rdbuf());
    {
        Parser p(code);
        p.set_mode(MODE_ERROR_CHECKING);
        p.parse();
    }
    cout.rdbuf(oldbuf);
    bool miss = false;
    int line = 0;
    {
        string s = errss.str();
        size_t pos = s.find("语法错误");
        if (pos != string::npos) {
            miss = true;
            size_t lp = s.find("第", pos);
            size_t lp2 = s.find("行", lp);
            if (lp != string::npos && lp2 != string::npos) {
                string num = s.substr(lp + 3, lp2 - (lp + 3));
                line = atoi(num.c_str());
            }
        }
    }
    string tree;
    stringstream outss;
    oldbuf = cout.rdbuf(outss.rdbuf());
    {
        Parser p(code);
        p.set_mode(MODE_PARSE);
        p.parse();
    }
    cout.rdbuf(oldbuf);
    tree = outss.str();
    string escaped;
    for (char c : tree) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    stringstream json;
    json << "{\"tree\":\"" << escaped << "\",";
    json << "\"missingSemicolon\":" << (miss ? "true" : "false") << ",";
    json << "\"missingLine\":" << line << ",";
    json << "\"syntaxError\":false}";
    return json.str();
}

string translationToJSON(const string& code) {
    IDMap.clear();
    string prog = code;
    string output;
#ifdef _WIN32
    int oldfd = _dup(_fileno(stdout));
    FILE* tmp = tmpfile();
    if (tmp) {
        _dup2(_fileno(tmp), _fileno(stdout));
        Translator t(prog);
        t.translate();
        fflush(stdout);
        fseek(tmp, 0, SEEK_SET);
        char buf[1024];
        while (true) {
            size_t n = fread(buf, 1, sizeof(buf), tmp);
            if (n == 0) break;
            output.append(buf, n);
        }
        _dup2(oldfd, _fileno(stdout));
        fclose(tmp);
        _close(oldfd);
    } else {
        Translator t(prog);
        t.translate();
    }
#else
    int oldfd = dup(fileno(stdout));
    FILE* tmp = tmpfile();
    if (tmp) {
        dup2(fileno(tmp), fileno(stdout));
        Translator t(prog);
        t.translate();
        fflush(stdout);
        fseek(tmp, 0, SEEK_SET);
        char buf[1024];
        while (true) {
            size_t n = fread(buf, 1, sizeof(buf), tmp);
            if (n == 0) break;
            output.append(buf, n);
        }
        dup2(oldfd, fileno(stdout));
        fclose(tmp);
        close(oldfd);
    } else {
        Translator t(prog);
        t.translate();
    }
#endif
    string escaped;
    for (char c : output) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    stringstream json;
    json << "{\"output\":\"" << escaped << "\"}";
    return json.str();
}

// 读取文件内容
string readFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        return "";
    }
    
    string content;
    file.seekg(0, ios::end);
    content.resize(file.tellg());
    file.seekg(0, ios::beg);
    file.read(&content[0], content.size());
    file.close();
    
    return content;
}

// 获取文件MIME类型
string getMimeType(const string& filename) {
    if (filename.find(".html") != string::npos) return "text/html; charset=utf-8";
    if (filename.find(".css") != string::npos) return "text/css; charset=utf-8";
    if (filename.find(".js") != string::npos) return "application/javascript; charset=utf-8";
    if (filename.find(".png") != string::npos) return "image/png";
    if (filename.find(".jpg") != string::npos || filename.find(".jpeg") != string::npos) return "image/jpeg";
    return "text/plain; charset=utf-8";
}

// 简单的HTTP服务器
void startServer() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return;
    }
#endif

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        cerr << "无法创建套接字" << endl;
        return;
    }
    
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "绑定失败" << endl;
        return;
    }
    
    if (listen(server_fd, 3) < 0) {
        cerr << "监听失败" << endl;
        return;
    }
    
    cout << "服务器启动在 http://localhost:8080" << endl;
    
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            cerr << "接受连接失败" << endl;
            continue;
        }
        
        char buffer[4096] = {0};
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read > 0) {
            string request(buffer);
            
            // 解析请求路径
            string path = "/home.html"; // 默认路径
            size_t path_start = request.find(" ");
            if (path_start != string::npos) {
                size_t path_end = request.find(" ", path_start + 1);
                if (path_end != string::npos) {
                    path = request.substr(path_start + 1, path_end - path_start - 1);
                    if (path == "/") path = "/home.html";
                }
            }
            
            // 处理分析请求
            if (request.find("POST /analyze") != string::npos) {
                // 提取JSON数据
                size_t json_start = request.find("\r\n\r\n");
                if (json_start != string::npos) {
                    string json_str = request.substr(json_start + 4);
                    
                    // 查找并解析 code 字段
                    size_t code_start = json_str.find("\"code\":\"");
                    if (code_start != string::npos) {
                        code_start += 8; // 跳过"code":"
                        size_t code_end = code_start;
                        bool escaped = false;
                        while (code_end < json_str.size()) {
                            char ch = json_str[code_end];
                            if (escaped) { escaped = false; }
                            else if (ch == '\\') { escaped = true; }
                            else if (ch == '"') { break; }
                            code_end++;
                        }
                        string encoded_code = json_str.substr(code_start, code_end - code_start);
                        string decoded_code;
                        for (size_t i = 0; i < encoded_code.length(); i++) {
                            if (encoded_code[i] == '\\' && i + 1 < encoded_code.length()) {
                                switch (encoded_code[i + 1]) {
                                    case 'n': decoded_code += '\n'; i++; break;
                                    case 'r': decoded_code += '\r'; i++; break;
                                    case 't': decoded_code += '\t'; i++; break;
                                    case '\\': decoded_code += '\\'; i++; break;
                                    case '"': decoded_code += '"'; i++; break;
                                    default: decoded_code += encoded_code[i]; break;
                                }
                            } else {
                                decoded_code += encoded_code[i];
                            }
                        }
                        string result = analyzeCode(decoded_code);
                        string response = "HTTP/1.1 200 OK\r\n";
                        response += "Content-Type: application/json\r\n";
                        response += "Access-Control-Allow-Origin: *\r\n";
                        response += "Content-Length: " + to_string(result.length()) + "\r\n";
                        response += "\r\n";
                        response += result;
                        send(client_fd, response.c_str(), response.length(), 0);
                    } else {
                        string response = "HTTP/1.1 400 Bad Request\r\n";
                        response += "Content-Type: text/plain; charset=utf-8\r\n";
                        string msg = "Missing 'code' field\n";
                        response += "Content-Length: " + to_string(msg.length()) + "\r\n\r\n" + msg;
                        send(client_fd, response.c_str(), response.length(), 0);
                    }
                }
            } else if (request.find("POST /llparse") != string::npos) {
                size_t json_start = request.find("\r\n\r\n");
                if (json_start != string::npos) {
                    string json_str = request.substr(json_start + 4);
                    size_t code_start = json_str.find("\"code\":\"");
                    if (code_start != string::npos) {
                        code_start += 8;
                        size_t code_end = code_start; bool escaped = false;
                        while (code_end < json_str.size()) {
                            char ch = json_str[code_end];
                            if (escaped) { escaped = false; }
                            else if (ch == '\\') { escaped = true; }
                            else if (ch == '"') { break; }
                            code_end++;
                        }
                        string encoded_code = json_str.substr(code_start, code_end - code_start);
                        string decoded_code;
                        for (size_t i = 0; i < encoded_code.length(); i++) {
                            if (encoded_code[i] == '\\' && i + 1 < encoded_code.length()) {
                                switch (encoded_code[i + 1]) {
                                    case 'n': decoded_code += '\n'; i++; break;
                                    case 'r': decoded_code += '\r'; i++; break;
                                    case 't': decoded_code += '\t'; i++; break;
                                    case '\\': decoded_code += '\\'; i++; break;
                                    case '"': decoded_code += '"'; i++; break;
                                    default: decoded_code += encoded_code[i]; break;
                                }
                            } else { decoded_code += encoded_code[i]; }
                        }
                        string result = llParseToJSON(decoded_code);
                        string response = "HTTP/1.1 200 OK\r\n";
                        response += "Content-Type: application/json\r\n";
                        response += "Access-Control-Allow-Origin: *\r\n";
                        response += "Content-Length: " + to_string(result.length()) + "\r\n\r\n";
                        response += result;
                        send(client_fd, response.c_str(), response.length(), 0);
                    } else {
                        string response = "HTTP/1.1 400 Bad Request\r\n";
                        response += "Content-Type: text/plain; charset=utf-8\r\n";
                        string msg = "Missing 'code' field\n";
                        response += "Content-Length: " + to_string(msg.length()) + "\r\n\r\n" + msg;
                        send(client_fd, response.c_str(), response.length(), 0);
                    }
                }
            } else if (request.find("POST /lrparse") != string::npos) {
                size_t json_start = request.find("\r\n\r\n");
                if (json_start != string::npos) {
                    string json_str = request.substr(json_start + 4);
                    size_t code_start = json_str.find("\"code\":\"");
                    if (code_start != string::npos) {
                        code_start += 8;
                        size_t code_end = code_start; bool escaped = false;
                        while (code_end < json_str.size()) {
                            char ch = json_str[code_end];
                            if (escaped) { escaped = false; }
                            else if (ch == '\\') { escaped = true; }
                            else if (ch == '"') { break; }
                            code_end++;
                        }
                        string encoded_code = json_str.substr(code_start, code_end - code_start);
                        string decoded_code;
                        for (size_t i = 0; i < encoded_code.length(); i++) {
                            if (encoded_code[i] == '\\' && i + 1 < encoded_code.length()) {
                                switch (encoded_code[i + 1]) {
                                    case 'n': decoded_code += '\n'; i++; break;
                                    case 'r': decoded_code += '\r'; i++; break;
                                    case 't': decoded_code += '\t'; i++; break;
                                    case '\\': decoded_code += '\\'; i++; break;
                                    case '"': decoded_code += '"'; i++; break;
                                    default: decoded_code += encoded_code[i]; break;
                                }
                            } else { decoded_code += encoded_code[i]; }
                        }
                        string result = lrParseToJSON(decoded_code);
                        string response = "HTTP/1.1 200 OK\r\n";
                        response += "Content-Type: application/json\r\n";
                        response += "Access-Control-Allow-Origin: *\r\n";
                        response += "Content-Length: " + to_string(result.length()) + "\r\n\r\n";
                        response += result;
                        send(client_fd, response.c_str(), response.length(), 0);
                    } else {
                        string response = "HTTP/1.1 400 Bad Request\r\n";
                        response += "Content-Type: text/plain; charset=utf-8\r\n";
                        string msg = "Missing 'code' field\n";
                        response += "Content-Length: " + to_string(msg.length()) + "\r\n\r\n" + msg;
                        send(client_fd, response.c_str(), response.length(), 0);
                    }
                }
            } else if (request.find("POST /translate") != string::npos) {
                size_t json_start = request.find("\r\n\r\n");
                if (json_start != string::npos) {
                    string json_str = request.substr(json_start + 4);
                    size_t code_start = json_str.find("\"code\":\"");
                    if (code_start != string::npos) {
                        code_start += 8;
                        size_t code_end = code_start; bool escaped = false;
                        while (code_end < json_str.size()) {
                            char ch = json_str[code_end];
                            if (escaped) { escaped = false; }
                            else if (ch == '\\') { escaped = true; }
                            else if (ch == '"') { break; }
                            code_end++;
                        }
                        string encoded_code = json_str.substr(code_start, code_end - code_start);
                        string decoded_code;
                        for (size_t i = 0; i < encoded_code.length(); i++) {
                            if (encoded_code[i] == '\\' && i + 1 < encoded_code.length()) {
                                switch (encoded_code[i + 1]) {
                                    case 'n': decoded_code += '\n'; i++; break;
                                    case 'r': decoded_code += '\r'; i++; break;
                                    case 't': decoded_code += '\t'; i++; break;
                                    case '\\': decoded_code += '\\'; i++; break;
                                    case '"': decoded_code += '"'; i++; break;
                                    default: decoded_code += encoded_code[i]; break;
                                }
                            } else { decoded_code += encoded_code[i]; }
                        }
                        string result = translationToJSON(decoded_code);
                        string response = "HTTP/1.1 200 OK\r\n";
                        response += "Content-Type: application/json\r\n";
                        response += "Access-Control-Allow-Origin: *\r\n";
                        response += "Content-Length: " + to_string(result.length()) + "\r\n\r\n";
                        response += result;
                        send(client_fd, response.c_str(), response.length(), 0);
                    } else {
                        string response = "HTTP/1.1 400 Bad Request\r\n";
                        response += "Content-Type: text/plain; charset=utf-8\r\n";
                        string msg = "Missing 'code' field\n";
                        response += "Content-Length: " + to_string(msg.length()) + "\r\n\r\n" + msg;
                        send(client_fd, response.c_str(), response.length(), 0);
                    }
                }
            } else {
                // 提供静态文件
                string filepath = "static" + path;
                string content = readFile(filepath);
                
                string response;
                if (!content.empty()) {
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: " + getMimeType(filepath) + "\r\n";
                    response += "Content-Length: " + to_string(content.length()) + "\r\n";
                    response += "\r\n";
                    response += content;
                } else {
                    // 文件未找到
                    response = "HTTP/1.1 404 Not Found\r\n";
                    response += "Content-Type: text/html; charset=utf-8\r\n";
                    response += "\r\n";
                    response += "<html><body><h1>404 Not Found</h1><p>文件 " + path + " 未找到</p></body></html>";
                }
                
                send(client_fd, response.c_str(), response.length(), 0);
            }
        }
        
#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
}

int main() {
    startServer();
    return 0;
}
