
#pragma once

#include <string>
#include <functional>

#include "pmath.h"

using std::string;
using std::function;

enum NodeType {
	NORMAL = 0,
	LEAF = 1,
	PROXY = 2,
};

struct Node {

	string name = "";
	AABB aabb;

	Node* parent = nullptr;
	Node* children[8] = {};
	int32_t nodeType = -1;
	int64_t byteOffset = 0;
	int64_t byteSize = 0;
	int64_t numPoints = 0;

	void traverse(function<void(Node*)> callback) {

		callback(this);

		for (auto child : children) {
			if (child != nullptr) {
				child->traverse(callback);
			}
		}

	}

};

struct Hierarchy {
	Node* root = nullptr;
	vector<Node*> nodes;
};