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

const string tokens_file = "../lexical/output/tokens_simple.txt";
const string projects_file = "output/projects.txt";
const string analysis = "output/analysis.txt";
const string grammar = "input/grammar.txt";

vector<map<char,map<int, map<int, set<char>>>>> projects;
map<int, map<char, pair<char, int>>> action;
vector<string> tokens;
vector<pair<int, char>> codetext_modified;
map<char, vector<pair<string, string>>> grammar_map;
map<char, vector<vector<string>>> sem_map;
map<char, set<char>> first_set;
map<int, map<char, int>> go_to;

bool is_upper(char ch){
    return (ch >= 'A' && ch <='Z');
}

string to_str(char ch){
    string ret = "";
    ret.push_back(ch);
    return ret;
}

bool preprocess_tokens(const string &tokens_file){
    ifstream fin(tokens_file.c_str());
    if(!fin.is_open()){
        cout<<"error: failed to open---file tokens"<<endl;
        return false;
    }
    char tmp;
    string token;
    int type, line_idx, i = 0;
    while(fin>>line_idx>>tmp>>token>>tmp>>type>>tmp){
        ++i;
        tokens.emplace_back(token);
        if(type == IDENTIFIER || type == CONSTANT){
            codetext_modified.emplace_back(line_idx,type + '0');
            //cout<<type;
        }else{
            codetext_modified.emplace_back(line_idx,token[0]);
            //cout<<token[0];
        }
    }
    //cout<<endl;
    codetext_modified.emplace_back(line_idx,INITIAL_SEARCH);
    fin.close();
    return true;
}

// bool get_tokens(const string tokens_file){
//     ifstream fin(tokens_file.c_str());
//     if(!fin.is_open()){
//         cout<<"error: failed to open---file tokens"<<endl;
//         return false;
//     }
//     char tmp;
//     string token;
//     int type, line_idx, i = 0;
//     tokens.clear();
//     while(fin>>line_idx>>tmp>>token>>tmp>>type>>tmp){
//         ++i;
//         tokens.emplace_back(char(type + '0'), token);
//     }
//     fin.close();
//     return true;
// }

bool get_grammar(const string &grammar){
    ifstream fin(grammar.c_str());
    grammar_map.clear();
    if(!fin.is_open()){
        cout<<"error: failed to open file---grammar "<<endl;
        return false;
    }
    char left, sem_left, sem_right, op;
    string right, tmp, sem;
    int i = 0;
    fin>>right;
    grammar_map[INITIAL_STATE].emplace_back(right, "");
    while(fin>>left>>tmp>>right){
        sem = "";
        fin>>tmp>>sem_left>>tmp>>sem_right>>op;
        sem += "%s";
        sem += tmp;
        if(is_upper(sem_right) || sem_right == IDENTIFIER + '0' || sem_right == CONSTANT + '0'){
            sem += "%s";
        }
        while(op != '}'){
            sem += op;
            fin>>sem_right>>op;
            if(is_upper(sem_right) || sem_right == IDENTIFIER + '0' || sem_right == CONSTANT + '0'){
                sem += "%s";
            }
        }
        grammar_map[left].emplace_back(right, sem);
    }
    fin.close();
    return true;
}

void show_grammar(){
    cout<<"grammar:"<<endl;
    for(auto &iter:grammar_map){
        char tmp = iter.first;
        for(auto &it:iter.second){
            cout<<tmp<<"->"<<it.first<<" {"<<it.second<<"}"<<endl;
        }
    }
}

void get_first_set(){
    bool changed = true;
    while(changed){
        changed = false;
        for(auto &iter:grammar_map){
            char state = iter.first;
            int pre_len = first_set[state].size();
            for(auto &it:iter.second){
                for(char ch:it.first){
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
                    if(pos >= grammar_map[state][gm_idx].first.size()) continue;
                    char ch = grammar_map[state][gm_idx].first[pos];
                    if(!is_upper(ch)) continue;
                    set<char> forward;
                    if(pos < grammar_map[state][gm_idx].first.size() - 1){
                        char chh = grammar_map[state][gm_idx].first[pos + 1];
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
                    if(pos >= grammar_map[state][gm_idx].first.size() || grammar_map[state][gm_idx].first[pos] == EPSILON){
                        for(char f_ch:pf.second){
                            if(action.count(idx) && action[idx].count(f_ch)) printf("conflict in project%d",idx);
                            action[idx][f_ch] = make_pair(state, gm_idx);
                        }
                        continue;
                    }
                    mp[grammar_map[state][gm_idx].first[pos]][state][gm_idx][pos+1] = pf.second;
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
                string str = grammar_map[state][gm_idx].first;
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
                    str = grammar_map[state][gm_idx].first;
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
    vector<pair<char, int>> st_char(code_len + 10);
    vector<string> v_str;
    map<char, int> alphabet_cnt;
    st_status[0] = 0;
    st_char[0] = make_pair(INITIAL_SEARCH, 0);
    v_str.emplace_back("");
    cout<<"analyse:"<<endl;
    for(int h = 0; h < code_len; ++h){
        tie(line, ch) = codetext_modified[h];
        // printf("(%d)    ",step);
        // fout<<"("<<step<<")    ";
        // for(int i = 0; i < status_len; ++i){
        //     cout<<st_status[i];
        //     fout<<st_status[i];
        // }
        // cout<<"                   ";
        // fout<<"                   ";
        // for(int i = 0; i < char_len; ++i){
        //     cout<<st_char[i].first;
        //     fout<<st_char[i].first;
        // }
        // cout<<"                   ";
        // for(int i = h; i < code_len; ++i){
        //     cout<<codetext_modified[i].second;
        //     fout<<codetext_modified[i].second;
        // }
        // cout<<"                   ";
        // fout<<"                   ";
        if(!status_len){
            printf("error0: 行%d", line);
            return false;
        }
        int status = st_status[status_len - 1];
        if(!action[status].count(ch)){
            printf("error1: 行%d 出错原因可能是未找到：", line);
            for(auto iter:action[status]) cout<<iter.first<<" ";
            return false;
        }
        tie(ch_state, next) = action[status][ch];
        if(ch_state == INITIAL_STATE){
            // cout<<"acc";
            // fout<<"acc";
            return true;
        }
        if(ch_state == -1){
            st_status[status_len++] = next;
            if(ch == IDENTIFIER + '0' || ch == CONSTANT + '0'){
                v_str.emplace_back(tokens[h]);
            }else{
                v_str.emplace_back(to_str(ch));
            }
            st_char[char_len++] = make_pair(ch, v_str.size() - 1);
            // printf("S%d", next);
            // fout<<"S"<<next;

        }else{
            alphabet_cnt[ch_state]++;
            vector<string> vtmp;
            for(auto iter= grammar_map[ch_state][next].first.rbegin(); iter != grammar_map[ch_state][next].first.rend(); ++iter){
                if(*iter == EPSILON) break;
                if(!char_len){+
                    printf("error2: 行%d", line);
                    return false;
                }
                char tmp_ch = st_char[char_len - 1].first;
                if(tmp_ch != *iter){
                    printf("error3: 行%d", line);
                    return false;
                }
                if(is_upper(tmp_ch) || tmp_ch == IDENTIFIER + '0' || tmp_ch == CONSTANT + '0'){
                    vtmp.emplace_back(v_str[st_char[char_len - 1].second]);
                }
                char_len--;
                status_len--;
            }
            v_str.emplace_back(to_str(ch_state) + to_string(alphabet_cnt[ch_state]));
            vtmp.emplace_back(to_str(ch_state) + to_string(alphabet_cnt[ch_state]));
            st_char[char_len++] = make_pair(ch_state, v_str.size() - 1);
            // printf("r%c%d        ", ch_state, next);
            // fout<<"r"<<ch_state<<next<<"        ";
            if(!status_len){
                printf("error4: 行%d", line);
                return false;
            }
            status = st_status[status_len - 1];
            if(!go_to[status].count(ch_state)){
                printf("error5: 行%d", line);
                return false;
            }
            st_status[status_len++] = go_to[status][ch_state];
            // cout<<go_to[status][ch_state]<<"        ";
            // fout<<go_to[status][ch_state]<<"        ";
            if(vtmp.size() == 2){
                printf(grammar_map[ch_state][next].second.c_str(), vtmp[1].c_str(), vtmp[0].c_str());
                cout<<endl;
            }else if(vtmp.size() == 3){
                printf(grammar_map[ch_state][next].second.c_str(), vtmp[2].c_str(), vtmp[1].c_str(), vtmp[0].c_str());
                cout<<endl;
            }
            h--;
        }
        step++;
        // cout<<endl;
        // fout<<endl;
    }
    fout.close();
    return false;
}

int main(){ 
    preprocess_tokens(tokens_file);
    // show_codetext_m();
    get_grammar(grammar);
    //show_grammar();
    get_first_set();
    //show_first_set();
    get_projects();
    //show_projects();
    //write_projects(projects_file);
    analyze(analysis);
    return 0;
}