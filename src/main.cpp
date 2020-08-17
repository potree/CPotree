
#include <string>
#include <functional>
#include <algorithm>
#include <execution>
#include <atomic>
#include <mutex>
#include <regex>

#include "json/json.hpp"

#include "laszip/laszip_api.h"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/constants.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include "pmath.h"
#include "unsuck/unsuck.hpp"
#include "arguments/Arguments.hpp"

#include "Attributes.h"

using std::string; 
using std::function;
using json = nlohmann::json;
using std::for_each;
using std::atomic_int64_t;
using std::mutex;
using std::lock_guard;
using std::regex;

using glm::dvec2;
using glm::dvec3;
using glm::dvec4;
using glm::dmat4;

//struct AABB {
//	dvec3 min = { Infinity, Infinity, Infinity };
//	dvec3 max = { -Infinity, -Infinity, -Infinity };
//
//	dvec3 size() {
//		return max - min;
//	}
//
//	void expand(double x, double y, double z) {
//		this->min.x = std::min(x, this->min.x);
//		this->min.y = std::min(y, this->min.y);
//		this->min.z = std::min(z, this->min.z);
//		this->max.x = std::max(x, this->max.x);
//		this->max.y = std::max(y, this->max.y);
//		this->max.z = std::max(z, this->max.z);
//	}
//};

//AABB childAABB(AABB& aabb, int& index) {
//
//	dvec3 min = aabb.min;
//	dvec3 max = aabb.max;
//
//	auto size = aabb.size();
//
//	if ((index & 0b0001) > 0) {
//		min.z += size.z / 2;
//	} else {
//		max.z -= size.z / 2;
//	}
//
//	if ((index & 0b0010) > 0) {
//		min.y += size.y / 2;
//	} else {
//		max.y -= size.y / 2;
//	}
//
//	if ((index & 0b0100) > 0) {
//		min.x += size.x / 2;
//	} else {
//		max.x -= size.x / 2;
//	}
//
//	return { min, max };
//}

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

struct AcceptedItem {
	Node* node;
	shared_ptr<Buffer> data;
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
				child->aabb = childAABB(current->aabb, childIndex);
				child->name = childName;
				current->children[childIndex] = child;
				child->parent = current;

				nodes.push_back(child);
			}
		}

		
		
	}
}

//vector<Node*> filterArea(vector<Node*>& nodes, dvec3 min, dvec3 max) {
//
//
//
//}

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

vector<function<void(int64_t, uint8_t*)>> createAttributeHandlers(laszip_header* header, laszip_point* point, Attributes& attributes) {

	vector<function<void(int64_t, uint8_t*)>> handlers;

	{ // STANDARD LAS ATTRIBUTES

		unordered_map<string, function<void(int64_t, uint8_t*)>> mapping;

		{ // POSITION
			int offset = attributes.getOffset("position");
			auto handler = [point, offset, attributes](int64_t index, uint8_t* data) {
				memcpy(&point->X, data + index * attributes.bytes + offset + 0, 4);
				memcpy(&point->Y, data + index * attributes.bytes + offset + 4, 4);
				memcpy(&point->Z, data + index * attributes.bytes + offset + 8, 4);
			};

			mapping["position"] = handler;
		}

		{ // RGB
			int offset = attributes.getOffset("rgb");
			auto handler = [point, offset, attributes](int64_t index, uint8_t* data) {
				auto& rgba = point->rgb;

				memcpy(&rgba, data + index * attributes.bytes + offset, 6);
			};

			mapping["rgb"] = handler;
		}

		{ // INTENSITY
			int offset = attributes.getOffset("intensity");
			auto handler = [point, offset, attributes](int64_t index, uint8_t* data) {
				memcpy(&point->intensity, data + index * attributes.bytes + offset, 2);
			};

			mapping["intensity"] = handler;
		}


		for (auto& attribute : attributes.list) {

			if (mapping.find(attribute.name) != mapping.end()) {
				handlers.push_back(mapping[attribute.name]);
			}
		}

	}

	return handlers;
}


void write(string path, vector<AcceptedItem>& items, json& jsMetadata, Attributes attributes) {
	
	AABB aabb;
	int64_t totalPoints = 0;
	for (auto& item : items) {
		int64_t numPoints = item.data->size / attributes.bytes;

		totalPoints += numPoints;

		auto min = item.node->aabb.min;
		auto max = item.node->aabb.max;
		aabb.expand(min.x, min.y, min.z);
		aabb.expand(max.x, max.y, max.z);
	}

	laszip_POINTER laszip_writer;
	laszip_point* point;

	laszip_create(&laszip_writer);

	laszip_header header;
	memset(&header, 0, sizeof(laszip_header));

	dvec3 scale;
	scale.x = jsMetadata["scale"][0];
	scale.y = jsMetadata["scale"][1];
	scale.z = jsMetadata["scale"][2];

	dvec3 offset;
	offset.x = jsMetadata["offset"][0];
	offset.y = jsMetadata["offset"][1];
	offset.z = jsMetadata["offset"][2];

	header.version_major = 1;
	header.version_minor = 2;
	header.header_size = 227;
	header.offset_to_point_data = 227;
	header.point_data_format = 2;

	header.min_x = aabb.min.x;
	header.min_y = aabb.min.y;
	header.min_z = aabb.min.z;
	header.max_x = aabb.max.x;
	header.max_y = aabb.max.y;
	header.max_z = aabb.max.z;

	header.x_scale_factor = scale.x;
	header.y_scale_factor = scale.y;
	header.z_scale_factor = scale.z;
		  
	header.x_offset = offset.x;
	header.y_offset = offset.y;
	header.z_offset = offset.z;
		  
	header.point_data_record_length = 26;
	header.number_of_point_records = totalPoints;

	laszip_BOOL request_writer = 1;
	laszip_request_compatibility_mode(laszip_writer, request_writer);

	laszip_set_header(laszip_writer, &header);

	laszip_open_writer(laszip_writer, path.c_str(), true);

	laszip_get_point_pointer(laszip_writer, &point);

	auto handlers = createAttributeHandlers(&header, point, attributes);

	//auto handler = [point, attributes](int64_t index, uint8_t* data) {
	//	memcpy(&point->X, data + index * attributes.bytes + 0, 4);
	//	memcpy(&point->Y, data + index * attributes.bytes + 4, 4);
	//	memcpy(&point->Z, data + index * attributes.bytes + 8, 4);
	//};

	for (auto& item : items) {
		
		int64_t numPoints = item.data->size / attributes.bytes;

		for (int64_t i = 0; i < numPoints; i++) {

			//handler(i, item.data->data_u8);

			for (auto& handler : handlers) {
				handler(i, item.data->data_u8);
			}

			laszip_write_point(laszip_writer);
		}
	}

	laszip_close_writer(laszip_writer);

}

struct AreaMinMax {
	dvec3 min = { -Infinity, -Infinity, -Infinity };
	dvec3 max = { Infinity, Infinity, Infinity };
};

struct AreaMatrix {
	dmat4 transform;
};

struct Area {
	vector<AreaMinMax> minmaxs;
	vector<OrientedBox> orientedBoxes;
};


vector<AreaMinMax> parseAreaMinMax(string strArea) {
	vector<AreaMinMax> areasMinMax;

	auto matches = getRegexMatches(strArea, "minmax\\([^\\)]*\\)");
	for (string match : matches) {
		cout << match << endl;

		AreaMinMax minmax;

		auto arrayMatches = getRegexMatches(match, "\\[[^\\]]*\\]");

		if (arrayMatches.size() == 2) {

			auto strip = [](string str) {
				str.erase(remove_if(str.begin(), str.end(), isspace), str.end());
				str.erase(std::remove(str.begin(), str.end(), '['), str.end());
				str.erase(std::remove(str.begin(), str.end(), ']'), str.end());

				return str;
			};

			{ // min
				string strMin = arrayMatches[0];
				strMin = strip(strMin);

				auto tokensMin = getRegexMatches(strMin, "[^\,]+");

				if (tokensMin.size() == 2) {
					double x = stod(tokensMin[0]);
					double y = stod(tokensMin[1]);

					minmax.min.x = x;
					minmax.min.y = y;
				} else if (tokensMin.size() == 3) {
					double x = stod(tokensMin[0]);
					double y = stod(tokensMin[1]);
					double z = stod(tokensMin[2]);

					minmax.min.x = x;
					minmax.min.y = y;
					minmax.min.z = z;
				} else {
					GENERATE_ERROR_MESSAGE << "could not parse area. Expected two or three min values, got: " << tokensMin.size() << endl;
				}
			}

			{ // max
				string strMax = arrayMatches[1];
				strMax = strip(strMax);
				auto tokensMax = getRegexMatches(strMax, "[^\,]+");

				if (tokensMax.size() == 2) {
					double x = stod(tokensMax[0]);
					double y = stod(tokensMax[1]);

					minmax.max.x = x;
					minmax.max.y = y;
				} else if (tokensMax.size() == 3) {
					double x = stod(tokensMax[0]);
					double y = stod(tokensMax[1]);
					double z = stod(tokensMax[2]);

					minmax.max.x = x;
					minmax.max.y = y;
					minmax.max.z = z;
				} else {
					GENERATE_ERROR_MESSAGE << "could not parse area. Expected two or three max values, got: " << tokensMax.size() << endl;
				}
			}

		} else {
			GENERATE_ERROR_MESSAGE << "could not parse area. Expected two minmax arrays, got: " << arrayMatches.size() << endl;
		}

		areasMinMax.push_back(minmax);
	}

	return areasMinMax;
}

vector<OrientedBox> parseAreaMatrices(string strArea) {
	vector<OrientedBox> areas;

	auto matches = getRegexMatches(strArea, "matrix\\([^\\)]*\\)");

	for (string match : matches) {
		cout << match << endl;

		auto arrayMatches = getRegexMatches(match, "[-+\\d][^,)]*");

		if (arrayMatches.size() == 16) {

			double values[16];
			for(int i = 0; i < 16; i++){
				double value = stod(arrayMatches[i]);
				values[i] = value;
			}

			auto transform = glm::make_mat4(values);

			OrientedBox box(transform);

			areas.push_back(box);

		}else{
			GENERATE_ERROR_MESSAGE << "expected 16 matrix component values, got: " << arrayMatches.size() << endl;
		}

	}

	return areas;
}

Area parseArea(string strArea) {

	Area area;

	area.minmaxs = parseAreaMinMax(strArea);
	area.orientedBoxes = parseAreaMatrices(strArea);

	return area;
}

bool intersects(Node* node, AABB region) {
	// box test from three.js, src/math/Box3.js

	AABB a = node->aabb;
	AABB b = region;

	if (b.max.x < a.min.x || b.min.x > a.max.x ||
		b.max.y < a.min.y || b.min.y > a.max.y ||
		b.max.z < a.min.z || b.min.z > a.max.z) {

		return false;

	}

	return true;
}

bool intersects(dvec3 point, AABB region) {

	if (point.x < region.min.x || point.x > region.max.x) {
		return false;
	}

	if (point.y < region.min.y || point.y > region.max.y) {
		return false;
	}

	if (point.z < region.min.z || point.z > region.max.z) {
		return false;
	}

	return true;
}

bool intersects(Node* node, Area& area) {
	auto a = node->aabb;
	
	for (auto& b : area.minmaxs) {
		if (b.max.x < a.min.x || b.min.x > a.max.x ||
			b.max.y < a.min.y || b.min.y > a.max.y ||
			b.max.z < a.min.z || b.min.z > a.max.z) {

			continue;
		} else {
			return true;
		}
	}

	for (auto& box : area.orientedBoxes) {
		
		if (box.intersects(a)) {
			return true;
		}

	}
	
	return false;
}

bool intersects(dvec3 point, Area& area) {

	for (auto minmax : area.minmaxs) {
		if (point.x < minmax.min.x || point.x > minmax.max.x) {
			continue;
		}else if (point.y < minmax.min.y || point.y > minmax.max.y) {
			continue;
		} else if (point.z < minmax.min.z || point.z > minmax.max.z) {
			continue;
		}

		return true;
	}

	for (auto orientedBoxes : area.orientedBoxes) {
		if (orientedBoxes.inside(point)) {
			return true;
		}
	}

	return false;
}

int main(int argc, char** argv) {

	auto tStart = now();

	Arguments args(argc, argv);

	args.addArgument("area", "clip area");

	string strArea = args.get("area").as<string>();
	
	Area area = parseArea(strArea);



	string path = "D:/temp/test/eclepens.las";
	string metadataPath = path + "/metadata.json";
	string octreePath = path + "/octree.bin";

	string strMetadata = readTextFile(metadataPath);
	json jsMetadata = json::parse(strMetadata);

	auto hierarchy = loadHierarchy(path, jsMetadata);
	
	vector<Node*> clippedNodes;
	for (auto node : hierarchy.nodes) {

		if (intersects(node, area)) {
			clippedNodes.push_back(node);
		}

	}

	cout << "#nodes: " << hierarchy.nodes.size() << endl;
	cout << "#clipped nodes: " << clippedNodes.size() << endl;

	auto attributes = parseAttributes(jsMetadata);

	dvec3 scale;
	scale.x = jsMetadata["scale"][0];
	scale.y = jsMetadata["scale"][1];
	scale.z = jsMetadata["scale"][2];

	dvec3 offset;
	offset.x = jsMetadata["offset"][0];
	offset.y = jsMetadata["offset"][1];
	offset.z = jsMetadata["offset"][2];

	vector<AcceptedItem> acceptedItems;
	mutex mtx_accept;

	atomic_int64_t checked = 0;
	atomic_int64_t accepted = 0;
	

	auto parallel = std::execution::par_unseq;
	for_each(parallel, clippedNodes.begin(), clippedNodes.end(), [octreePath, attributes, scale, offset, &area, &acceptedItems, &mtx_accept, &checked, &accepted](Node* node) {
		auto data = readBinaryFile(octreePath, node->byteOffset, node->byteSize);

		vector<int64_t> acceptedIndices;


		for (int64_t i = 0; i < node->numPoints; i++) {
			int64_t byteOffset = i * attributes.bytes;

			int32_t ix, iy, iz;
			memcpy(&ix, data.data() + byteOffset + 0, 4);
			memcpy(&iy, data.data() + byteOffset + 4, 4);
			memcpy(&iz, data.data() + byteOffset + 8, 4);

			double x = double(ix) * scale.x + offset.x;
			double y = double(iy) * scale.y + offset.y;
			double z = double(iz) * scale.z + offset.z;

			dvec3 point = { x, y, z };

			if (intersects(point, area)) {
				acceptedIndices.push_back(i);
			}
		}

		checked += node->numPoints;
		accepted += acceptedIndices.size();

		auto buffer = make_shared<Buffer>(acceptedIndices.size() * attributes.bytes);

		for (auto index : acceptedIndices) {
			buffer->write(data.data() + index * attributes.bytes, attributes.bytes);
		}

		{
			lock_guard<mutex> lock(mtx_accept);
			AcceptedItem item;
			item.node = node;
			item.data = buffer;

			acceptedItems.push_back(item);
		}
	});

	write("D:/temp/test.laz", acceptedItems, jsMetadata, attributes);

	printElapsedTime("duration", tStart);

	cout << acceptedItems.size() << endl;
	cout << "#checked: " << checked << endl;
	cout << "#accepted: " << accepted << endl;
	cout << "ratio: " << 100 * (double(accepted) / double(checked)) << "%" << endl;



	return 0;
}