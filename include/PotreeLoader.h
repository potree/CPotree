
#pragma once

#include "json/json.hpp"

#include "Attributes.h"
#include "unsuck/unsuck.hpp"
#include "Node.h"

using json = nlohmann::json;


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
				child->aabb = childAABB(current->aabb, childIndex);
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

	AABB aabb;
	{
		aabb.min.x = metadata["boundingBox"]["min"][0];
		aabb.min.y = metadata["boundingBox"]["min"][1];
		aabb.min.z = metadata["boundingBox"]["min"][2];

		aabb.max.x = metadata["boundingBox"]["max"][0];
		aabb.max.y = metadata["boundingBox"]["max"][1];
		aabb.max.z = metadata["boundingBox"]["max"][2];
	}

	Node* root = new Node();
	root->name = "r";
	root->aabb = aabb;

	Hierarchy hierarchy;

	int64_t offset = 0;
	loadHierarchyRecursive(hierarchy, root, data, offset, firstChunkSize);

	vector<Node*> nodes;
	root->traverse([&nodes](Node* node) {
		nodes.push_back(node);
		});

	hierarchy.root = root;
	hierarchy.nodes = nodes;

	return hierarchy;
}

Attributes parseAttributes(json& metadata) {
	vector<Attribute> attributeList;
	auto jsAttributes = metadata["attributes"];
	for (auto jsAttribute : jsAttributes) {

		string name = jsAttribute["name"];
		string description = jsAttribute["description"];
		int size = jsAttribute["size"];
		int numElements = jsAttribute["numElements"];
		int elementSize = jsAttribute["elementSize"];
		AttributeType type = typenameToType(jsAttribute["type"]);

		auto jsMin = jsAttribute["min"];
		auto jsMax = jsAttribute["max"];

		Attribute attribute(name, size, numElements, elementSize, type);

		if (numElements >= 1) {
			attribute.min.x = jsMin[0] == nullptr ? Infinity : double(jsMin[0]);
			attribute.max.x = jsMax[0] == nullptr ? Infinity : double(jsMax[0]);
		}
		if (numElements >= 2) {
			attribute.min.y = jsMin[1] == nullptr ? Infinity : double(jsMin[1]);
			attribute.max.y = jsMax[1] == nullptr ? Infinity : double(jsMax[1]);
		}
		if (numElements >= 3) {
			attribute.min.z = jsMin[2] == nullptr ? Infinity : double(jsMin[2]);
			attribute.max.z = jsMax[2] == nullptr ? Infinity : double(jsMax[2]);
		}

		attributeList.push_back(attribute);
	}

	double scaleX = metadata["scale"][0];
	double scaleY = metadata["scale"][1];
	double scaleZ = metadata["scale"][2];

	double offsetX = metadata["offset"][0];
	double offsetY = metadata["offset"][1];
	double offsetZ = metadata["offset"][2];

	Attributes attributes(attributeList);
	attributes.posScale = { scaleX, scaleY, scaleZ };
	attributes.posOffset = { offsetX, offsetY, offsetZ };

	return attributes;
}

struct Metadata {

	AABB aabb;
	dvec3 scale;
	dvec3 offset;

};


struct PotreeLoader {

	string path;
	Metadata metadata;

	PotreeLoader() {

	}

	void loadMetadata() {
		
		string metadataPath = path + "/metadata.json";
		string strMetadata = readTextFile(metadataPath);
		json jsMetadata = json::parse(strMetadata);

		{ // AABB
			metadata.aabb.min.x = jsMetadata["boundingBox"]["min"][0];
			metadata.aabb.min.y = jsMetadata["boundingBox"]["min"][1];
			metadata.aabb.min.z = jsMetadata["boundingBox"]["min"][2];

			metadata.aabb.max.x = jsMetadata["boundingBox"]["max"][0];
			metadata.aabb.max.y = jsMetadata["boundingBox"]["max"][1];
			metadata.aabb.max.z = jsMetadata["boundingBox"]["max"][2];
		}

		{ // SCALE
			metadata.scale.x = jsMetadata["scale"][0];
			metadata.scale.y = jsMetadata["scale"][1];
			metadata.scale.z = jsMetadata["scale"][2];
		}

		{ // OFFSET
			metadata.offset.x = jsMetadata["offset"][0];
			metadata.offset.y = jsMetadata["offset"][1];
			metadata.offset.z = jsMetadata["offset"][2];
		}
	}

	static PotreeLoader load(string path) {

		PotreeLoader loader;
		loader.path = path;
		loader.loadMetadata();


		return loader;
	}

};


