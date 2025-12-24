#ifndef TABLE_GENERATOR_H
#define TABLE_GENERATOR_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <sstream>

using namespace std;

// --- LL(1) Table Generation ---

// Check if a symbol is a terminal
bool isTerminal(const string& s, const set<string>& terminals) {
    return terminals.count(s) || s == "E"; // E is epsilon
}

// Compute First set for a sequence of symbols
set<string> computeFirstSeq(const vector<string>& seq, 
                          map<string, set<string>>& firstSets, 
                          const set<string>& terminals) {
    set<string> result;
    bool allDeriveEpsilon = true;
    
    for (const string& sym : seq) {
        if (sym == "E") {
            continue;
        }
        
        if (terminals.count(sym)) {
            result.insert(sym);
            allDeriveEpsilon = false;
            break;
        } else {
            // Non-terminal
            const set<string>& f = firstSets[sym];
            bool derivesEpsilon = false;
            for (const string& f_sym : f) {
                if (f_sym == "E") derivesEpsilon = true;
                else result.insert(f_sym);
            }
            
            if (!derivesEpsilon) {
                allDeriveEpsilon = false;
                break;
            }
        }
    }
    
    if (allDeriveEpsilon) {
        result.insert("E");
    }
    
    return result;
}

void generateLLTableData(
    const map<string, vector<vector<string>>>& grammar,
    const set<string>& terminals,
    map<pair<string, string>, vector<string>>& table
) {
    // 1. Initialize First Sets
    map<string, set<string>> firstSets;
    for (const auto& pair : grammar) firstSets[pair.first] = {};
    
    // 2. Compute First Sets (Fixed Point)
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : grammar) {
            string lhs = rule.first;
            size_t initialSize = firstSets[lhs].size();
            
            for (const auto& rhs : rule.second) {
                // Compute First(RHS)
                set<string> rhsFirst;
                bool allEpsilon = true;
                for (const string& sym : rhs) {
                    if (sym == "E") continue;
                    if (terminals.count(sym)) {
                        rhsFirst.insert(sym);
                        allEpsilon = false;
                        break;
                    } else {
                        // Non-terminal
                        bool symDerivesEpsilon = false;
                        for (const string& f : firstSets[sym]) {
                            if (f == "E") symDerivesEpsilon = true;
                            else rhsFirst.insert(f);
                        }
                        if (!symDerivesEpsilon) {
                            allEpsilon = false;
                            break;
                        }
                    }
                }
                if (allEpsilon) rhsFirst.insert("E");
                
                // Add to First(LHS)
                firstSets[lhs].insert(rhsFirst.begin(), rhsFirst.end());
            }
            
            if (firstSets[lhs].size() > initialSize) changed = true;
        }
    }
    
    // 3. Compute Follow Sets
    map<string, set<string>> followSets;
    followSets["program"].insert("$"); // Start symbol gets $
    
    changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : grammar) {
            string A = rule.first;
            for (const auto& rhs : rule.second) {
                for (size_t i = 0; i < rhs.size(); ++i) {
                    string B = rhs[i];
                    if (terminals.count(B) || B == "E") continue;
                    
                    // B is non-terminal
                    size_t initialSize = followSets[B].size();
                    
                    // Compute First(beta) where A -> alpha B beta
                    vector<string> beta;
                    for (size_t j = i + 1; j < rhs.size(); ++j) beta.push_back(rhs[j]);
                    
                    set<string> firstBeta = computeFirstSeq(beta, firstSets, terminals);
                    
                    bool betaDerivesEpsilon = false;
                    for (const string& f : firstBeta) {
                        if (f == "E") betaDerivesEpsilon = true;
                        else followSets[B].insert(f);
                    }
                    
                    if (betaDerivesEpsilon || beta.empty()) {
                        followSets[B].insert(followSets[A].begin(), followSets[A].end());
                    }
                    
                    if (followSets[B].size() > initialSize) changed = true;
                }
            }
        }
    }
    
    // 4. Construct Parse Table
    table.clear();
    for (const auto& rule : grammar) {
        string A = rule.first;
        for (const auto& rhs : rule.second) {
            set<string> firstAlpha = computeFirstSeq(rhs, firstSets, terminals);
            
            for (const string& a : firstAlpha) {
                if (a != "E") {
                    table[{A, a}] = rhs;
                } else {
                    for (const string& b : followSets[A]) {
                        table[{A, b}] = rhs; // A -> E
                    }
                }
            }
        }
    }
    
    cout << "LL(1) Table Generated. Entries: " << table.size() << endl;
}

// --- LR Table Generation ---

struct LRItem {
    int ruleIndex; // Index in the list of all productions
    int dotPos;
    string lhs;
    vector<string> rhs;
    
    bool operator<(const LRItem& other) const {
        if (ruleIndex != other.ruleIndex) return ruleIndex < other.ruleIndex;
        return dotPos < other.dotPos;
    }
    bool operator==(const LRItem& other) const {
        return ruleIndex == other.ruleIndex && dotPos == other.dotPos;
    }
};

struct LRRule {
    string lhs;
    vector<string> rhs;
};

// Helper to hash LRItem for unordered_set
struct LRItemHash {
    size_t operator()(const LRItem& item) const {
        return hash<int>()(item.ruleIndex) ^ hash<int>()(item.dotPos);
    }
};

// Compute Closure of an item set
set<LRItem> computeClosure(const set<LRItem>& items, const vector<LRRule>& rules) {
    set<LRItem> closure = items;
    bool changed = true;
    
    while (changed) {
        changed = false;
        size_t initialSize = closure.size();
        
        for (const auto& item : closure) {
            if (item.dotPos < item.rhs.size()) {
                string B = item.rhs[item.dotPos];
                // Check if B is a non-terminal (appears on LHS of some rule)
                for (int i = 0; i < rules.size(); ++i) {
                    if (rules[i].lhs == B) {
                        LRItem newItem = {i, 0, B, rules[i].rhs};
                        if (closure.find(newItem) == closure.end()) {
                            closure.insert(newItem);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    return closure;
}

// Compute Goto(I, X)
set<LRItem> computeGoto(const set<LRItem>& items, const string& X, const vector<LRRule>& rules) {
    set<LRItem> j;
    for (const auto& item : items) {
        if (item.dotPos < item.rhs.size() && item.rhs[item.dotPos] == X) {
            LRItem newItem = item;
            newItem.dotPos++;
            j.insert(newItem);
        }
    }
    return computeClosure(j, rules);
}

void generateLRTableData(
    const multimap<string, string>& rawRules,
    unordered_map<string, unordered_map<string, string>>& actionTable,
    unordered_map<string, unordered_map<string, string>>& gotoTable,
    unordered_map<string, ReductionRule>& reductionRules
) {
    // 1. Preprocess rules into vector
    vector<LRRule> rules;
    // Add augmented start rule: program' -> program
    rules.push_back({"program'", {"program"}});
    
    // Convert multimap to vector and tokenize RHS
    for (const auto& p : rawRules) {
        LRRule rule;
        rule.lhs = p.first;
        stringstream ss(p.second);
        string sym;
        while (ss >> sym) {
            if (sym != "E") // E is epsilon, empty RHS
                rule.rhs.push_back(sym);
        }
        rules.push_back(rule);
    }
    
    // Fill reductionRules map (r0, r1, ...)
    reductionRules.clear();
    for (int i = 0; i < rules.size(); ++i) {
        string key = "r" + to_string(i);
        reductionRules[key] = {rules[i].lhs, (int)rules[i].rhs.size()};
    }
    
    // 2. Build Canonical Collection
    vector<set<LRItem>> states;
    map<set<LRItem>, int> stateIndex;
    
    // Initial state: Closure({program' -> . program})
    set<LRItem> startItemSet;
    startItemSet.insert({0, 0, "program'", {"program"}});
    set<LRItem> startState = computeClosure(startItemSet, rules);
    
    states.push_back(startState);
    stateIndex[startState] = 0;
    
    actionTable.clear();
    gotoTable.clear();
    
    // Collect all symbols (terminals and non-terminals)
    set<string> symbols;
    for (const auto& r : rules) {
        symbols.insert(r.lhs);
        for (const auto& s : r.rhs) symbols.insert(s);
    }
    
    int stateCount = 0;
    while (stateCount < states.size()) {
        set<LRItem> I = states[stateCount];
        string stateName = "s" + to_string(stateCount);
        
        // Handle reductions and accepts
        for (const auto& item : I) {
            if (item.dotPos == item.rhs.size()) {
                if (item.lhs == "program'") {
                    actionTable[stateName]["$"] = "acc";
                } else {
                    // SLR(1) Logic: Compute Follow sets for LR grammar
                     string ruleId = "r" + to_string(item.ruleIndex);
                     
                     // Since we don't have them pre-computed, let's compute them on the fly or beforehand.
                    // For simplicity in this generated file, we'll assume a helper function.
                    // We need to re-compute First/Follow for the LR grammar structure.
                    
                    // Re-use the LL logic but adapted for LRRule structure
                    static map<string, set<string>> lrFollowSets;
                    static bool setsComputed = false;
                    
                    if (!setsComputed) {
                        // Convert LRRule to map<string, vector<vector<string>>> for compatibility
                        map<string, vector<vector<string>>> grammarMap;
                        set<string> terminals;
                         vector<string> allTerminals = {
                            "{", "}", "if", "(", ")", "then", "else", "while",
                            "ID", "=", ">", "<", ">=", "<=", "==", "+", "-",
                            "*", "/", "NUM", ";", "$"
                        };
                        for(const string& t : allTerminals) terminals.insert(t);

                        for (const auto& r : rules) {
                            grammarMap[r.lhs].push_back(r.rhs);
                        }
                        
                        // 1. First Sets
                        map<string, set<string>> firstSets;
                         for (const auto& pair : grammarMap) firstSets[pair.first] = {};
                        
                        bool changed = true;
                        while (changed) {
                            changed = false;
                            for (const auto& rule : grammarMap) {
                                string lhs = rule.first;
                                size_t init = firstSets[lhs].size();
                                for (const auto& rhs : rule.second) {
                                    set<string> rhsFirst = computeFirstSeq(rhs, firstSets, terminals);
                                    firstSets[lhs].insert(rhsFirst.begin(), rhsFirst.end());
                                }
                                if (firstSets[lhs].size() > init) changed = true;
                            }
                        }
                        
                        // 2. Follow Sets
                        lrFollowSets["program'"].insert("$");
                        changed = true;
                        while (changed) {
                            changed = false;
                            for (const auto& rule : grammarMap) {
                                string A = rule.first;
                                for (const auto& rhs : rule.second) {
                                    for (size_t i = 0; i < rhs.size(); ++i) {
                                        string B = rhs[i];
                                        if (terminals.count(B)) continue;
                                        size_t init = lrFollowSets[B].size();
                                        
                                        vector<string> beta;
                                        for (size_t j = i + 1; j < rhs.size(); ++j) beta.push_back(rhs[j]);
                                        set<string> firstBeta = computeFirstSeq(beta, firstSets, terminals);
                                        
                                        bool eps = false;
                                        for(const string& s : firstBeta) {
                                            if(s == "E") eps = true;
                                            else lrFollowSets[B].insert(s);
                                        }
                                        if(eps || beta.empty()) {
                                            lrFollowSets[B].insert(lrFollowSets[A].begin(), lrFollowSets[A].end());
                                        }
                                        if(lrFollowSets[B].size() > init) changed = true;
                                    }
                                }
                            }
                        }
                        setsComputed = true;
                    }
                    
                    for (const string& la : lrFollowSets[item.lhs]) {
                        actionTable[stateName][la] = ruleId;
                    }
                }
            }
        }
        
        // Handle shifts and gotos
        for (const string& X : symbols) {
            set<LRItem> J = computeGoto(I, X, rules);
            if (!J.empty()) {
                if (stateIndex.find(J) == stateIndex.end()) {
                    stateIndex[J] = states.size();
                    states.push_back(J);
                }
                
                int nextStateId = stateIndex[J];
                string nextStateStr = "s" + to_string(nextStateId);
                
                // Determine if X is terminal or non-terminal
                bool isNonTerminal = false;
                for(const auto& r : rules) if(r.lhs == X) isNonTerminal = true;
                
                if (isNonTerminal) {
                    gotoTable[stateName][X] = nextStateStr;
                } else {
                    actionTable[stateName][X] = nextStateStr;
                }
            }
        }
        
        stateCount++;
    }
    
    cout << "LR Table Generated. States: " << states.size() << endl;
}

#endif