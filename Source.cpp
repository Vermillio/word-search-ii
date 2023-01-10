#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <cassert>
#include <cstring>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include "SuffixTree.hpp"

using namespace std;


class ChronoProfiler
{
public:
    ChronoProfiler(const std::string& name) : m_name(name), m_start(std::chrono::steady_clock::now())
    {
    }

    ~ChronoProfiler()
    {
        auto end = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();

        // Add elapsed time to timer with the same name
        s_timers[m_name] += elapsed_time;
    }

    // Static function to print elapsed time for a specific timer
    static void printTime(const std::string& name)
    {
        std::cout << name << " elapsed time: " << s_timers[name] << "ms" << std::endl;
    }

private:
    std::string m_name;
    std::chrono::steady_clock::time_point m_start;

    // Static map to store timers
    inline static std::unordered_map<std::string, long long> s_timers;
};

struct cell
{
    char c{'0'};
    cell* neighbors[4] = { nullptr, nullptr, nullptr, nullptr };
    int neighbor{0};

    cell() {}
    cell(char _c)
        : c(_c)
    {}
};

struct WordFinder
{
    vector<string>& out_words;
    const vector<vector<char>>& board;

    cell* grid;
    int grid_size;
    int n;

    int path[11];
    int path_index{ 0 };

    WordFinder(const vector<vector<char>>& in_board, vector<string>& words) :
        board(in_board),
        out_words(words)
    {
        int m = (int)board.size();
        n = (int)board[0].size();
        grid_size = (int)board.size() * (int)board[0].size();

        grid = new cell[grid_size];
        for (int idx = 0; idx < grid_size; ++idx)
        {
            int j = idx % n;
            int i = (idx - j) / n;
            grid[idx] = cell{ board[i][j] };
            if (j > 0)
            {
                grid[idx].neighbors[0] = &grid[idx - 1];
            }
            if (j < n - 1)
            {
                grid[idx].neighbors[1] = &grid[idx + 1];
            }
            if (i > 0)
            {
                grid[idx].neighbors[2] = &grid[idx - n];
            }
            if (i < m - 1)
            {
                grid[idx].neighbors[3] = &grid[idx + n];
            }
        }
    }

    bool has_valid_path(const Node& node, const int key, int idx)
    {
        const auto& paths = node.paths.paths.find(key)->second;
        for (int i = 0; i < paths.paths.size(); ++i)
        {
            const auto& path{ paths.paths[i] };
            if (paths.valid_paths[i] != -1 || find(path.begin(), path.end(), idx) != path.end())
            {
                continue;
            }
            if (!node.parent)
            {
                return true;
            }
            if (has_valid_path(*node.parent, path[0], idx))
            {
                return true;
            }
        }
        return false;
    }

    void invalidate_paths(Node& node, const int key, int idx)
    {
        auto& paths = node.paths.paths.find(key)->second;
        if (!node.parent)
        {
            for (int i = 0; i < paths.paths.size(); ++i)
            {
                auto& path{ paths.paths[i] };
                if (paths.valid_paths[i] != -1
                    || find(path.begin(), path.end(), idx) == path.end())
                {
                    continue;
                }
                paths.valid_paths[i] = idx;
            }
            return;
        }
        auto& parent{ *node.parent };
        for (int i = 0; i < paths.paths.size(); ++i)
        {
            auto& path{ paths.paths[i] };
            if (paths.valid_paths[i] != -1)
            {
                continue;
            }
            if (find(path.begin(), path.end(), idx) == path.end())
            {
                if (bool res = has_valid_path(parent, path[0], idx))
                {
                    continue;
                }
            }
            paths.valid_paths[i] = idx;
        }
    }

    void validate_paths(Node& node, const int key, int idx)
    {
        auto& paths = node.paths.paths.find(key)->second;
        for (int i = 0; i < paths.paths.size(); ++i)
        {
            auto& path{ paths.paths[i] };
            if (paths.valid_paths[i] == idx)
            {
                paths.valid_paths[i] = -1;
            }
        }
    }

    bool process_neighbor(const Node& node, const char s, const cell& cell_n)
    {
        return true;
    }

    bool search_impl(Node& node, int start, int idx, bool bFindFirst)
    {
        for (int i = 0; i < grid_size; ++i)
        {
            grid[i].neighbor = 0;
        }
        int cached_idx = idx;
        int path_size = path_index;
        grid[path[path_index - 1]].c = '$';
        auto pattern{ node.get_pattern() };

        do
        {
            if (path_index - path_size == pattern.size() - cached_idx)
            {
                auto& container = node.paths.paths[path[path_index - 1]];
                container.valid_paths.push_back(-1);
                container.paths.push_back({});
                container.paths.back().reserve(path_index);
                if (bFindFirst)
                {
                    for (int i = 0; i < path_index; ++i)
                    {
                        container.paths.back().push_back(path[i]);
                        grid[path[i]].c = board[path[i] / n][path[i] % n];
                    }
                    return true;
                }
                else
                {
                    for (int i = 0; i < path_index; ++i)
                    {
                        container.paths.back().push_back(path[i]);
                    }
                }
            }
            else
            {
                cell* c = &grid[path[path_index - 1]];
                const char p = pattern[idx];
                bool found{ false };
                for (int i = c->neighbor; i < 4; ++i)
                {
                    if (auto & neighbor_cell_ptr{ c->neighbors[i] })
                    {
                        auto& neighbor_cell{ *neighbor_cell_ptr };
                        const int neighbor_cell_index = neighbor_cell_ptr - grid;
                        if (neighbor_cell.c == p
                            && process_neighbor(node, p, neighbor_cell))
                        {
                            if (node.parent)
                            {
                                if (!has_valid_path(*node.parent, start, neighbor_cell_index))
                                {
                                    continue;
                                }
                                invalidate_paths(*node.parent, start, neighbor_cell_index);
                            }
                            found = true;
                            c->neighbor = i + 1;
                            path[path_index] = neighbor_cell_index;
                            neighbor_cell.c = '$';
                            ++path_index;
                            c = neighbor_cell_ptr;
                            ++idx;
                            break;
                        }
                    }
                }
                if (found)
                {
                    continue;
                }
                else
                {
                    c->neighbor = 0;
                }
            }
            if (node.parent)
            {
                validate_paths(*node.parent, start, path[path_index - 1]);
            }
            --path_index;
            grid[path[path_index]].c = board[path[path_index] / n][path[path_index] % n];
            --idx;
        } while (path_index > 0);

        for (int i = 0; i < grid_size; ++i)
        {
            assert(grid[i].c != '$');
        }
        return !node.paths.paths.empty();
    }

    bool search(Node& node)
    {
        bool bFindFirst = !node.children_num;
        bool Found = false;
        if (bFindFirst)
        {
            for (auto& pair : node.parent->paths.paths)
            {
                path_index = 1;
                path[0] = pair.first;
                if (search_impl(node, pair.first, 0, bFindFirst))
                {
                    return true;
                }
            }
        }
        else
        {
            for (auto& pair : node.parent->paths.paths)
            {
                path_index = 1;
                path[0] = pair.first;
                if (search_impl(node, pair.first, 0, bFindFirst))
                {
                    Found = true;
                }
            }
        }
        return Found;
    }

    bool SearchSuffixes(Node& Node)
    {
        vector<int> out_path;
        bool res = search(Node);
        if (res && Node.mask)
        {
            if (Node.mask % 2 == 1)
            {
                out_words.push_back(string{ Node.pattern });
            }
            if (Node.mask >= 2)
            {
                out_words.push_back(string{ Node.pattern });
                reverse(out_words.back().begin(), out_words.back().end());
            }
            return true;
        }
        return true;
    }

    void FindWords(SuffixTree& Tree)
    {
        Node* RootsToSearch[26];
        memset(RootsToSearch, 0, 26 * sizeof(Node*));
        for (int i = 0; i < grid_size; ++i)
        {
            if (Tree.Roots[grid[i].c - 97] != nullptr)
            {
                int RootIdx = grid[i].c - 97;
                auto& Root = Tree.Roots[RootIdx];
                auto root_pattern{ Root->pattern };
                path_index = 1;
                path[0] = i;
                if (search_impl(*Root, i, 1, Root->children_num == 0))
                {
                    RootsToSearch[RootIdx] = Tree.Roots[RootIdx];
                }
            }
        }
        for (auto& Root : RootsToSearch)
        {
            if (!Root)
            {
                continue;
            }
            if (Root->mask % 2 == 1)
            {
                out_words.push_back(string{ Root->pattern.begin(), Root->pattern.end() });
            }
            if (Root->mask >= 2)
            {
                out_words.push_back(string{ Root->pattern.begin(), Root->pattern.end() });
                reverse(out_words.back().begin(), out_words.back().end());
            }
            for (auto c : Root->children)
            {
                if (c)
                {
                    DFS(*c);
                }
            }
        }
        return;
    }

    void DFS(Node& Node)
    {
        if (!SearchSuffixes(Node))
        {
            return;
        }
        for (auto& c : Node.children)
        {
            if (c)
            {
                DFS(*c);
            }
        }
        return;
    }
};

void FindWords(vector<string>& words, const vector<vector<char>>& board, vector<string>& out)
{
    out.reserve(words.size());
    vector<int> mask(words.size(), 1);
    {
        ChronoProfiler profiler("prune");
        int grid_size = board.size() * board[0].size();
        int occ_table[26];
        memset(occ_table, 0, 26 * sizeof(int));
        for (auto& row : board)
        {
            for (auto& col : row)
            {
                occ_table[col - 97]++;
            }
        }
        int occ[26];
        for (auto i{ 0 }; i < words.size(); ++i)
        {
            auto& word{ words[i] };
            copy(occ_table, occ_table + 26, occ);
            if (word.size() > grid_size)
            {
                mask[i] = 0;
                continue;
            }
            for (auto& c : word)
            {
                if (--occ[c - 97] < 0)
                {
                    mask[i] = 0;
                    break;
                }
            }

            int left = word.find_first_not_of(word[0]);
            int right = word.size() - word.find_last_not_of(word[word.size() - 1]);
            if (left > right)
            {
                reverse(begin(word), end(word));
                mask[i] = 2;
            }
        }

    }
    SuffixTree T;
    {
        ChronoProfiler profiler("build_tree");
        T.Build(words, mask);
    }
    {
        ChronoProfiler profiler("find_words");
        WordFinder Finder{ board, out };
        Finder.FindWords(T);
    }
}

int main()
{
    //std::vector<std::vector<char>> board
    //{
    //    {'o','a','a','n'},
    //    {'e','t','a','e'},
    //    {'i','h','k','r'},
    //    {'i','f','l','v'}
    //};
    //std::vector<std::string> words{ "oath", "pea", "eat", "rain", 
    //    //"oathi", "oathk", "oathf", "oate", "oathii", "oathfi", "oathfii"
    //};
   // std::vector<std::string> words { "oath","pea","eat","rain","hklf", "hf" };


   //std::vector<std::vector<char>> board{
   //    {'a', 'b'},
   //    {'c', 'd'}
   //};
   //std::vector<std::string> words{ "abcb" };

   //std::vector<std::vector<char>> board{
   //   {'a', 'a'}
   //};
   //std::vector<std::string> words{ "a" };


    //std::vector<std::vector<char>> board{
    //   {'a', 'b'},
    //   {'a', 'a'}
    //};
    //std::vector<std::string> words{ "aba", "baa", "bab", "aaab", "aaa", "aaaa", "aaba" };

    //std::vector<std::vector<char>> board {
    //    {'o', 'a', 'b', 'n'},
    //    {'o', 't', 'a', 'e'},
    //    {'a', 'h', 'k', 'r'},
    //    {'a', 'f', 'l', 'v'}
    //};
    //std::vector<std::string> words{ "oa", "oaa" };

    //std::vector<std::vector<char>> board{
    //    {'a', 'b', 'c'},
    //    {'a', 'e', 'd'},
    //    {'a', 'f', 'g'}
    //};
    //std::vector<std::string> words{ "abcdefg","gfedcbaaa","eaabcdgfa","befa","dgc","ade" };
    // std::vector<std::vector<char>> board
    // {
    //     {'a','a'}
    // };

    // std::vector<std::string> words { "aaa" };
    // std::vector<std::vector<char>> board
    // {
    //     {'a','b','c'},
    //     {'a','e','d'},
    //     {'a','f','g'}
    // };
    // std::vector<std::string> words { "abcdefg","gfedcbaaa","eaabcdgfa","befa","dgc","ade" };
    //std::vector<std::string> words {"eaafgdcba", "eaabcdgfa"};

    // std::vector<std::vector<char>> board
    // {
    //     {'a','b','e'},
    //     {'b','c','d'}
    // };
    // std::vector<std::string> words {"abcdeb"};

    {
        std::vector<std::vector<char>> board{ {'b','a','b','a','b','a','b','a','b','a'},
                                               {'a','b','a','b','a','b','a','b','a','b'},
                                               {'b','a','b','a','b','a','b','a','b','a'},
                                               {'a','b','a','b','a','b','a','b','a','b'},
                                               {'b','a','b','a','b','a','b','a','b','a'},
                                               {'a','b','a','b','a','b','a','b','a','b'},
                                               {'b','a','b','a','b','a','b','a','b','a'},
                                               {'a','b','a','b','a','b','a','b','a','b'},
                                               {'b','a','b','a','b','a','b','a','b','a'},
                                               {'a','b','a','b','a','b','a','b','a','b'} };

        std::vector<std::string> words{
            "ababababaa","ababababab","ababababac","ababababad","ababababae","ababababaf","ababababag","ababababah","ababababai","ababababaj",
            "ababababak","ababababal",
            "ababababam","ababababan",
            "ababababao","ababababap","ababababaq","ababababar","ababababas","ababababat","ababababau","ababababav","ababababaw","ababababax","ababababay","ababababaz","ababababba","ababababbb","ababababbc","ababababbd","ababababbe","ababababbf","ababababbg","ababababbh","ababababbi","ababababbj","ababababbk","ababababbl","ababababbm","ababababbn","ababababbo","ababababbp","ababababbq","ababababbr","ababababbs","ababababbt","ababababbu","ababababbv","ababababbw","ababababbx","ababababby",
            "ababababbz","ababababca","ababababcb","ababababcc","ababababcd","ababababce","ababababcf","ababababcg","ababababch","ababababci","ababababcj","ababababck","ababababcl","ababababcm","ababababcn","ababababco","ababababcp","ababababcq","ababababcr","ababababcs","ababababct","ababababcu","ababababcv","ababababcw","ababababcx","ababababcy","ababababcz","ababababda","ababababdb","ababababdc","ababababdd","ababababde","ababababdf","ababababdg","ababababdh","ababababdi","ababababdj","ababababdk","ababababdl","ababababdm","ababababdn","ababababdo","ababababdp","ababababdq","ababababdr","ababababds","ababababdt","ababababdu","ababababdv","ababababdw","ababababdx","ababababdy","ababababdz","ababababea","ababababeb","ababababec","ababababed","ababababee","ababababef","ababababeg","ababababeh","ababababei","ababababej","ababababek","ababababel","ababababem","ababababen","ababababeo","ababababep","ababababeq","ababababer","ababababes","ababababet","ababababeu","ababababev","ababababew","ababababex","ababababey","ababababez","ababababfa","ababababfb","ababababfc","ababababfd","ababababfe","ababababff","ababababfg","ababababfh","ababababfi","ababababfj","ababababfk","ababababfl","ababababfm","ababababfn","ababababfo","ababababfp","ababababfq","ababababfr","ababababfs","ababababft","ababababfu",
            "ababababfv","ababababfw","ababababfx","ababababfy","ababababfz","ababababga","ababababgb","ababababgc","ababababgd","ababababge","ababababgf","ababababgg","ababababgh","ababababgi","ababababgj","ababababgk","ababababgl","ababababgm","ababababgn","ababababgo","ababababgp","ababababgq","ababababgr","ababababgs","ababababgt","ababababgu","ababababgv","ababababgw","ababababgx","ababababgy","ababababgz","ababababha","ababababhb","ababababhc","ababababhd","ababababhe","ababababhf","ababababhg","ababababhh","ababababhi","ababababhj","ababababhk","ababababhl","ababababhm","ababababhn","ababababho","ababababhp","ababababhq","ababababhr","ababababhs","ababababht","ababababhu","ababababhv","ababababhw","ababababhx","ababababhy","ababababhz","ababababia","ababababib","ababababic","ababababid","ababababie","ababababif","ababababig","ababababih","ababababii","ababababij","ababababik","ababababil","ababababim","ababababin","ababababio","ababababip","ababababiq","ababababir","ababababis","ababababit","ababababiu","ababababiv","ababababiw","ababababix","ababababiy","ababababiz","ababababja","ababababjb","ababababjc","ababababjd","ababababje","ababababjf","ababababjg","ababababjh","ababababji","ababababjj","ababababjk","ababababjl","ababababjm","ababababjn","ababababjo","ababababjp","ababababjq","ababababjr","ababababjs","ababababjt","ababababju","ababababjv","ababababjw",
            "ababababjx","ababababjy","ababababjz","ababababka","ababababkb","ababababkc","ababababkd","ababababke","ababababkf","ababababkg","ababababkh","ababababki","ababababkj","ababababkk","ababababkl","ababababkm","ababababkn","ababababko","ababababkp","ababababkq","ababababkr","ababababks","ababababkt","ababababku","ababababkv","ababababkw","ababababkx","ababababky","ababababkz","ababababla","abababablb","abababablc","ababababld","abababable","abababablf","abababablg","abababablh","ababababli","abababablj","abababablk","ababababll","abababablm","ababababln","abababablo","abababablp","abababablq","abababablr","ababababls","abababablt","abababablu","abababablv","abababablw","abababablx","abababably","abababablz","ababababma","ababababmb","ababababmc","ababababmd","ababababme","ababababmf","ababababmg","ababababmh","ababababmi","ababababmj","ababababmk","ababababml","ababababmm","ababababmn","ababababmo","ababababmp","ababababmq","ababababmr","ababababms","ababababmt","ababababmu","ababababmv","ababababmw","ababababmx","ababababmy","ababababmz","ababababna","ababababnb","ababababnc","ababababnd","ababababne","ababababnf","ababababng","ababababnh","ababababni","ababababnj","ababababnk","ababababnl","ababababnm","ababababnn","ababababno","ababababnp","ababababnq","ababababnr","ababababns","ababababnt","ababababnu","ababababnv","ababababnw","ababababnx","ababababny","ababababnz","ababababoa","ababababob",
            "ababababoc","ababababod","ababababoe","ababababof","ababababog","ababababoh","ababababoi","ababababoj","ababababok","ababababol","ababababom","ababababon","ababababoo","ababababop","ababababoq","ababababor","ababababos","ababababot","ababababou","ababababov","ababababow","ababababox","ababababoy","ababababoz","ababababpa","ababababpb","ababababpc","ababababpd","ababababpe","ababababpf","ababababpg","ababababph","ababababpi","ababababpj","ababababpk","ababababpl","ababababpm","ababababpn","ababababpo","ababababpp","ababababpq","ababababpr","ababababps","ababababpt","ababababpu","ababababpv","ababababpw","ababababpx","ababababpy","ababababpz","ababababqa","ababababqb","ababababqc","ababababqd","ababababqe","ababababqf","ababababqg","ababababqh","ababababqi","ababababqj","ababababqk","ababababql","ababababqm","ababababqn","ababababqo","ababababqp","ababababqq","ababababqr","ababababqs","ababababqt","ababababqu","ababababqv","ababababqw","ababababqx","ababababqy","ababababqz","ababababra","ababababrb","ababababrc","ababababrd","ababababre","ababababrf","ababababrg","ababababrh","ababababri","ababababrj","ababababrk","ababababrl","ababababrm","ababababrn","ababababro","ababababrp","ababababrq","ababababrr","ababababrs","ababababrt","ababababru","ababababrv","ababababrw","ababababrx","ababababry","ababababrz","ababababsa","ababababsb","ababababsc","ababababsd","ababababse","ababababsf","ababababsg",
            "ababababsh","ababababsi","ababababsj","ababababsk","ababababsl","ababababsm","ababababsn","ababababso","ababababsp","ababababsq","ababababsr","ababababss","ababababst","ababababsu","ababababsv","ababababsw","ababababsx","ababababsy","ababababsz","ababababta","ababababtb","ababababtc","ababababtd","ababababte","ababababtf","ababababtg","ababababth","ababababti","ababababtj","ababababtk","ababababtl","ababababtm","ababababtn","ababababto","ababababtp","ababababtq","ababababtr","ababababts","ababababtt","ababababtu","ababababtv","ababababtw","ababababtx","ababababty","ababababtz","ababababua","ababababub","ababababuc","ababababud","ababababue","ababababuf","ababababug","ababababuh","ababababui","ababababuj","ababababuk","ababababul","ababababum","ababababun","ababababuo","ababababup","ababababuq","ababababur","ababababus","ababababut","ababababuu","ababababuv","ababababuw","ababababux","ababababuy","ababababuz","ababababva","ababababvb","ababababvc","ababababvd","ababababve","ababababvf","ababababvg","ababababvh","ababababvi","ababababvj","ababababvk","ababababvl","ababababvm","ababababvn","ababababvo","ababababvp","ababababvq","ababababvr","ababababvs","ababababvt","ababababvu","ababababvv","ababababvw","ababababvx","ababababvy","ababababvz","ababababwa","ababababwb","ababababwc","ababababwd","ababababwe","ababababwf","ababababwg","ababababwh","ababababwi","ababababwj","ababababwk","ababababwl","ababababwm","ababababwn","ababababwo","ababababwp","ababababwq","ababababwr","ababababws","ababababwt","ababababwu","ababababwv","ababababww","ababababwx","ababababwy","ababababwz","ababababxa","ababababxb","ababababxc","ababababxd","ababababxe","ababababxf","ababababxg","ababababxh","ababababxi","ababababxj","ababababxk","ababababxl","ababababxm","ababababxn","ababababxo","ababababxp","ababababxq","ababababxr","ababababxs","ababababxt","ababababxu","ababababxv","ababababxw","ababababxx","ababababxy","ababababxz","ababababya","ababababyb","ababababyc","ababababyd","ababababye","ababababyf","ababababyg","ababababyh","ababababyi","ababababyj","ababababyk","ababababyl","ababababym","ababababyn","ababababyo","ababababyp","ababababyq","ababababyr","ababababys","ababababyt","ababababyu","ababababyv","ababababyw","ababababyx","ababababyy","ababababyz","ababababza","ababababzb","ababababzc","ababababzd","ababababze","ababababzf","ababababzg","ababababzh","ababababzi","ababababzj","ababababzk","ababababzl","ababababzm","ababababzn","ababababzo","ababababzp","ababababzq","ababababzr","ababababzs","ababababzt","ababababzu","ababababzv","ababababzw","ababababzx","ababababzy","ababababzz"
        };
        vector<string> res;
        {
            FindWords(words, board, res);
        }
        cout << " RESULT:" << endl;
        for (auto const& res_word : res)
        {
            std::cout << " " << res_word << " ";
        }
        std::cout << '\n';
        if (res.size() == 0)
        {
            std::cout << "Not found" << '\n';
        }
    }
    //{
    //    std::vector<std::vector<char>> board{ {'m','b','c','d','e','f','g','h','i','j','k','l'},
    //                                       {'n','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'o','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'p','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'q','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'r','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'s','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'t','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'u','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'v','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'w','a','a','a','a','a','a','a','a','a','a','a'},
    //                                       {'x','y','z','a','a','a','a','a','a','a','a','a'} };
    //    std::vector<std::string> words{ "aaaaaaaaaa","aaaaaaaaab","aaaaaaaaac","aaaaaaaaad","aaaaaaaaae","aaaaaaaaaf","aaaaaaaaag","aaaaaaaaah","aaaaaaaaai","aaaaaaaaaj","aaaaaaaaak","aaaaaaaaal","aaaaaaaaam","aaaaaaaaan","aaaaaaaaao","aaaaaaaaap","aaaaaaaaaq","aaaaaaaaar","aaaaaaaaas","aaaaaaaaat","aaaaaaaaau","aaaaaaaaav","aaaaaaaaaw","aaaaaaaaax","aaaaaaaaay","aaaaaaaaaz","aaaaaaaaba","aaaaaaaabb","aaaaaaaabc","aaaaaaaabd","aaaaaaaabe","aaaaaaaabf","aaaaaaaabg","aaaaaaaabh","aaaaaaaabi","aaaaaaaabj","aaaaaaaabk","aaaaaaaabl","aaaaaaaabm",
    //    "aaaaaaaabn","aaaaaaaabo","aaaaaaaabp","aaaaaaaabq","aaaaaaaabr","aaaaaaaabs","aaaaaaaabt","aaaaaaaabu","aaaaaaaabv","aaaaaaaabw","aaaaaaaabx","aaaaaaaaby","aaaaaaaabz","aaaaaaaaca","aaaaaaaacb","aaaaaaaacc","aaaaaaaacd","aaaaaaaace","aaaaaaaacf","aaaaaaaacg","aaaaaaaach","aaaaaaaaci","aaaaaaaacj","aaaaaaaack","aaaaaaaacl","aaaaaaaacm","aaaaaaaacn","aaaaaaaaco","aaaaaaaacp","aaaaaaaacq","aaaaaaaacr","aaaaaaaacs","aaaaaaaact","aaaaaaaacu","aaaaaaaacv","aaaaaaaacw","aaaaaaaacx","aaaaaaaacy","aaaaaaaacz","aaaaaaaada","aaaaaaaadb","aaaaaaaadc","aaaaaaaadd","aaaaaaaade","aaaaaaaadf","aaaaaaaadg","aaaaaaaadh","aaaaaaaadi","aaaaaaaadj","aaaaaaaadk","aaaaaaaadl","aaaaaaaadm","aaaaaaaadn","aaaaaaaado","aaaaaaaadp","aaaaaaaadq","aaaaaaaadr","aaaaaaaads","aaaaaaaadt","aaaaaaaadu","aaaaaaaadv","aaaaaaaadw","aaaaaaaadx","aaaaaaaady","aaaaaaaadz","aaaaaaaaea","aaaaaaaaeb","aaaaaaaaec","aaaaaaaaed","aaaaaaaaee","aaaaaaaaef","aaaaaaaaeg","aaaaaaaaeh","aaaaaaaaei","aaaaaaaaej","aaaaaaaaek","aaaaaaaael","aaaaaaaaem","aaaaaaaaen","aaaaaaaaeo","aaaaaaaaep","aaaaaaaaeq","aaaaaaaaer","aaaaaaaaes","aaaaaaaaet","aaaaaaaaeu","aaaaaaaaev","aaaaaaaaew","aaaaaaaaex","aaaaaaaaey","aaaaaaaaez","aaaaaaaafa","aaaaaaaafb","aaaaaaaafc","aaaaaaaafd","aaaaaaaafe","aaaaaaaaff","aaaaaaaafg","aaaaaaaafh","aaaaaaaafi","aaaaaaaafj","aaaaaaaafk","aaaaaaaafl","aaaaaaaafm","aaaaaaaafn","aaaaaaaafo","aaaaaaaafp","aaaaaaaafq","aaaaaaaafr","aaaaaaaafs","aaaaaaaaft","aaaaaaaafu","aaaaaaaafv","aaaaaaaafw","aaaaaaaafx","aaaaaaaafy","aaaaaaaafz","aaaaaaaaga","aaaaaaaagb","aaaaaaaagc","aaaaaaaagd","aaaaaaaage","aaaaaaaagf","aaaaaaaagg","aaaaaaaagh","aaaaaaaagi","aaaaaaaagj","aaaaaaaagk","aaaaaaaagl","aaaaaaaagm","aaaaaaaagn","aaaaaaaago","aaaaaaaagp","aaaaaaaagq","aaaaaaaagr","aaaaaaaags","aaaaaaaagt","aaaaaaaagu","aaaaaaaagv","aaaaaaaagw","aaaaaaaagx","aaaaaaaagy","aaaaaaaagz","aaaaaaaaha","aaaaaaaahb","aaaaaaaahc","aaaaaaaahd","aaaaaaaahe","aaaaaaaahf","aaaaaaaahg","aaaaaaaahh","aaaaaaaahi","aaaaaaaahj","aaaaaaaahk","aaaaaaaahl","aaaaaaaahm","aaaaaaaahn","aaaaaaaaho","aaaaaaaahp","aaaaaaaahq",
    //    "aaaaaaaahr","aaaaaaaahs","aaaaaaaaht","aaaaaaaahu","aaaaaaaahv","aaaaaaaahw","aaaaaaaahx","aaaaaaaahy","aaaaaaaahz","aaaaaaaaia","aaaaaaaaib","aaaaaaaaic","aaaaaaaaid","aaaaaaaaie","aaaaaaaaif","aaaaaaaaig","aaaaaaaaih","aaaaaaaaii","aaaaaaaaij","aaaaaaaaik","aaaaaaaail","aaaaaaaaim","aaaaaaaain","aaaaaaaaio","aaaaaaaaip","aaaaaaaaiq","aaaaaaaair","aaaaaaaais","aaaaaaaait","aaaaaaaaiu","aaaaaaaaiv","aaaaaaaaiw","aaaaaaaaix","aaaaaaaaiy","aaaaaaaaiz","aaaaaaaaja","aaaaaaaajb","aaaaaaaajc","aaaaaaaajd","aaaaaaaaje","aaaaaaaajf","aaaaaaaajg","aaaaaaaajh","aaaaaaaaji","aaaaaaaajj","aaaaaaaajk","aaaaaaaajl","aaaaaaaajm","aaaaaaaajn","aaaaaaaajo","aaaaaaaajp","aaaaaaaajq","aaaaaaaajr","aaaaaaaajs","aaaaaaaajt","aaaaaaaaju","aaaaaaaajv","aaaaaaaajw","aaaaaaaajx","aaaaaaaajy","aaaaaaaajz","aaaaaaaaka","aaaaaaaakb","aaaaaaaakc","aaaaaaaakd","aaaaaaaake","aaaaaaaakf","aaaaaaaakg","aaaaaaaakh","aaaaaaaaki","aaaaaaaakj","aaaaaaaakk","aaaaaaaakl","aaaaaaaakm","aaaaaaaakn","aaaaaaaako","aaaaaaaakp","aaaaaaaakq","aaaaaaaakr","aaaaaaaaks","aaaaaaaakt","aaaaaaaaku","aaaaaaaakv","aaaaaaaakw","aaaaaaaakx","aaaaaaaaky","aaaaaaaakz","aaaaaaaala","aaaaaaaalb","aaaaaaaalc","aaaaaaaald","aaaaaaaale","aaaaaaaalf","aaaaaaaalg","aaaaaaaalh","aaaaaaaali","aaaaaaaalj","aaaaaaaalk","aaaaaaaall","aaaaaaaalm","aaaaaaaaln","aaaaaaaalo","aaaaaaaalp","aaaaaaaalq","aaaaaaaalr","aaaaaaaals","aaaaaaaalt","aaaaaaaalu","aaaaaaaalv","aaaaaaaalw","aaaaaaaalx","aaaaaaaaly","aaaaaaaalz","aaaaaaaama","aaaaaaaamb","aaaaaaaamc",
    //        "aaaaaaaamd","aaaaaaaame","aaaaaaaamf","aaaaaaaamg","aaaaaaaamh","aaaaaaaami","aaaaaaaamj","aaaaaaaamk","aaaaaaaaml","aaaaaaaamm","aaaaaaaamn","aaaaaaaamo","aaaaaaaamp","aaaaaaaamq","aaaaaaaamr","aaaaaaaams","aaaaaaaamt","aaaaaaaamu","aaaaaaaamv","aaaaaaaamw","aaaaaaaamx","aaaaaaaamy","aaaaaaaamz","aaaaaaaana","aaaaaaaanb","aaaaaaaanc","aaaaaaaand","aaaaaaaane","aaaaaaaanf","aaaaaaaang","aaaaaaaanh","aaaaaaaani","aaaaaaaanj","aaaaaaaank","aaaaaaaanl","aaaaaaaanm","aaaaaaaann","aaaaaaaano","aaaaaaaanp","aaaaaaaanq","aaaaaaaanr","aaaaaaaans","aaaaaaaant","aaaaaaaanu","aaaaaaaanv","aaaaaaaanw","aaaaaaaanx","aaaaaaaany","aaaaaaaanz","aaaaaaaaoa","aaaaaaaaob","aaaaaaaaoc","aaaaaaaaod","aaaaaaaaoe","aaaaaaaaof","aaaaaaaaog","aaaaaaaaoh","aaaaaaaaoi","aaaaaaaaoj","aaaaaaaaok","aaaaaaaaol","aaaaaaaaom","aaaaaaaaon","aaaaaaaaoo","aaaaaaaaop","aaaaaaaaoq","aaaaaaaaor","aaaaaaaaos","aaaaaaaaot","aaaaaaaaou","aaaaaaaaov","aaaaaaaaow","aaaaaaaaox","aaaaaaaaoy","aaaaaaaaoz","aaaaaaaapa","aaaaaaaapb","aaaaaaaapc","aaaaaaaapd","aaaaaaaape","aaaaaaaapf","aaaaaaaapg","aaaaaaaaph","aaaaaaaapi","aaaaaaaapj","aaaaaaaapk","aaaaaaaapl","aaaaaaaapm","aaaaaaaapn","aaaaaaaapo","aaaaaaaapp","aaaaaaaapq","aaaaaaaapr","aaaaaaaaps","aaaaaaaapt","aaaaaaaapu","aaaaaaaapv","aaaaaaaapw","aaaaaaaapx","aaaaaaaapy","aaaaaaaapz","aaaaaaaaqa","aaaaaaaaqb","aaaaaaaaqc","aaaaaaaaqd","aaaaaaaaqe","aaaaaaaaqf","aaaaaaaaqg","aaaaaaaaqh","aaaaaaaaqi","aaaaaaaaqj","aaaaaaaaqk","aaaaaaaaql","aaaaaaaaqm","aaaaaaaaqn","aaaaaaaaqo","aaaaaaaaqp","aaaaaaaaqq","aaaaaaaaqr","aaaaaaaaqs","aaaaaaaaqt","aaaaaaaaqu","aaaaaaaaqv","aaaaaaaaqw","aaaaaaaaqx","aaaaaaaaqy","aaaaaaaaqz","aaaaaaaara","aaaaaaaarb","aaaaaaaarc","aaaaaaaard","aaaaaaaare","aaaaaaaarf","aaaaaaaarg","aaaaaaaarh","aaaaaaaari","aaaaaaaarj","aaaaaaaark","aaaaaaaarl","aaaaaaaarm","aaaaaaaarn","aaaaaaaaro","aaaaaaaarp","aaaaaaaarq","aaaaaaaarr","aaaaaaaars","aaaaaaaart","aaaaaaaaru","aaaaaaaarv","aaaaaaaarw","aaaaaaaarx","aaaaaaaary","aaaaaaaarz","aaaaaaaasa","aaaaaaaasb","aaaaaaaasc","aaaaaaaasd","aaaaaaaase","aaaaaaaasf","aaaaaaaasg","aaaaaaaash","aaaaaaaasi","aaaaaaaasj","aaaaaaaask","aaaaaaaasl","aaaaaaaasm","aaaaaaaasn","aaaaaaaaso","aaaaaaaasp","aaaaaaaasq","aaaaaaaasr","aaaaaaaass","aaaaaaaast","aaaaaaaasu","aaaaaaaasv","aaaaaaaasw","aaaaaaaasx","aaaaaaaasy","aaaaaaaasz","aaaaaaaata","aaaaaaaatb","aaaaaaaatc","aaaaaaaatd","aaaaaaaate","aaaaaaaatf","aaaaaaaatg","aaaaaaaath","aaaaaaaati","aaaaaaaatj","aaaaaaaatk","aaaaaaaatl","aaaaaaaatm","aaaaaaaatn","aaaaaaaato","aaaaaaaatp","aaaaaaaatq","aaaaaaaatr","aaaaaaaats","aaaaaaaatt","aaaaaaaatu","aaaaaaaatv","aaaaaaaatw","aaaaaaaatx","aaaaaaaaty","aaaaaaaatz","aaaaaaaaua","aaaaaaaaub","aaaaaaaauc","aaaaaaaaud","aaaaaaaaue","aaaaaaaauf","aaaaaaaaug","aaaaaaaauh","aaaaaaaaui","aaaaaaaauj","aaaaaaaauk","aaaaaaaaul","aaaaaaaaum","aaaaaaaaun","aaaaaaaauo","aaaaaaaaup","aaaaaaaauq","aaaaaaaaur","aaaaaaaaus","aaaaaaaaut","aaaaaaaauu","aaaaaaaauv","aaaaaaaauw","aaaaaaaaux","aaaaaaaauy","aaaaaaaauz","aaaaaaaava","aaaaaaaavb","aaaaaaaavc","aaaaaaaavd","aaaaaaaave","aaaaaaaavf","aaaaaaaavg","aaaaaaaavh","aaaaaaaavi","aaaaaaaavj","aaaaaaaavk","aaaaaaaavl","aaaaaaaavm","aaaaaaaavn","aaaaaaaavo","aaaaaaaavp","aaaaaaaavq","aaaaaaaavr","aaaaaaaavs","aaaaaaaavt","aaaaaaaavu","aaaaaaaavv","aaaaaaaavw","aaaaaaaavx","aaaaaaaavy","aaaaaaaavz","aaaaaaaawa","aaaaaaaawb","aaaaaaaawc","aaaaaaaawd","aaaaaaaawe","aaaaaaaawf","aaaaaaaawg","aaaaaaaawh","aaaaaaaawi","aaaaaaaawj","aaaaaaaawk","aaaaaaaawl","aaaaaaaawm","aaaaaaaawn","aaaaaaaawo","aaaaaaaawp","aaaaaaaawq","aaaaaaaawr","aaaaaaaaws","aaaaaaaawt","aaaaaaaawu","aaaaaaaawv","aaaaaaaaww","aaaaaaaawx","aaaaaaaawy","aaaaaaaawz","aaaaaaaaxa","aaaaaaaaxb","aaaaaaaaxc","aaaaaaaaxd","aaaaaaaaxe","aaaaaaaaxf","aaaaaaaaxg","aaaaaaaaxh","aaaaaaaaxi","aaaaaaaaxj","aaaaaaaaxk","aaaaaaaaxl","aaaaaaaaxm","aaaaaaaaxn","aaaaaaaaxo","aaaaaaaaxp","aaaaaaaaxq","aaaaaaaaxr","aaaaaaaaxs","aaaaaaaaxt","aaaaaaaaxu","aaaaaaaaxv","aaaaaaaaxw","aaaaaaaaxx","aaaaaaaaxy","aaaaaaaaxz","aaaaaaaaya","aaaaaaaayb","aaaaaaaayc","aaaaaaaayd","aaaaaaaaye","aaaaaaaayf","aaaaaaaayg","aaaaaaaayh","aaaaaaaayi","aaaaaaaayj","aaaaaaaayk","aaaaaaaayl","aaaaaaaaym","aaaaaaaayn","aaaaaaaayo","aaaaaaaayp","aaaaaaaayq","aaaaaaaayr","aaaaaaaays","aaaaaaaayt","aaaaaaaayu","aaaaaaaayv","aaaaaaaayw","aaaaaaaayx","aaaaaaaayy","aaaaaaaayz","aaaaaaaaza","aaaaaaaazb","aaaaaaaazc","aaaaaaaazd","aaaaaaaaze","aaaaaaaazf","aaaaaaaazg","aaaaaaaazh","aaaaaaaazi","aaaaaaaazj","aaaaaaaazk","aaaaaaaazl","aaaaaaaazm","aaaaaaaazn","aaaaaaaazo","aaaaaaaazp","aaaaaaaazq","aaaaaaaazr","aaaaaaaazs","aaaaaaaazt","aaaaaaaazu","aaaaaaaazv","aaaaaaaazw","aaaaaaaazx","aaaaaaaazy","aaaaaaaazz"
    //    };

    //    vector<string> res;
    //    {
    //        FindWords(words, board, res);
    //    }
    //    cout << " RESULT:" << endl;
    //    for (auto const& res_word : res)
    //    {
    //        std::cout << " " << res_word << " ";
    //    }
    //    std::cout << '\n';
    //    if (res.size() == 0)
    //    {
    //        std::cout << "Not found" << '\n';
    //    }
    //}
    ChronoProfiler::printTime("prune");
    ChronoProfiler::printTime("build_tree");
    ChronoProfiler::printTime("find_words");
    return 0;
}