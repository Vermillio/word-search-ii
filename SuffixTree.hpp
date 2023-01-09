#pragma once

#include <string>
#include <vector>
#include <iostream>

using namespace std;

int intersection(const string& a, const string& b)
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
		struct explored_path
		{
			vector<int> path;
		};
		vector<explored_path> paths;
		vector<int> valid_paths;
	};
	unordered_map<int, PrefixPaths> paths;

	void AddPath(const vector<int>& path)
	{
		auto& container = paths[path.back()];
		container.paths.push_back({ path });
		container.valid_paths.push_back(-1);
	}

};

struct Node
{
	std::string pattern;
	bool bIsFinal = false;
	Paths paths;
	Node* parent{ nullptr };
	std::vector<Node*> children;

	~Node()
	{
		for (auto& child : children)
		{
			delete child;
			child = nullptr;
		}
	}

	bool Insert(const std::string& word)
	{
		if (int index = intersection(pattern, word))
		{
			if (index == 0)
			{
				return false;
			}
			else if (index == pattern.size())
			{
				if (index == word.size())
				{
					bIsFinal = true;
					return true;
				}
				string subword = string{ word.begin() + index, word.end() };
				for (const auto& C : children)
				{
					if (C->Insert(subword))
					{
						return true;
					}
				}
				Node* node = new Node{ subword, true, {} };
				node->parent = this;
				children.push_back(node);
				return true;
			}
			
			Node* split_node = new Node{ string{pattern.begin() + index, pattern.end() }, bIsFinal, {} };
			split_node->parent = this;
			split_node->children.push_back(split_node);
			
			if (index < word.size())
			{
				Node* subword_node = new Node{ string{ word.begin() + index, word.end()}, true, { } };
				subword_node->parent = this;
				split_node->children.push_back(subword_node);
			}

			pattern = std::string{ pattern.begin(), pattern.begin() + index };

			for (auto& c : children)
			{
				c->parent = split_node;
			}
			swap(split_node->children, children);
			bIsFinal = index == word.size();
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
		//cout << pattern << endl;
		for (auto& cl : children)
		{
			cl->print(indent + 1);
		}
	}

};

struct SuffixTree
{
	std::vector<Node*> Roots;

	~SuffixTree()
	{
		for (auto& Root : Roots)
		{
			delete Root;
			Root = nullptr;
		}
	}

	void Build(const std::vector<std::string>& words)
	{
		for (const auto& word : words)
		{
			bool Inserted = false;
			for (const auto& Root : Roots)
			{
				if (Root->Insert(word))
				{
					Inserted = true;
					break;
				}
			}
			if (!Inserted)
			{
				Roots.push_back(new Node{ word, true, {} });
			}
		}
	}


	void print()
	{
		for (auto& R : Roots)
		{
			R->print(0);
		}
	}


	template<typename Object, typename Predicate>
	void DFS(Object object, Predicate pred)
	{
		for (auto& R : Roots)
		{
			if (R->DFS(object, pred))
			{
				return;
			}
		}
	}
};