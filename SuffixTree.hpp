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
	struct PrefixPaths
	{
		vector<vector<int>> paths;
		vector<int> valid_paths;
	};
	unordered_map<int, PrefixPaths> paths;

	void AddPath(const vector<int>& path)
	{
		auto& container = paths[path.back()];
		container.paths.push_back(path);
		container.valid_paths.push_back(-1);
	}
};

struct Node
{
	std::string_view pattern;
	int prefix_idx;
	bool bIsFinal = false;
	Paths paths;
	Node* parent{ nullptr };
	Node* children[26];
	int children_num{ 0 };

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

	bool Insert(const std::string_view& word)
	{
		string_view pattern_suffix{ get_pattern() };
		string_view word_suffix{ word };
		word_suffix.remove_prefix(prefix_idx);
		if (int index = intersection(pattern_suffix, word_suffix))
		{
			if (index == 0)
			{
				return false;
			}
			else if (index == pattern_suffix.size())
			{
				if (index == word_suffix.size())
				{
					bIsFinal = true;
					return true;
				}
				for (const auto& c : children)
				{
					if (c && c->Insert(word))
					{
						return true;
					}
				}
				int new_prefix{ prefix_idx + index };
				Node* node = new Node{ word, new_prefix, true, {} };
				assert(node->pattern.size() > prefix_idx);
				node->parent = this;
				children[word[new_prefix]-'a'] = node;
				++children_num;
				return true;
			}

			int new_prefix = prefix_idx + index;
			Node* split_node = new Node{ pattern, new_prefix, bIsFinal, {}};
			assert(split_node->pattern.size() > split_node->prefix_idx);
			split_node->parent = this;
			split_node->children_num = children_num;
			split_node->children[pattern[new_prefix] - 'a'] = split_node;
			if (index < word_suffix.size())
			{
				Node* subword_node = new Node{ string_view{ word.data(), word.size()}, new_prefix, true, { } };
				assert(subword_node->pattern.size() > subword_node->prefix_idx);
				subword_node->parent = this;
				split_node->children[word[new_prefix] - 'a'] = subword_node;
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
			bIsFinal = index == word_suffix.size();
			return true;
		}
		return false;
	}

	void print(int indent)
	{
		for (int i = 0; i < indent; ++i)
		{
			cout << "---";
		}
		cout << pattern << bIsFinal << endl;
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
			delete Root;
			Root = nullptr;
		}
	}

	void Build(const vector<string>& words, const vector<bool>& mask)
	{
		for (int i = 0; i < words.size(); ++i)
		{
			if (!mask[i])
			{
				continue;
			}
			const auto& word{ words[i] };

			int idx = word[0] - 'a';
			if (!Roots[idx])
			{
				Roots[idx] = new Node{ word, 0, true, {} };
				continue;
			}
			Roots[idx]->Insert(word);
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