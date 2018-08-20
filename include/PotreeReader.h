#pragma once

#include "StandardIncludes.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#include "Point.h"
#include "pmath.h"

using rapidjson::Document;
using rapidjson::Value;

class PotreeReader;

class PointAttribute{
public:
	static const PointAttribute POSITION_CARTESIAN;
	static const PointAttribute POSITION_PROJECTED_PROFILE;
	static const PointAttribute COLOR_PACKED;
	static const PointAttribute RGB;
	static const PointAttribute INTENSITY;
	static const PointAttribute CLASSIFICATION;
	static const PointAttribute NORMAL_SPHEREMAPPED;
	static const PointAttribute NORMAL_OCT16;
	static const PointAttribute NORMAL;

	int ordinal;
	string name;
	int numElements;
	int byteSize;

	PointAttribute(int ordinal, string name, int numElements, int byteSize){
		this->ordinal = ordinal;
		this->name = name;
		this->numElements = numElements;
		this->byteSize = byteSize;
	}

	static PointAttribute fromString(string name);

};

inline bool operator==(const PointAttribute& lhs, const PointAttribute& rhs){
	return lhs.ordinal == rhs.ordinal;
}


struct PointAttributes{

	vector<PointAttribute> attributes;
	int byteSize = 0;

	PointAttributes(){

	}

	PointAttributes(vector<PointAttribute> attributes){
		this->attributes = attributes;

		for(auto attribute : attributes){
			byteSize += attribute.byteSize;
		}
	}

};

class PotreeMetadata {
public:

	string version = "";
	string software = "";
	AABB boundingBox;
	PointAttributes pointAttributes;
	double spacing = 0;
	glm::dvec3 scale;
	int hierarchyStepSize = 0;
	string projection = "";

	PotreeMetadata() {

	}

	static PotreeMetadata fromString(string content) {
		PotreeMetadata metadata;

		Document d;
		d.Parse(content.c_str());

		Value &vVersion = d["version"];
		Value &vBoundingBox = d["boundingBox"];
		Value &vPointAttributes = d["pointAttributes"];
		Value &vSpacing = d["spacing"];
		Value &vScale = d["scale"];
		Value &vHierarchyStepSize = d["hierarchyStepSize"];

		metadata.version = vVersion.GetString();

		if (d.HasMember("projection")) {
			Value &vProjection = d["projection"];
			metadata.projection = vProjection.GetString();
		}

		{
			glm::dvec3 min{
				vBoundingBox["lx"].GetDouble(),
				vBoundingBox["ly"].GetDouble(),
				vBoundingBox["lz"].GetDouble()
			};

			glm::dvec3 max{
				vBoundingBox["ux"].GetDouble(),
				vBoundingBox["uy"].GetDouble(),
				vBoundingBox["uz"].GetDouble()
			};

			metadata.boundingBox.min = min;
			metadata.boundingBox.max = max;
		}

		if (vPointAttributes.IsArray()) {
			vector<PointAttribute> attributes;

			for (Value::ConstValueIterator itr = vPointAttributes.Begin(); itr != vPointAttributes.End(); ++itr) {
				string strpa = itr->GetString();
				PointAttribute pa = PointAttribute::fromString(strpa);
				attributes.push_back(pa);
			}
			metadata.pointAttributes = attributes;
		}

		metadata.spacing = vSpacing.GetDouble();
		metadata.scale = glm::dvec3(
			vScale.GetDouble(),
			vScale.GetDouble(),
			vScale.GetDouble());

		metadata.hierarchyStepSize = vHierarchyStepSize.GetInt();

		return metadata;
	}
};


struct PRNode{

	bool loaded = false;
	bool hierarchyLoaded = false;
	int index = -1;
	int level = 0;
	AABB boundingBox;
	int numPoints = 0;
	string name = "";
	PRNode *parent = nullptr;
	vector<PRNode*> _children = vector<PRNode*>(8, nullptr);
	PotreeReader *reader = nullptr;
	vector<Point> _points;

	PRNode(PotreeReader *reader){
		this->reader = reader;
	}

	vector<PRNode*> children();


	vector<Point> points();

};

class PotreeReader{
public:

	string file = "";
	PRNode* root = nullptr;
	PotreeMetadata metadata;

	PotreeReader(string file){
		this->file = file;

		loadMetadata();

		root = new PRNode(this);
		root->name = "r";
		root->boundingBox = metadata.boundingBox;

		loadHierarchy(root);
	}

	string getHierarchyPath(PRNode *node){
		string path = "r/";

		int hierarchyStepSize = metadata.hierarchyStepSize;
		string indices = node->name.substr(1);

		int numParts = floor(indices.size() / hierarchyStepSize);
		for(int i = 0; i < numParts; i++){
			path += indices.substr(i * hierarchyStepSize, hierarchyStepSize) + "/";
		}

		path.pop_back();

		return path;
	}

	void loadMetadata(){
		ifstream in(file, std::ios::binary);

		stringstream buffer;
		buffer << in.rdbuf();

		metadata = PotreeMetadata::fromString(buffer.str());

		in.close();
	}

	void load(PRNode *node);

	void loadHierarchy(PRNode *node){
		if(node->hierarchyLoaded){
			return;
		}

		node->hierarchyLoaded = true;
		size_t rIndex = file.rfind("cloud.js");
		string basePath = file.substr(0, rIndex);
		string fHierarchy = basePath + "data/" + getHierarchyPath(node) + "/" + node->name + ".hrc";
		fHierarchy = fs::canonical(fHierarchy).string();

		if(!fs::exists(fHierarchy)){
			return;
		}


		ifstream in(fHierarchy, std::ios::binary);
		int hierarchyByteSize = fs::file_size(fHierarchy);
		int numNodes = hierarchyByteSize / 5;

		vector<char> buffer(hierarchyByteSize);
		in.read(reinterpret_cast<char*>(&buffer[0]), hierarchyByteSize);

		vector<PRNode*> nodes = {node};

		for(int i = 0; i < numNodes; i++){

			PRNode *node = nodes[i];

			unsigned char childBitset = buffer[5*i];
			unsigned int numPoints = reinterpret_cast<unsigned int*>(&buffer[5*i + 1])[0];

			node->numPoints = numPoints;

			if(childBitset == 0){
				// no children
				continue;
			}

			node->_children.resize(8, nullptr);

			for(int j = 0; j < 8; j++){

				if((childBitset & (1 << j)) == 0){
					// no child at index j
					continue;
				}

				PRNode *child = new PRNode(this);
				child->index = j;
				child->parent = node;
				child->name = node->name + to_string(j);
				child->level = node->level + 1;
				child->boundingBox = childAABB(node->boundingBox, j);
				child->hierarchyLoaded = (child->level % metadata.hierarchyStepSize) != 0;
				node->_children[j] = child;

				nodes.push_back(child);
			}

		}

	}
};
