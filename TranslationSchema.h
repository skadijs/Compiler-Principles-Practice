#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

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
        
        // Check for compound assignment
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


/* 不要修改这个标准输入函数 */
void read_prog(string& prog)
{
    char c;
    while(scanf("%c",&c)!=EOF){
        prog += c;
    }
}
/* 你可以添加其他函数 */

void Analysis()
{
    string prog;
    read_prog(prog);
    /* 骚年们 请开始你们的表演 */
    /********* Begin *********/
    Translator t(prog);
    t.translate();
    /********* End *********/
    
}