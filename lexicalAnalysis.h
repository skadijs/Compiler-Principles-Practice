#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include<algorithm>
using namespace std;

//枚举token存在的类型
unordered_map<string,int>typeName={
    {"auto",1}, {"break", 2}, {"case",3}, {"char",4}, {"const",5},{"continue",6}, {"default",7}, {"do",8}, {"double",9}, {"else",      10},{"enum",11}, {"extern",12}, {"float",13}, {"for",14}, {"goto",15},{"if",16}, {"int",17}, {"long",18}, {"register",19}, {"return",20},{"short",21}, {"signed",22}, {"sizeof",23}, {"static",24}, {"struct",25},{"switch",26}, {"typedef",27}, {"union",28}, {"unsigned",29}, {"void",30},{"volatile",   31}, {"while",32}, {"-",33}, {"--",34}, {"-=",35},{"->",36}, {"!",37}, {"!=",38}, {"%",39}, {"%=",40},{"&",41}, {"&&",42}, {"&=",43}, {"(",44}, {")",45},{"*",46}, {"*=",47}, {",",48}, {".", 49}, {"/",50},{"/=",51}, {":",52}, {";",53}, {"?",54}, {"[",55},{"]",56}, {"^",57}, {"^=",58}, {"{",59}, {"|",60},{"||",61}, {"|=",62}, {"}",63}, {"~",64}, {"+",65},{"++",66}, {"+=",67}, {"<",68}, {"<<",69}, {"<<=",70},{"<=",71}, {"=",72}, {"==",73}, {">",74}, {">=",75},{">>",76}, {">>=",77}, {"\"",78}, {"Comment",79}, {"Constant",80},{"Identifier", 81}
};

//关键字数组
vector<string>Keywords={
"auto", "break", "case", "char","const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
};

// 字符类型枚举
enum CharType {
    Alphabet, Number, Symbol, Slash, Blank, Quote, Endl, Star, Dot, Eof
};

// 全局变量声明
extern string s;
extern int pos;
extern int tokenCount;
extern short quoteStatus;

// 函数声明
CharType peekChar();
char getChar();
void printToken(const string& lexeme, short type);
void getIdentifierOrKeyword();
void getNumber();
void getOperator();
void getCommentOrOperator();
void getQuoteContent();

// 全局变量定义
string s;
int pos = 0;
int tokenCount = 0;
short quoteStatus = 0;

// 查看下一个字符的类型
CharType peekChar() {
    if (pos >= s.length()) return Eof;

    char ch = s[pos];
    if (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z') return Alphabet;
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
    if (pos >= s.length()) return '\0';
    return s[pos++];
}

// 打印token
void printToken(const string& lexeme, short type) {
    if (tokenCount != 1) printf("\n");
    printf("%d: <%s,%d>", tokenCount, lexeme.c_str(), type);
}

// 获取关键字或标识符
void getIdentifierOrKeyword() {
    string lexeme = "";
    while (pos < s.length()) {
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

    if (isKeyword) {
        printToken(lexeme, typeName[lexeme]);
    } else {
        printToken(lexeme, 81); // Identifier
    }
}

// 获取数字常量
void getNumber() {
    string lexeme = "";
    bool hasDot = false;

    while (pos < s.length()) {
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

    printToken(lexeme, 80); // Constant
}

// 获取运算符
void getOperator() {
    string lexeme = "";
    lexeme += getChar();

    // 检查是否为多字符运算符
    if (pos < s.length()) {
        string twoChar = lexeme + s[pos];
        if (typeName.find(twoChar) != typeName.end()) {
            // 检查是否为三字符运算符
            if (pos + 1 < s.length()) {
                string threeChar = twoChar + s[pos + 1];
                if (typeName.find(threeChar) != typeName.end()) {
                    lexeme = threeChar;
                    pos += 2;
                    printToken(lexeme, typeName[lexeme]);
                    return;
                }
            }

            // 两个字符的运算符
            if (typeName.find(twoChar) != typeName.end()) {
                lexeme = twoChar;
                pos++;
            }
        }
    }

    printToken(lexeme, typeName[lexeme]);
}

// 处理注释或除法运算符
void getCommentOrOperator() {
    string lexeme = "/";
    pos++; // 跳过第一个'/'

    if (pos >= s.length()) {
        printToken(lexeme, typeName[lexeme]);
        return;
    }

    char nextChar = s[pos];

    // 单行注释
    if (nextChar == '/') {
        lexeme += '/';
        pos++;
        // 直接获取到行尾
        while (pos < s.length() && s[pos] != '\n') {
            lexeme += s[pos];
            pos++;
        }
        printToken(lexeme, 79); // Comment
    }
    // 多行注释
    else if (nextChar == '*') {
        lexeme += '*';
        pos++;
        bool commentEnded = false;

        while (pos < s.length() - 1) {
            if (s[pos] == '*' && s[pos + 1] == '/') {
                lexeme += "*/";
                pos += 2;
                commentEnded = true;
                break;
            }
            lexeme += s[pos];
            pos++;
        }

        if (!commentEnded && pos < s.length()) {
            lexeme += s[pos];
            pos++;
        }

        printToken(lexeme, 79); // Comment
    }
    // 除法运算符   
    else if (nextChar == '=') {
        lexeme += '=';
        pos++;
        printToken(lexeme, typeName[lexeme]);
    }
    // 普通除法运算符
    else {
        printToken(lexeme, typeName[lexeme]);
    }
}

// 处理引号内的内容
void getQuoteContent() {
    if (quoteStatus == 0) {
        // 开引号
        string lexeme = "\"";
        pos++;
        printToken(lexeme, typeName[lexeme]);
        quoteStatus = 1;
    } else if (quoteStatus == 1) {
        // 引号内的内容
        string lexeme = "";
        while (pos < s.length() && s[pos] != '"') {
            lexeme += s[pos];
            pos++;
        }
        printToken(lexeme, 81); // Identifier
        quoteStatus = 2;
    } else {
        // 闭引号
        string lexeme = "\"";
        pos++;
        printToken(lexeme, typeName[lexeme]);
        quoteStatus = 0;
    }
}

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
    s = prog;
    pos = 0;
    tokenCount = 0;
    quoteStatus = 0;

    while (pos < s.length()) {
        // 跳过空白字符
        while (pos < s.length()) {
            CharType ct = peekChar();
            if (ct == Blank || ct == Endl) {
                pos++;
            } else {
                break;
            }
        }

        if (pos >= s.length()) break;

        tokenCount++;
        CharType currentType = peekChar();

        if (quoteStatus > 0) {
            getQuoteContent();
        } else if (currentType == Alphabet) {
            getIdentifierOrKeyword();
        } else if (currentType == Number) {
            getNumber();
        } else if (currentType == Slash) {
            getCommentOrOperator();
        } else if (currentType == Quote) {
            getQuoteContent();
        } else if (currentType == Symbol) {
            getOperator();
        } else if (currentType == Dot) {
            getOperator();
        } else {
            pos++; // 跳过未知字符
        }
    }


    /********* End *********/

}
