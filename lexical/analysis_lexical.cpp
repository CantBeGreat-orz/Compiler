#include <cstring>
#include <fstream>
#include <vector>
#include <iostream>
#include <set>
#include <map>
#include <queue>

#define DIGITAL 'd' // digitals: 0 - 9
#define LETTER 'c'  // all uppercase and lowercase letters and underline: a - z, A - Z, _
#define EPSILON '$' // epsilon   
#define TERMINAL 'Y' //终态
#define INITIAL_STATUS 'S'  //初态
#define MAX_STATUS 128  //状态数上限
#define MAX_INPUT_LETTER 128    //输入字符的范围
#define KEYWORD 0
#define IDENTIFIER 1
#define CONSTANT 2
#define OPERATOR 3
#define DELIMITER 4

using namespace std;

const string grammar = "input/grammar.txt";
const string codetext = "input/codetext.txt";
const string tokens = "output/tokens.txt";
const string codetext_simple = "input/codetext_simple.txt";
const string tokens_simple = "output/tokens_simple.txt";

const set<string> keyword = {"include", "int", "real", "return", "main", "iostream", "string", "char", "while", "cstring"};
const set<char> opt_ch = {'+', '-', '*', '/', '!', '%', '~', '&', '|', '^', '=', '<', '>'}; //单个字符的运算符
const set<char> opt_double = {'+', '-',  '&', '|', '<', '>'}; //相同两个字符的运算符 and 带运算的赋值运算符的运算部分
const set<char> delimiter = {',', '(', ')', '{', '}', ';', '#'};
const set<char> separator = {' ', '\n', '\t', '\t'};

bool is_letter(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

bool is_digital(char ch)
{
    return ch <= '9' && ch >= '0';
}

class FA{
    set<char> input_letter;
    set<char> nfa[MAX_STATUS][MAX_INPUT_LETTER];
    // int dfa[MAX_STATUS][MAX_INPUT_LETTER];

    set<char> get_closure(const set<char> &s){
        set<char> ret, tmp;
        ret.insert(s.begin(),s.end());
        while(ret.size() != tmp.size()){
            tmp = ret;
            for(auto it = tmp.begin(); it != tmp.end(); ++it){
                //if(nfa[*it][EPSILON].empty()) continue;
                ret.insert(nfa[*it][EPSILON].begin(), nfa[*it][EPSILON].end());
            }
        }
        return ret;
    }

public:
    set<int> terminals;
    vector<vector<int>> dfa;
    void show_dfa(){
        cout<<"dfa: "<<endl;
        for(int i = 0; i < dfa.size(); ++i){
            for(int j = 0; j < dfa[i].size(); ++j){
                if(dfa[i][j] >= 0){
                    cout<<"("<<i<<")"<<"->"<<(char)j<<"("<<dfa[i][j]<<")"<<endl;
                }
            }
        }
    }

    void show_nfa(){
        cout<<"nfa: "<<endl;
        for(int i = 0; i < MAX_STATUS; ++i){
            for(int j = 0; j < MAX_INPUT_LETTER; ++j){
                for(auto it:nfa[i][j]){
                    cout<<(char)i<<"->"<<(char)j<<it<<endl;
                }
            }
        }
    }

    void show_input_letter(){
        cout<<"input_letter: ";
        for(auto it:input_letter){
            cout<<it<<" ";
        }
        cout<<endl;
    }

    void show_terminals(){
        cout<<"terminals: ";
        for(auto it:terminals){
            cout<<"("<<it<<")"<<" ";
        }
        cout<<endl;
    }

    bool cin_nfa(const string &grammar){
        for(int i = 0; i < MAX_STATUS; ++i){
            for(int j = 0; j < MAX_INPUT_LETTER; ++j){
                nfa[i][j].clear();
            }
        }
        input_letter.clear();
        ifstream f(grammar.c_str());
        if(!f.is_open()){
            cout<<"error: failed to open file---grammar "<<endl;
            return false;
        }
        char stmp;
        string tmp, ntmp;
        int i = 0;
        while(f>>stmp>>tmp>>ntmp){
            ++i;
            if(ntmp.size() == 1){
                if(ntmp[0] >= 'A' && ntmp[0] <= 'Z'){
                    nfa[stmp][EPSILON].emplace(ntmp[0]);
                }else{
                    nfa[stmp][ntmp[0]].emplace(TERMINAL);
                }
            }
            else if(ntmp.size() == 2){
                nfa[stmp][ntmp[0]].emplace(ntmp[1]);
                input_letter.emplace(ntmp[0]);
            }else{
                cout<<"error: wrong grammar input in line "<<i<<endl;
                return false;
            }
        }
        f.close();
        return true;
    }

    void to_dfa(){
        dfa = vector<vector<int>>(MAX_STATUS, vector<int>(MAX_INPUT_LETTER, -1));
        terminals.clear();
        map<set<char>, int> mp_closures;
        queue<set<char>> q_closures;
        set<char> tmp = get_closure(set<char>{INITIAL_STATUS});
        q_closures.push(tmp);
        mp_closures[tmp] = 0;
        if(tmp.count(TERMINAL)) terminals.emplace(0);
        int cnt = 1;
        while(!q_closures.empty()){
            set<char> &tmp = q_closures.front();
            for(auto iter = input_letter.begin(); iter != input_letter.end(); ++iter){
                set<char> next;
                for(auto it = tmp.begin(); it != tmp.end(); ++it){
                    next.insert(nfa[*it][*iter].begin(), nfa[*it][*iter].end());
                }
                if(next.empty()) continue;
                next = get_closure(next);
                if(!mp_closures.count(next)){
                    if(next.count(TERMINAL)) terminals.emplace(cnt);
                    mp_closures[next] = cnt++;
                    q_closures.emplace(next);
                }
                dfa[mp_closures[tmp]][*iter] = mp_closures[next];
            }
            q_closures.pop();
        }
    }
};

bool analyze(const vector<vector<int>> &dfa,const set<int> &terminals, const string &codetext, const string &tokens){
    FILE* f=fopen(codetext.c_str(),"r+");
    ofstream fout(tokens.c_str());
    if(!fout.is_open()){
        cout<<"error: failed to open file---tokens "<<endl;
        return false;
    }
    string str;
    char ch;
    ch = fgetc(f);
    int line_idx = 1;
    while(1){
        str.clear();
        while(separator.count(ch)){
            if(ch == '\n' || ch == '\r') ++line_idx;
            ch = fgetc(f);
            if(ch == EOF){
                break;
            }
        }
        if(ch == '\''){
            str += ch;
            bool flag = false;
            while((ch = fgetc(f)) != EOF){
                str += ch;
                if(ch == '\'' && !flag){
                    break;
                }
                if(flag) flag = false;
                else if(ch == '\\') flag = true;
            }
            if(str.size() > 4 || str.size() < 3 || str[str.size() - 1] != '\'' || (str.size() == 4 && str[1] != '\\')|| (str.size() == 3 && str[1] == '\\')){
                fout<<line_idx<<str<<" "<<"error: invalid char"<<endl;
                return false;
            }else{
                fout<<"("<<line_idx<<" , "<<str<<" , "<<CONSTANT<<" )"<<endl;
            }
            ch = fgetc(f);
        }else if(ch == '\"'){
            str += ch;
            bool flag = false;
            while((ch = fgetc(f)) != EOF){
                str += ch;
                if(ch == '\"' && !flag){
                    break;
                }
                if(flag) flag = false;
                else if(ch == '\\') flag = true;
            }
            if(str[str.size() - 1] != '\"' || flag){
                fout<<line_idx<<str<<" "<<"error: no matching double quotes  "<<endl;
                return false;
            }else{
                fout<<"("<<line_idx<<" , "<<str<<" , "<<CONSTANT<<" )"<<endl;
            }
            ch = fgetc(f);
        }else if(ch == EOF){
            break;
        }else if(delimiter.count(ch)){
            fout<<"("<<line_idx<<" , "<<ch<<" , "<<DELIMITER<<" )"<<endl;
            ch = fgetc(f);
        }else if(opt_ch.count(ch)){
            char tmp = ch;
            ch = fgetc(f);
            if(ch == '=' || (tmp == ch && opt_double.count(ch))){
                fout<<"("<<line_idx<<" , "<<tmp<<ch<<" , "<<OPERATOR<<" )"<<endl;
                ch = fgetc(f);
            }else{
                fout<<"("<<line_idx<<" , "<<tmp<<" , "<<OPERATOR<<" )"<<endl;
            }
        }else if(is_letter(ch) || is_digital(ch) || ch == '+' || ch == '-'){
            str += ch;
            while((ch = fgetc(f)) != EOF){
                if(!is_letter(ch) && !is_digital(ch) && ch != '.' && ch != '+' && ch != '-'){
                    break;
                }
                str += ch; 
            }
            fout<<"("<<line_idx<<" , "<<str<<" , ";
            if(keyword.count(str)){
                fout<<KEYWORD<<" )"<<endl;
            }else{
                int state = 0;
                for(int i = 0; i < str.size() && state != -1; i++){
                    if(is_digital(str[i])){
                        state = dfa[state][DIGITAL];
                    }else if(str[i]=='e' || str[i] == 'i'){
                        state = dfa[state][str[i]];
                    }
                    else if(is_letter(str[i])){
                        state = dfa[state][LETTER];
                    }else{
                        state = dfa[state][str[i]];
                    }  
                }
                if(state == -1 || terminals.count(state) == 0){
                    fout<<-1<<" )"<<endl;
                }else{
                    if(is_letter(str[0])){
                        fout<<IDENTIFIER<<" )"<<endl;
                    }else{
                        fout<<CONSTANT<<" )"<<endl;
                    }
                }
            }
            //ch = fgetc(f);
        }else{
            cout<<line_idx<<" "<<ch<<" what?";
            return false;
        }
    }
    fclose(f);
    fout.close();
    return true;
}
                    
int main()
{
    FA fa;
    fa.cin_nfa(grammar);
    // fa.show_nfa();
    // fa.show_input_letter();
    fa.to_dfa();
    //fa.show_dfa();
    // fa.show_terminals();
    if(analyze(fa.dfa,fa.terminals,codetext,tokens)){
        cout<<"lexical analysis for "<<codetext<<"task1 succeed"<<endl;
    }else{
        cout<<"lexical analysis for "<<codetext<<"task1 fail"<<endl;
    }
    if(analyze(fa.dfa,fa.terminals,codetext_simple,tokens_simple)){
        cout<<"lexical analysis for "<<codetext_simple<<"task1 succeed"<<endl;
    }else{
        cout<<"lexical analysis for "<<codetext_simple<<"task1 fail"<<endl;
    }
    system("pause");
    return 0;
}