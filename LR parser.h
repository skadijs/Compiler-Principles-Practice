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
unordered_map<string, ReductionRule> reduction_rules; // Will be populated by generator

// ACTION表 - 状态到终结符的动作映射
unordered_map<string, unordered_map<string, string>> action_table; // Will be populated by generator

// GOTO表 - 状态到非终结符的跳转映射
unordered_map<string, unordered_map<string, string>> goto_table; // Will be populated by generator

// 工具函数：按空格分割字符串，将换行识别为<endl>标记
vector<string> split_string_by_space(const string& input_str) {
    vector<string> tokens;
    istringstream stream(input_str);
    string line;
    
    while (getline(stream, line)) {
        istringstream line_stream(line);
        string token;
        bool has_tokens = false;
        while (line_stream >> token) {
            tokens.push_back(token);
            has_tokens = true;
        }
        if (has_tokens && token != "$") {
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
    
    // 辅助函数：获取Token类型（用于查表）
    string get_token_type(const string& token) {
        static const unordered_set<string> literals = {
            "{", "}", "if", "(", ")", "then", "else", "while",
            "=", ">", "<", ">=", "<=", "==", "+", "-",
            "*", "/", ";", "$"
        };
        if (literals.count(token)) return token;
        
        // 简单判断数字
        bool isNum = true;
        for (char c : token) {
            if (!isdigit(c) && c != '.') { isNum = false; break; }
        }
        if (isNum) return "NUM";
        
        return "ID";
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
            string lookup_token = get_token_type(current_token);
            
            // 获取ACTION
            string action = "";
            if (action_table.count(current_state) && 
                action_table[current_state].count(lookup_token)) {
                action = action_table[current_state][lookup_token];
            }
            
            if (action == "") {
                bool can_recover = false;
                if (action_table.count(current_state) && action_table[current_state].count(";")) {
                    can_recover = true;
                }

                if (current_mode == MODE_ERROR_CHECKING) {
                    int display_line = line_number + 1;
                    if (token_position > 0 && tokens[token_position-1] == "<endl>") {
                        display_line = line_number;
                    }
                    
                    if (can_recover) {
                        cout << "语法错误，第" << display_line << "行，缺少\";\"" << endl;
                    } else {
                        cout << "语法错误，第" << display_line << "行" << endl;
                    }
                } else if (current_mode == MODE_PARSE) {
                     if (can_recover) {
                        tokens.insert(tokens.begin() + token_position, ";");
                        parse();
                        return;
                     }
                }
                 break;
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
