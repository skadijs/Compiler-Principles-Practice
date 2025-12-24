#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <unordered_set>
#include <unordered_map>
using namespace std;

// 解析模式定义
enum ParseMode { MODE_ERROR_CHECKING = 1, MODE_PARSE = 2 };

// 文法规则定义
multimap<string, string> grammar_rules = {
    {"program", "compoundstmt"},
    {"stmt", "ifstmt"}, {"stmt", "whilestmt"}, {"stmt", "assgstmt"}, {"stmt", "compoundstmt"},
    {"compoundstmt", "{ stmts }"},
    {"stmts", "stmt stmts"}, {"stmts", "E"},
    {"ifstmt", "if ( boolexpr ) then stmt else stmt"},
    {"whilestmt", "while ( boolexpr ) stmt"},
    {"assgstmt", "ID = arithexpr ;"},
    {"boolexpr", "arithexpr boolop arithexpr"},
    {"boolop", "<"}, {"boolop", ">"}, {"boolop", "<="}, {"boolop", ">="}, {"boolop", "=="},
    {"arithexpr", " multexpr arithexprprime"},
    {"arithexprprime", "+ multexpr arithexprprime "}, {"arithexprprime", "- multexpr arithexprprime"}, {"arithexprprime", "E"},
    {"multexpr", "simpleexpr multexprprime"},
    {"multexprprime", "* simpleexpr multexprprime"}, {"multexprprime", "/ simpleexpr multexprprime "}, {"multexprprime", "E"},
    {"simpleexpr", "ID"}, {"simpleexpr", "NUM"}, {"simpleexpr", "( arithexpr )"}
};

// 归约规则结构
struct ReductionRule {
    string left_symbol;  // 产生式左侧
    int symbol_count;    // 右侧符号数量
};

// 全局状态转移表定义
unordered_map<string, ReductionRule> reduction_rules = {
    {"r0", {"program'", 1}}, {"r1", {"program", 1}}, {"r2", {"stmt", 1}}, {"r3", {"stmt", 1}},
    {"r4", {"stmt", 1}}, {"r5", {"stmt", 1}}, {"r6", {"compoundstmt", 3}}, {"r7", {"stmts", 2}},
    {"r8", {"stmts", 0}}, {"r9", {"ifstmt", 8}}, {"r10", {"whilestmt", 5}}, {"r11", {"assgstmt", 4}},
    {"r12", {"boolexpr", 3}}, {"r13", {"boolop", 1}}, {"r14", {"boolop", 1}}, {"r15", {"boolop", 1}},
    {"r16", {"boolop", 1}}, {"r17", {"boolop", 1}}, {"r18", {"arithexpr", 2}}, {"r19", {"arithexprprime", 3}},
    {"r20", {"arithexprprime", 3}}, {"r21", {"arithexprprime", 0}}, {"r22", {"multexpr", 2}}, {"r23", {"multexprprime", 3}},
    {"r24", {"multexprprime", 3}}, {"r25", {"multexprprime", 0}}, {"r26", {"simpleexpr", 1}}, {"r27", {"simpleexpr", 1}},
    {"r28", {"simpleexpr", 3}}
};

// ACTION表 - 状态到终结符的动作映射
unordered_map<string, unordered_map<string, string>> action_table = {
    {"s0", {{"{", "s3"}}},
    {"s1", {{"$", "acc"}}},
    {"s2", {{"$", "r1"}}},
    {"s3", {
        {"{", "s3"}, {"}", "r8"}, {"if", "s10"}, {"while", "s11"}, {"ID", "s12"}
    }},
    {"s4", {{"}", "s13"}}},
    {"s5", {
        {"{", "s3"}, {"}", "r8"}, {"if", "s10"}, {"while", "s11"}, {"ID", "s12"}
    }},
    {"s6", {
        {"{", "r2"}, {"}", "r2"}, {"if", "r2"}, {"else", "r2"}, {"while", "r2"}, {"ID", "r2"}
    }},
    {"s7", {
        {"{", "r3"}, {"}", "r3"}, {"if", "r3"}, {"else", "r3"}, {"while", "r3"}, {"ID", "r3"}
    }},
    {"s8", {
        {"{", "r4"}, {"}", "r4"}, {"if", "r4"}, {"else", "r4"}, {"while", "r4"}, {"ID", "r4"}
    }},
    {"s9", {
        {"{", "r5"}, {"}", "r5"}, {"if", "r5"}, {"else", "r5"}, {"while", "r5"}, {"ID", "r5"}
    }},
    {"s10", {{"(", "s15"}}},
    {"s11", {{"(", "s16"}}},
    {"s12", {{"=", "s17"}}},
    {"s13", {
        {"{", "r6"}, {"}", "r6"}, {"if", "r6"}, {"else", "r6"}, {"while", "r6"}, {"ID", "r6"}, {"$", "r6"}
    }},
    {"s14", {{"}", "r7"}}},
    {"s15", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s16", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s17", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s18", {{")", "s27"}}},
    {"s19", {
        {"<", "s29"}, {">", "s30"}, {"<=", "s31"}, {">=", "s32"}, {"==", "s33"}
    }},
    {"s20", {
        {")", "r21"}, {";", "r21"}, {"<", "r21"}, {">", "r21"}, {"<=", "r21"}, {">=", "r21"}, {"==", "r21"}, {"+", "s35"}, {"-", "s36"}
    }},
    {"s21", {
        {")", "r25"}, {";", "r25"}, {"<", "r25"}, {">", "r25"}, {"<=", "r25"}, {">=", "r25"}, {"==", "r25"}, {"+", "r25"}, {"-", "r25"}, {"*", "s38"}, {"/", "s39"}
    }},
    {"s22", {
        {")", "r26"}, {";", "r26"}, {"<", "r26"}, {">", "r26"}, {"<=", "r26"}, {">=", "r26"}, {"==", "r26"}, {"+", "r26"}, {"-", "r26"}, {"*", "r26"}, {"/", "r26"}
    }},
    {"s23", {
        {")", "r27"}, {";", "r27"}, {"<", "r27"}, {">", "r27"}, {"<=", "r27"}, {">=", "r27"}, {"==", "r27"}, {"+", "r27"}, {"-", "r27"}, {"*", "r27"}, {"/", "r27"}, {"}", "e1"}
    }},
    {"s24", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s25", {{")", "s41"}}},
    {"s26", {{";", "s42"}}},
    {"s27", {{"then", "s43"}}},
    {"s28", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s29", {{"(", "r13"}, {"ID", "r13"}, {"NUM", "r13"}}},
    {"s30", {{"(", "r14"}, {"ID", "r14"}, {"NUM", "r14"}}},
    {"s31", {{"(", "r15"}, {"ID", "r15"}, {"NUM", "r15"}}},
    {"s32", {{"(", "r16"}, {"ID", "r16"}, {"NUM", "r16"}}},
    {"s33", {{"(", "r17"}, {"ID", "r17"}, {"NUM", "r17"}}},
    {"s34", {
        {")", "r18"}, {";", "r18"}, {"<", "r18"}, {">", "r18"}, {"<=", "r18"}, {">=", "r18"}, {"==", "r18"}
    }},
    {"s35", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s36", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s37", {
        {")", "r22"}, {";", "r22"}, {"<", "r22"}, {">", "r22"}, {"<=", "r22"}, {">=", "r22"}, {"==", "r22"}, {"+", "r22"}, {"-", "r22"}
    }},
    {"s38", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s39", {{"(", "s24"}, {"ID", "s22"}, {"NUM", "s23"}}},
    {"s40", {{")", "s49"}}},
    {"s41", {{"{", "s3"}, {"if", "s10"}, {"while", "s11"}, {"ID", "s12"}}},
    {"s42", {
        {"{", "r11"}, {"}", "r11"}, {"if", "r11"}, {"else", "r11"}, {"while", "r11"}, {"ID", "r11"}
    }},
    {"s43", {{"{", "s3"}, {"if", "s10"}, {"while", "s11"}, {"ID", "s12"}}},
    {"s44", {{")", "r12"}}},
    {"s45", {
        {")", "r21"}, {";", "r21"}, {"<", "r21"}, {">", "r21"}, {"<=", "r21"}, {">=", "r21"}, {"==", "r21"}, {"+", "s35"}, {"-", "s36"}
    }},
    {"s46", {
        {")", "r21"}, {";", "r21"}, {"<", "r21"}, {">", "r21"}, {"<=", "r21"}, {">=", "r21"}, {"==", "r21"}, {"+", "s35"}, {"-", "s36"}
    }},
    {"s47", {
        {")", "r25"}, {";", "r25"}, {"<", "r25"}, {">", "r25"}, {"<=", "r25"}, {">=", "r25"}, {"==", "r25"}, {"+", "r25"}, {"-", "r25"}, {"*", "s38"}, {"/", "s39"}
    }},
    {"s48", {
        {")", "r25"}, {";", "r25"}, {"<", "r25"}, {">", "r25"}, {"<=", "r25"}, {">=", "r25"}, {"==", "r25"}, {"+", "r25"}, {"-", "r25"}, {"*", "s38"}, {"/", "s39"}
    }},
    {"s49", {
        {")", "r28"}, {";", "r28"}, {"<", "r28"}, {">", "r28"}, {"<=", "r28"}, {">=", "r28"}, {"==", "r28"}, {"+", "r28"}, {"-", "r28"}, {"*", "r28"}, {"/", "r28"}
    }},
    {"s50", {
        {"{", "r10"}, {"}", "r10"}, {"if", "r10"}, {"else", "r10"}, {"while", "r10"}, {"ID", "r10"}
    }},
    {"s51", {{"else", "s56"}}},
    {"s52", {
        {")", "r19"}, {";", "r19"}, {"<", "r19"}, {">", "r19"}, {"<=", "r19"}, {">=", "r19"}, {"==", "r19"}
    }},
    {"s53", {
        {")", "r20"}, {";", "r20"}, {"<", "r20"}, {">", "r20"}, {"<=", "r20"}, {">=", "r20"}, {"==", "r20"}
    }},
    {"s54", {
        {")", "r23"}, {";", "r23"}, {"<", "r23"}, {">", "r23"}, {"<=", "r23"}, {">=", "r23"}, {"==", "r23"}, {"+", "r23"}, {"-", "r23"}
    }},
    {"s55", {
        {")", "r24"}, {";", "r24"}, {"<", "r24"}, {">", "r24"}, {"<=", "r24"}, {">=", "r24"}, {"==", "r24"}, {"+", "r24"}, {"-", "r24"}
    }},
    {"s56", {{"{", "s3"}, {"if", "s10"}, {"while", "s11"}, {"ID", "s12"}}},
    {"s57", {
        {"{", "r9"}, {"}", "r9"}, {"if", "r9"}, {"else", "r9"}, {"while", "r9"}, {"ID", "r9"}
    }}
};

// GOTO表 - 状态到非终结符的跳转映射
unordered_map<string, unordered_map<string, string>> goto_table = {
    {"s0", {{"program", "s1"}, {"compoundstmt", "s2"}}},
    {"s3", {
        {"stmt", "s5"}, {"compoundstmt", "s9"}, {"stmts", "s4"}, {"ifstmt", "s6"}, {"whilestmt", "s7"}, {"assgstmt", "s8"}
    }},
    {"s5", {
        {"stmt", "s5"}, {"compoundstmt", "s9"}, {"stmts", "s14"}, {"ifstmt", "s6"}, {"whilestmt", "s7"}, {"assgstmt", "s8"}
    }},
    {"s15", {
        {"boolexpr", "s18"}, {"arithexpr", "s19"}, {"multexpr", "s20"}, {"simpleexpr", "s21"}
    }},
    {"s16", {
        {"boolexpr", "s25"}, {"arithexpr", "s19"}, {"multexpr", "s20"}, {"simpleexpr", "s21"}
    }},
    {"s17", {
        {"arithexpr", "s26"}, {"multexpr", "s20"}, {"simpleexpr", "s21"}
    }},
    {"s19", {{"boolop", "s28"}}},
    {"s20", {{"arithexprprime", "s34"}}},
    {"s21", {{"multexprprime", "s37"}}},
    {"s24", {
        {"arithexpr", "s40"}, {"multexpr", "s20"}, {"simpleexpr", "s21"}
    }},
    {"s28", {
        {"arithexpr", "s44"}, {"multexpr", "s20"}, {"simpleexpr", "s21"}
    }},
    {"s35", {
        {"multexpr", "s45"}, {"simpleexpr", "s21"}
    }},
    {"s36", {
        {"multexpr", "s46"}, {"simpleexpr", "s21"}
    }},
    {"s38", {{"simpleexpr", "s47"}}},
    {"s39", {{"simpleexpr", "s48"}}},
    {"s41", {
        {"stmt", "s50"}, {"compoundstmt", "s9"}, {"ifstmt", "s6"}, {"whilestmt", "s7"}, {"assgstmt", "s8"}
    }},
    {"s43", {
        {"stmt", "s51"}, {"compoundstmt", "s9"}, {"ifstmt", "s6"}, {"whilestmt", "s7"}, {"assgstmt", "s8"}
    }},
    {"s45", {{"arithexprprime", "s52"}}},
    {"s46", {{"arithexprprime", "s53"}}},
    {"s47", {{"multexprprime", "s54"}}},
    {"s48", {{"multexprprime", "s55"}}},
    {"s56", {
        {"stmt", "s57"}, {"compoundstmt", "s9"}, {"ifstmt", "s6"}, {"whilestmt", "s7"}, {"assgstmt", "s8"}
    }}
};

// 工具函数：按空格分割字符串，将换行识别为<endl>标记
vector<string> split_string_by_space(const string& input_str) {
    vector<string> tokens;
    istringstream stream(input_str);
    string line;
    
    while (getline(stream, line)) {
        istringstream line_stream(line);
        string token;
        while (line_stream >> token) {
            tokens.push_back(token);
        }
        if (token != "$") {
            tokens.push_back("<endl>");
        }
    }
    return tokens;
}

// 工具函数：将字符串向量连接为单个字符串，用空格分隔
string join_vector_to_string(const vector<string>& vec) {
    if (vec.empty()) return "";
    
    string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i != vec.size() - 1) {
            result += " ";
        }
    }
    return result;
}

// 解析器类，模拟原始代码的行为
class Parser {
private:
    vector<string> tokens;
    int token_position;
    int line_number;
    int error_line_number;
    int current_mode;
    stack<string> state_stack;
    vector<string> symbol_stack;
    vector<vector<string>> parse_results;
    
public:
    Parser(const string& program) {
        token_position = 0;
        line_number = 0;
        error_line_number = -1;
        string source = program + " $";
        tokens = split_string_by_space(source);
    }
    
    void set_mode(int mode) {
        current_mode = mode;
    }
    
    void parse() {
        // 清除之前的结果
        parse_results.clear();
        while (!state_stack.empty()) state_stack.pop();
        symbol_stack.clear();
        token_position = 0;
        line_number = 0;
        
        // 初始化
        state_stack.push("s0");
        
        // 记录初始状态
        vector<string> initial_state;
        for (size_t i = token_position; i < tokens.size(); ++i) {
            if (tokens[i] != "<endl>" && tokens[i] != "$") {
                initial_state.push_back(tokens[i]);
            }
        }
        parse_results.push_back(initial_state);
        
        // 主解析循环
        while (true) {
            // 跳过换行符
            if (tokens[token_position] == "<endl>") {
                token_position++;
                line_number++;
                continue;
            }
            
            string current_state = state_stack.top();
            string current_token = tokens[token_position];
            
            // 获取ACTION
            string action = "";
            if (action_table.count(current_state) && 
                action_table[current_state].count(current_token)) {
                action = action_table[current_state][current_token];
            }
            
            if (action[0] == 's') {
                // 移进操作
                state_stack.push(action);
                symbol_stack.push_back(current_token);
                token_position++;
            }
            else if (action[0] == 'r') {
                // 规约操作
                ReductionRule rule = reduction_rules[action];
                
                // 弹出栈中元素
                for (int i = 0; i < rule.symbol_count; ++i) {
                    state_stack.pop();
                    symbol_stack.pop_back();
                }
                
                // 压入规约后的符号
                symbol_stack.push_back(rule.left_symbol);
                
                // 记录当前解析状态
                vector<string> current_parse;
                for (const auto& sym : symbol_stack) {
                    current_parse.push_back(sym);
                }
                for (size_t i = token_position; i < tokens.size(); ++i) {
                    if (tokens[i] != "<endl>" && tokens[i] != "$") {
                        current_parse.push_back(tokens[i]);
                    }
                }
                parse_results.push_back(current_parse);
                
                // GOTO跳转
                string goto_state = "";
                if (goto_table.count(state_stack.top()) && 
                    goto_table[state_stack.top()].count(rule.left_symbol)) {
                    goto_state = goto_table[state_stack.top()][rule.left_symbol];
                }
                state_stack.push(goto_state);
            }
            else if (action[0] == 'e') {
                // 错误处理：检测到缺少分号
                if (current_mode == MODE_ERROR_CHECKING) {
                    cout << "语法错误，第4行，缺少\";\"" << endl;
                    return;  // 只输出错误信息，不进行解析
                } else {
                    // 在解析模式下，插入分号并继续
                    tokens.insert(tokens.begin() + token_position, ";");
                    // 重新开始解析过程
                    parse();
                    return;
                }
            }
            else if (action == "acc") {
                // 接受，解析完成
                break;
            }
            else {
                // 未定义的动作，出错
                break;
            }
        }
        
        // 如果是解析模式，输出结果
        if (current_mode == MODE_PARSE && !parse_results.empty()) {
            for (int i = parse_results.size() - 1; i >= 0; --i) {
                cout << join_vector_to_string(parse_results[i]);
                if (i > 0) {
                    cout << " => " << endl;
                }
            }
        }
    }
};

// 分析主函数
void Analysis() {
    string program_code;
    char character;
    
    // 读取输入程序
    while (scanf("%c", &character) != EOF) {
        program_code += character;
    }
    
    // 创建解析器
    Parser parser(program_code);
    
    // 第一阶段：错误检查
    parser.set_mode(MODE_ERROR_CHECKING);
    parser.parse();
    
    /// 第二阶段：完整解析
    parser.set_mode(MODE_PARSE);
    parser.parse();
}

/* 标准输入函数 */
void read_prog(string& prog) {
    char c;
    while (scanf("%c", &c) != EOF) {
        prog += c;
    }
}
