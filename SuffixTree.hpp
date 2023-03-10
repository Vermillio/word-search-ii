#pragma once

#include <deque>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

int intersection(const string_view& a, const string_view& b)
{
	int i = 0;
	while (i < a.size() && i < b.size())
	{
		if (a[i] != b[i])
		{
			return i;
		}
		++i;
	}
	return i;
}

struct Paths
{
	struct Path
	{
		int valid;
		vector<int> path;
	};
	unordered_map<int, vector<Path>> paths;
};

struct Node
{
	std::string_view pattern;
	int prefix_idx;
	Node* parent{ nullptr };
	Node* children[26];
	int children_num{ 0 };
	int mask{ 0 }; // 0 - not a word, 1 - forward, 2 - reversed, 3 - both reversed and forward
	Paths paths;

	~Node()
	{
		for (auto& c : children)
		{
			if (c)
			{
				delete c;
			}
			c = nullptr;
		}
	}

	string_view get_pattern() const
	{
		string_view res{ pattern };
		res.remove_prefix(prefix_idx);
		return res;
	}

	Node* Insert(const std::string_view& word)
	{
		string_view pattern_suffix{ get_pattern() };
		string_view word_suffix{ word };
		word_suffix.remove_prefix(prefix_idx);
		if (int index = intersection(pattern_suffix, word_suffix))
		{
			if (index == 0)
			{
				return nullptr;
			}
			else if (index == pattern_suffix.size())
			{
				if (index == word_suffix.size())
				{
					return this;
				}
				int new_prefix{ prefix_idx + index };
				const auto& c = children[word[new_prefix] - 97];
				if (c)
				{
					Node* node = c->Insert(word);
					assert(node);
					return node;
				}
				Node* node = new Node{ word, new_prefix, {} };
				assert(node->pattern.size() > prefix_idx);
				node->parent = this;
				children[word[new_prefix]-97] = node;
				++children_num;
				return node;
			}

			int new_prefix = prefix_idx + index;
			Node* split_node = new Node{ pattern, new_prefix, {}};
			assert(split_node->pattern.size() > split_node->prefix_idx);
			split_node->mask = mask;
			mask = 0;
			split_node->parent = this;
			split_node->children_num = children_num;
			split_node->children[pattern[new_prefix] - 97] = split_node;
			Node* out_node = this;
			if (index < word_suffix.size())
			{
				out_node = new Node{ string_view{ word.data(), word.size()}, new_prefix, { } };
				assert(out_node->pattern.size() > out_node->prefix_idx);
				out_node->parent = this;
				split_node->children[word[new_prefix] - 97] = out_node;
			}

			pattern.remove_suffix(pattern_suffix.size() - index);
			assert(pattern.size() > prefix_idx);

			for (auto& c : children)
			{
				if (c)
				{
					c->parent = split_node;
				}
			}
			swap(split_node->children, children);
			children_num = 2;
			return out_node;
		}
		return nullptr;
	}

	void print(int indent)
	{
		for (int i = 0; i < indent; ++i)
		{
			cout << "---";
		}
		cout << get_pattern() << endl;
		for (auto& c : children)
		{
			if (c)
			{
				c->print(indent + 1);
			}
		}
	}
};

struct SuffixTree
{
	Node* Roots[26];

	SuffixTree()
	{
		memset(Roots, 0, 26 * sizeof(Node*));
	}

	~SuffixTree()
	{
		for (auto& Root : Roots)
		{
			if (Root)
			{
				delete Root;
				Root = nullptr;
			}
		}
	}

	void Build(vector<string>& words, const vector<vector<char>>& board)
	{
		int grid_size = board.size() * board[0].size();
		int occ_table[26];
		memset(occ_table, 0, 26 * sizeof(int));
		for (auto& row : board)
		{
			for (auto& col : row)
			{
				++occ_table[col - 97];
			}
		}
		int occ[26];
		for (int i = 0; i < words.size(); ++i)
		{
			auto& word{ words[i] };
			copy(occ_table, occ_table + 26, occ);
			if (word.size() > grid_size)
			{
				continue;
			}
			int mask{ 1 };
			for (auto& c : word)
			{
				if (--occ[c - 97] < 0)
				{
					mask = 0;
					break;
				}
			}

			if (mask == 0) continue;

			int left = word.find_first_not_of(word[0]);
			int right = word.size() - word.find_last_not_of(word[word.size() - 1]);
			if (left > right)
			{
				reverse(begin(word), end(word));
				mask = 2;
			}

			int idx = word[0] - 97;
			if (!Roots[idx])
			{
				Roots[idx] = new Node{ word, 0, {} };
				Roots[idx]->mask = mask;
				continue;
			}
			Node* node = Roots[idx]->Insert(word);
			if (node->mask < 3)
			{
				node->mask += mask;
			}
		}
	}

	void print()
	{
		for (auto& R : Roots)
		{
			if (R)
			{
				R->print(0);
			}
		}
	}
};