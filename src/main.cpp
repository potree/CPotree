
#include <string>
#include <functional>

#include "json/json.hpp"

#include "unsuck/unsuck.hpp"

using std::string; 
using std::function;
using json = nlohmann::json;


enum NodeType {
	NORMAL = 0,
	LEAF = 1,
	PROXY = 2,
};

struct Node {

	string name = "";
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

//
// hierarchy, which is stored in chunks.
// note that byteOffset and byteSize have different meanings for proxy nodes and regular nodes.
// proxy nodes: there is more hierarchy below this node, but it's stored in another hierarchy chunk
//     byteOffset and byteSize specify the location of additional hierarchy data in hierarchy.bin
// regular nodes: node in the octree containing points. proxy nodes will eventually be replaced by regular nodes
//     as additional chunks of the hierarchy are loaded.
//     byteOffset and byteSize specify the location of point data in octree.bin
//
void loadHierarchyRecursive(Hierarchy& hierarchy, Node* root, shared_ptr<Buffer> data, int64_t offset, int64_t size) {

	int64_t bytesPerNode = 22;
	int64_t numNodes = size / bytesPerNode;

	vector<Node*> nodes;
	nodes.reserve(numNodes);
	nodes.push_back(root);

	for (int i = 0; i < numNodes; i++) {

		auto current = nodes[i];

		uint64_t offsetNode = offset + i * bytesPerNode;
		uint8_t type = data->read<uint8_t>(offsetNode + 0);
		int32_t childMask = data->read<uint8_t>(offsetNode + 1);
		uint32_t numPoints = data->read<uint32_t>(offsetNode + 2);
		int64_t byteOffset = data->read<int64_t>(offsetNode + 6);
		int64_t byteSize = data->read<int64_t>(offsetNode + 14);

		current->byteOffset = byteOffset;
		current->byteSize = byteSize;
		current->numPoints = numPoints;
		current->nodeType = type;

		if (current->nodeType == NodeType::PROXY) {
			// load proxy node
			loadHierarchyRecursive(hierarchy, current, data, byteOffset, byteSize);
		} else {
			// load child node data for current node

			for (int32_t childIndex = 0; childIndex < 8; childIndex++) {
				bool childExists = ((1 << childIndex) & childMask) != 0;

				if (!childExists) {
					continue;
				}

				string childName = current->name + to_string(childIndex);

				Node* child = new Node();
				child->name = childName;
				current->children[childIndex] = child;
				child->parent = current;

				nodes.push_back(child);
			}
		}

		
		
	}

}

Hierarchy loadHierarchy(string path, json& metadata) {

	string hierarchyPath = path + "/hierarchy.bin";
	auto data = readBinaryFile(hierarchyPath);

	auto jsHierarchy = metadata["hierarchy"];
	int64_t firstChunkSize = jsHierarchy["firstChunkSize"];
	int64_t stepSize = jsHierarchy["stepSize"];
	int64_t depth = jsHierarchy["depth"];

	Node* root = new Node();
	root->name = "r";

	Hierarchy hierarchy;
	hierarchy.root = root;

	int64_t offset = 0;
	loadHierarchyRecursive(hierarchy, root, data, offset, firstChunkSize);

	return hierarchy;
}

int main(int argc, char** argv) {

	auto tStart = now();

	string path = "D:/temp/test/eclepens.las";
	string metadataPath = path + "/metadata.json";
	string octreePath = path + "/octree.bin";

	string strMetadata = readTextFile(metadataPath);
	json jsMetadata = json::parse(strMetadata);

	auto hierarchy = loadHierarchy(path, jsMetadata);

	int64_t numNodes = 0;
	hierarchy.root->traverse([&numNodes](Node* node) {

		numNodes++;
		//cout << node->name << endl;

	});

	printElapsedTime("duration", tStart);

	cout << numNodes << endl;
	





	//auto loader = PotreeLoader(path);

	//auto nodes = intersectingNodes(loader, area);

	//for (auto& node : nodes) {
	//	// filter
	//}


	return 0;
}