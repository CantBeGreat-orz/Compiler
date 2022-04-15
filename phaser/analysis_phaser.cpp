#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
#include <iostream>
#include <set>
#include <map>
#include <cstdlib>

#define LOGIC_CONNECT_OPT 'f'
#define ASSIGN_OPT 'd'
#define CONPARE_OPT 'e'
#define BINO_OPT 'o'
#define MONO_OPT 'p'
#define LIB 'l'
#define EPSILON '$' // epsilon   
#define INITIAL_SEARCH '@' // #  
#define INITIAL_STATE 'S'  //初态
#define KEYWORD 0
#define IDENTIFIER 1
#define CONSTANT 2
#define OPERATOR 3
#define DELIMITER 4

using namespace std;

const string codetext = "output/codetext_modified.txt";
const string tokens_file = "../lexical/output/tokens.txt";
const string projects_file = "output/projects.txt";
const string analysis = "output/analysis.txt";
const string grammar = "input/grammar.txt";

vector<map<char,map<int, map<int, set<char>>>>> projects;
map<int, map<char, pair<char, int>>> action;
vector<pair<int, char>> codetext_modified;
map<char, vector<string>> grammar_map;
map<char, set<char>> first_set;
map<int, map<char, int>> go_to;
map<string, char> mapping;

void generate_mapping(){
    mapping.clear();
    mapping["include"] = 'i';
    mapping["int"] = 't';
    mapping["char"] = 'c';
    mapping["string"] = 's';
    mapping["return"] = 'r';
    mapping["iostream"] = LIB;
    mapping["main"] = 'm';
    mapping["while"] = 'w';
    mapping["cstring"] = LIB;
    mapping["real"] = 'a';
    mapping["+"] = BINO_OPT; 
    // mapping["-"] = BINO_OPT; //既可以为双目运算符，也可以为单目运算符
    mapping["*"] = BINO_OPT;
    mapping["/"] = BINO_OPT;
    mapping["%"] = BINO_OPT;
    mapping["&"] = BINO_OPT;
    mapping["|"] = BINO_OPT;
    mapping["^"] = BINO_OPT;
    mapping["<<"] = BINO_OPT;
    mapping[">>"] = BINO_OPT;
    mapping["++"] = MONO_OPT;
    mapping["--"] = MONO_OPT;
    mapping["~"] = MONO_OPT;
    mapping["!"] = MONO_OPT;
    mapping["&&"] = LOGIC_CONNECT_OPT;
    mapping["||"] = LOGIC_CONNECT_OPT;
    mapping["=="] = CONPARE_OPT;
    mapping["<="] = CONPARE_OPT;
    mapping[">="] = CONPARE_OPT;
    mapping["<"] = CONPARE_OPT;
    mapping[">"] = CONPARE_OPT;
    mapping["+="] = ASSIGN_OPT;
    mapping["-="] = ASSIGN_OPT;
    mapping["*="] = ASSIGN_OPT;
    mapping["/="] = ASSIGN_OPT;
    mapping["+="] = ASSIGN_OPT;
    mapping["&="] = ASSIGN_OPT;
    mapping["|="] = ASSIGN_OPT;
    mapping["^="] = ASSIGN_OPT;
    mapping["%="] = ASSIGN_OPT;
}
        
bool preprocess_tokens(const string &codetext, const string &tokens_file){
    ifstream fin(tokens_file.c_str());
    if(!fin.is_open()){
        cout<<"error: failed to open---file tokens"<<endl;
        return false;
    }
    ofstream fout(codetext.c_str());
    if(!fout.is_open()){
        cout<<"error: failed to open---file codetext"<<endl;
        return false;
    }
    char tmp;
    string token;
    int type, line_idx, i = 0;
    while(fin>>tmp>>line_idx>>tmp>>token>>tmp>>type>>tmp){
        ++i;
        switch(type){
            case KEYWORD:
                codetext_modified.emplace_back(line_idx,mapping[token]);
                fout<<mapping[token];
                break;
            case OPERATOR:
                if(token == "=" || token == "-"){
                    codetext_modified.emplace_back(line_idx,token[0]);
                    fout<<token;
                }
                else{
                    codetext_modified.emplace_back(line_idx,mapping[token]);
                    fout<<mapping[token];
                }
                break;
            case DELIMITER:
                codetext_modified.emplace_back(line_idx,token[0]);
                fout<<token;
                break;
            case IDENTIFIER:
                codetext_modified.emplace_back(line_idx,IDENTIFIER + '0');
                fout<<IDENTIFIER;
                break;
            case CONSTANT:
                codetext_modified.emplace_back(line_idx,CONSTANT + '0');
                fout<<CONSTANT;
                break;
            default:
                cout<<"wrong token in line"<<i<<endl;
        }
    }
    codetext_modified.emplace_back(line_idx,INITIAL_SEARCH);
    fin.close();
    fout.close();
    return true;
}

void show_codetext_m(){
    cout<<"codetext_modified:\n";
    for(auto lt:codetext_modified){
        cout<<lt.second;
    }
    cout<<endl;
}

bool get_grammar(const string &grammar){
    ifstream fin(grammar.c_str());
    grammar_map.clear();
    if(!fin.is_open()){
        cout<<"error: failed to open file---grammar "<<endl;
        return false;
    }
    char left;
    string right, tmp;
    int i = 0;
    fin>>right;
    grammar_map[INITIAL_STATE].emplace_back(right);
    while(fin>>left>>tmp>>right){
        grammar_map[left].emplace_back(right);
    }
    fin.close();
    return true;
}

void show_grammar(){
    cout<<"grammar:"<<endl;
    for(auto &iter:grammar_map){
        char tmp = iter.first;
        for(auto &str:iter.second){
            cout<<tmp<<"->"<<str<<endl;
        }
    }
}

bool is_upper(char ch){
    return (ch >= 'A' && ch <='Z');
}

void get_first_set(){
    bool changed = true;
    while(changed){
        changed = false;
        for(auto &iter:grammar_map){
            char state = iter.first;
            int pre_len = first_set[state].size();
            for(auto &str:iter.second){
                for(char ch:str){
                    if(is_upper(ch)){
                        first_set[state].insert(first_set[ch].begin(), first_set[ch].end());
                        if(!first_set[ch].count(EPSILON)) break;
                    }else{
                        first_set[state].emplace(ch);
                        break;
                    }
                }
            }
            if(!changed && pre_len != first_set[state].size()){
                changed = true;
            }
        }
    }
}

void show_first_set(){
    cout<<"first set:"<<endl;
    for(auto &iter:first_set){
        char tmp = iter.first;
        printf("first(%c): ", tmp);
        for(auto &it:iter.second){
            cout<<it<<" ";
        }
        cout<<endl;
    }
}

void get_closure(map<char,map<int, map<int, set<char>>>> &np){
    map<char,map<int, map<int, set<char>>>> tmp;
    bool changed = true;
    while(changed){
        changed = false;
        tmp = np;
        for(auto &iter:tmp){
            char state = iter.first;
            for(auto &it:iter.second){
                int gm_idx = it.first;
                for(auto &pf:it.second){
                    int pos = pf.first;
                    if(pos >= grammar_map[state][gm_idx].size()) continue;
                    char ch = grammar_map[state][gm_idx][pos];
                    if(!is_upper(ch)) continue;
                    set<char> forward;
                    if(pos < grammar_map[state][gm_idx].size() - 1){
                        char chh = grammar_map[state][gm_idx][pos + 1];
                        if(isupper(chh))
                            forward.insert(first_set[chh].begin(), first_set[chh].end());
                        else
                            forward.insert(chh);
                    }else{
                        forward.insert(EPSILON);
                    }
                    auto ep = forward.find(EPSILON);
                    if(ep != forward.end()){
                        forward.erase(ep);
                        forward.insert(pf.second.begin(), pf.second.end());
                    }
                    for(int i = 0; i < grammar_map[ch].size(); ++i){
                        if(!changed && ((!np.count(ch)) || (!np[ch].count(i)) || (!(np[ch][i].count(0)))))
                            changed = true;
                        np[ch][i][0].insert(forward.begin(), forward.end());
                    }
                }
            }
        }
    }
}

int same_project(map<char,map<int, map<int, set<char>>>> &np){
    for(int idx = 0; idx < projects.size(); ++idx){
        if(projects[idx].size() != np.size()) continue;
        bool is_same = true;
        for(auto &iter:projects[idx]){
            char state = iter.first;
            if(!is_same || !np.count(state) || iter.second.size() != np[state].size()){
                is_same = false;
                break;
            }
            for(auto &it:iter.second){
                int gm_idx = it.first;
                if(!is_same || !np[state].count(gm_idx) || it.second.size() != np[state][gm_idx].size()){
                    is_same = false;
                    break;
                }
                for(auto &pf:it.second){
                    int pos = pf.first;
                    if(!is_same || !np[state][gm_idx].count(pos) || pf.second.size() != np[state][gm_idx][pos].size()){
                        is_same = false;
                        break;
                    }
                    for(auto &ch:pf.second){
                        if(!np[state][gm_idx][pos].count(ch)){
                            is_same = false;
                            break;
                        }
                    }
                }
            }
        }
        if(is_same) return idx;
    }
    return -1;
}

void get_projects(){
    projects.clear();
    projects.emplace_back(map<char,map<int, map<int, set<char>>>>());
    for(int i = 0; i < grammar_map[INITIAL_STATE].size(); ++i)
        projects[0][INITIAL_STATE][i][0].emplace(INITIAL_SEARCH);
    int idx = 0;
    get_closure(projects[0]);
    while(idx < projects.size()){
        map<char, map<char,map<int, map<int, set<char>>>>> mp;
        for(auto &iter:projects[idx]){
            char state = iter.first;
            for(auto &it:iter.second){
                int gm_idx = it.first;
                for(auto &pf:it.second){
                    int pos = pf.first;
                    if(pos >= grammar_map[state][gm_idx].size() || grammar_map[state][gm_idx][pos] == EPSILON){
                        for(char f_ch:pf.second){
                            if(action.count(idx) && action[idx].count(f_ch)) printf("conflict in project%d",idx);
                            action[idx][f_ch] = make_pair(state, gm_idx);
                        }
                        continue;
                    }
                    mp[grammar_map[state][gm_idx][pos]][state][gm_idx][pos+1] = pf.second;
                }
            }
        }
        for(auto &iter:mp){
            char input_ch = iter.first;
            map<char,map<int, map<int, set<char>>>> new_project;
            for(auto &it:iter.second){
                char state = it.first;
                for(auto &gpf:it.second){
                    int gm_idx = gpf.first;
                    for(auto &pf:gpf.second){
                        int pos = pf.first;
                        new_project[state][gm_idx][pos] = pf.second;
                    }
                }
            }
            get_closure(new_project);
            int p_idx = same_project(new_project);
            if(p_idx == -1){
                p_idx = projects.size();
                projects.emplace_back(new_project);
            }
            if(is_upper(input_ch))
                go_to[idx][input_ch] = p_idx;
            else
                action[idx][input_ch] = make_pair(-1, p_idx);
        }
        idx++;
    }
}

void show_projects(){
    for(int idx = 0; idx < projects.size(); ++idx){
        printf("I%d:\n",idx);
        for(auto &iter:projects[idx]){
            char state = iter.first;
            for(auto &it:iter.second){
                int gm_idx = it.first;
                string str = grammar_map[state][gm_idx];
                for(auto &pf:it.second){
                    str.insert(pf.first, "·");
                    cout<<state<<"->"<<str<<",";
                    for(auto &ch:pf.second) cout<<" "<<ch;
                    cout<<endl;
                }
            }
        }
        cout<<endl;
    }
}

bool write_projects(const string &projects_file){
    ofstream fout(projects_file.c_str());
    if(!fout.is_open()){
        cout<<"error: failed to open---file projects"<<endl;
        return false;
    }
    for(int idx = 0; idx < projects.size(); ++idx){
        fout<<"I"<<idx<<endl;
        for(auto &iter:projects[idx]){
            char state = iter.first;
            for(auto &it:iter.second){
                int gm_idx = it.first;
                string str;
                for(auto &pf:it.second){
                    str = grammar_map[state][gm_idx];
                    str.insert(pf.first, "·");
                    fout<<state<<"->"<<str<<",";
                    for(auto &ch:pf.second) fout<<" "<<ch;
                    fout<<endl;
                }
            }
        };
        fout<<"------------------------------------------------------------"<<endl;
    }
    fout.close();
    return true;
}

bool analyze(const string &analysis){
    ofstream fout(analysis.c_str());
    if(!fout.is_open()){
        cout<<"error: failed to open---file analysis"<<endl;
        return false;
    }
    char ch, ch_state;
    int next, line, step = 1, status_len = 1, char_len = 1, code_len = codetext_modified.size();
    vector<int> st_status(code_len + 10);
    vector<char> st_char(code_len + 10);
    st_status[0] = 0;
    st_char[0] = INITIAL_SEARCH;
    for(int h = 0; h < code_len; ++h){
        tie(line, ch) = codetext_modified[h];
        //printf("(%d)    ",step);
        fout<<"("<<step<<")    ";
        for(int i = 0; i < status_len; ++i){
            //cout<<st_status[i];
            fout<<"("<<st_status[i]<<")";
        }
        //cout<<"                   ";
        fout<<"                   ";
        for(int i = 0; i < char_len; ++i){
            //cout<<st_char[i];
            fout<<st_char[i];
        }
        //cout<<"                   ";
        for(int i = h; i < code_len; ++i){
            //cout<<codetext_modified[i].second;
            fout<<codetext_modified[i].second;
        }
        //cout<<"                   ";
        fout<<"                   ";
        if(!status_len){
            //printf("error0: 行%d", line);
            fout<<"error0: 行"<<line;
            return false;
        }
        int status = st_status[status_len - 1];
        if(!action[status].count(ch)){
            //printf("error1: 行%d 出错原因可能是：", line);
            fout<<"error1: 行"<<line<<"出错原因可能是：";
            int err_idx = 1;
            for(auto iter:action[status]){
                switch(iter.first){
                    case LIB:
                        //printf("(%d)未知的库文件 ", err_idx);
                        fout<<"("<<err_idx<<")"<<"未知的库文件 ";
                        break;
                    case MONO_OPT:
                        //printf("(%d)期待单目运算符但未出现 ", err_idx);
                        fout<<"("<<err_idx<<")"<<"期待单目运算符但未出现 ";
                        break;
                    case BINO_OPT:
                        //printf("(%d)期待双目运算符但未出现 ", err_idx);
                        fout<<"("<<err_idx<<")"<<"期待双目运算符但未出现 ";
                        break;
                    case CONPARE_OPT:
                        //printf("(%d)期待比较运算符但未出现 ", err_idx);
                        fout<<"("<<err_idx<<")"<<"期待比较运算符但未出现 ";
                        break;
                    case ASSIGN_OPT:
                        //printf("(%d)期待赋值运算符但未出现 ", err_idx);
                        fout<<"("<<err_idx<<")"<<"期待赋值运算符但未出现 ";
                        break;
                    case LOGIC_CONNECT_OPT:
                        //printf("(%d)期待逻辑连接符运算符但未出现 ", err_idx);
                        fout<<"("<<err_idx<<")"<<"期待逻辑连接符运算符但未出现 ";
                        break;
                    default:
                        //printf("(%d)期待%c但未出现 ", err_idx, ch);
                        fout<<"("<<err_idx<<")"<<"期待"<<ch<<"但未出现 ";
                        break;
                }
            } 
            return false;
        }
        tie(ch_state, next) = action[status][ch];
        if(ch_state == INITIAL_STATE){
            //cout<<"acc";
            fout<<"acc";
            break;
        }
        if(ch_state == -1){
            st_status[status_len++] = next;
            st_char[char_len++] = ch;
            //printf("S%d", next);
            fout<<"S"<<next;

        }else{
            for(auto iter= grammar_map[ch_state][next].rbegin(); iter != grammar_map[ch_state][next].rend(); ++iter){
                if(*iter == EPSILON) break;
                if(!char_len){
                    //printf("error2: 行%d", line);
                    fout<<"error2: 行"<<line;
                    return false;
                }
                char tmp_ch = st_char[char_len - 1];
                if(tmp_ch != *iter){
                    //printf("error3: 行%d", line);
                    fout<<"error3: 行"<<line;
                    return false;
                }
                char_len--;
                status_len--;
            }
            st_char[char_len++] = ch_state;
            //printf("r%c%d        ", ch_state, next);
            fout<<"r"<<ch_state<<next<<"        ";
            if(!status_len){
                //printf("error4: 行%d", line);
                fout<<"error4: 行"<<line;
                return false;
            }
            status = st_status[status_len - 1];
            if(!go_to[status].count(ch_state)){
                //printf("error5: 行%d", line);
                fout<<"error5: 行"<<line;
                return false;
            }
            st_status[status_len++] = go_to[status][ch_state];
            //cout<<go_to[status][ch_state];
            fout<<go_to[status][ch_state];
            h--;
        }
        step++;
        //cout<<endl;
        fout<<endl;
    }
    fout.close();
    return true;
}

int main(){ 
    generate_mapping();
    preprocess_tokens(codetext,tokens_file);
    //show_codetext_m();
    get_grammar(grammar);
    //show_grammar();
    get_first_set();
    // show_first_set();
    get_projects();
    //show_projects();
    //write_projects(projects_file);
    if(analyze(analysis)) cout<<"YES"<<endl;
    else cout<<"NO"<<endl;
    system("pause");
    return 0;
}