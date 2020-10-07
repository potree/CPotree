
#pragma once

#include <iostream>
#include <algorithm>
#include <functional>
#include <execution>
#include <atomic>
#include <mutex>
#include <regex>
#include<memory>

#include "json/json.hpp"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/constants.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include "unsuck/unsuck.hpp"
#include "pmath.h"
#include "PotreeLoader.h"
#include "Attributes.h"
#include "Node.h"

using glm::dvec2;
using glm::dvec3;
using glm::dvec4;
using glm::dmat4;

using std::shared_ptr;
using std::cout; 
using json = nlohmann::json;

using std::atomic_int64_t;
using std::mutex;
using std::lock_guard;
using std::regex;
using std::function;


struct AreaMinMax {
	dvec3 min = { -Infinity, -Infinity, -Infinity };
	dvec3 max = { Infinity, Infinity, Infinity };
};

struct Profile {
	vector<dvec3> points;
	double width = 0.0;
};

struct Area {
	vector<AreaMinMax> minmaxs;
	vector<OrientedBox> orientedBoxes;
	vector<Profile> profiles;
};


vector<AreaMinMax> parseAreaMinMax(string strArea) {
	vector<AreaMinMax> areasMinMax;

	auto matches = getRegexMatches(strArea, "minmax\\([^\\)]*\\)");
	for (string match : matches) {

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

		auto arrayMatches = getRegexMatches(match, "[-+\\d][^,)]*");

		if (arrayMatches.size() == 16) {

			double values[16];
			for (int i = 0; i < 16; i++) {
				double value = stod(arrayMatches[i]);
				values[i] = value;
			}

			auto transform = glm::make_mat4(values);

			OrientedBox box(transform);

			areas.push_back(box);

		} else {
			GENERATE_ERROR_MESSAGE << "expected 16 matrix component values, got: " << arrayMatches.size() << endl;
		}

	}

	return areas;
}

vector<Profile> parseAreaProfile(string strArea) {
	vector<Profile> profiles;

	auto matches = getRegexMatches(strArea, "profile\\(([^)]*)\\)");
	for (string match : matches) {

		Profile profile;

		cout << "'" << match << "'" << endl;

	}

	exit(123);

	return profiles;
}

Area parseArea(string strArea) {

	Area area;

	area.minmaxs = parseAreaMinMax(strArea);
	area.orientedBoxes = parseAreaMatrices(strArea);
	//area.profiles = parseAreaProfile(strArea);

	return area;
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

	for (auto& orientedBoxes : area.orientedBoxes) {
		if (orientedBoxes.inside(point)) {
			return true;
		}
	}

	return false;
}

int64_t getNumCandidates(string path, Area area) {
	string metadataPath = path + "/metadata.json";
	string octreePath = path + "/octree.bin";

	string strMetadata = readTextFile(metadataPath);
	json jsMetadata = json::parse(strMetadata);

	auto hierarchy = loadHierarchy(path, jsMetadata);

	int64_t numCandidates = 0;

	vector<Node*> clippedNodes;
	for (auto node : hierarchy.nodes) {

		if (intersects(node, area)) {
			numCandidates += node->numPoints;
		}
	}

	return numCandidates;
}


void filterPointcloud(string path, Area area, int minLevel, int maxLevel, function<void(Attributes&, Node*, shared_ptr<Buffer>, int64_t, int64_t)> callback) {

	double tStart = now();

	string metadataPath = path + "/metadata.json";
	string octreePath = path + "/octree.bin";

	string strMetadata = readTextFile(metadataPath);
	json jsMetadata = json::parse(strMetadata);

	auto hierarchy = loadHierarchy(path, jsMetadata);

	vector<Node*> clippedNodes;
	for (auto node : hierarchy.nodes) {

		bool inArea = intersects(node, area);
		bool inLevelRange = node->level() >= minLevel && node->level() <= maxLevel;

		if (inArea && inLevelRange) {
			clippedNodes.push_back(node);
		}

	}

	auto attributes = parseAttributes(jsMetadata);

	dvec3 scale;
	scale.x = jsMetadata["scale"][0];
	scale.y = jsMetadata["scale"][1];
	scale.z = jsMetadata["scale"][2];

	dvec3 offset;
	offset.x = jsMetadata["offset"][0];
	offset.y = jsMetadata["offset"][1];
	offset.z = jsMetadata["offset"][2];

	mutex mtx_accept;

	atomic_int64_t checked = 0;
	atomic_int64_t accepted = 0;

	auto parallel = std::execution::par_unseq;
	for_each(parallel, clippedNodes.begin(), clippedNodes.end(), [octreePath, &attributes, scale, offset, &area, &mtx_accept, &checked, &accepted, &callback](Node* node) {
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

			int64_t batch_accepted = acceptedIndices.size();
			int64_t batch_rejected = node->numPoints - batch_accepted;

			callback(attributes, node, buffer, batch_accepted, batch_rejected);
		}
	});
}




//bool intersects(Node* node, AABB region) {
//	// box test from three.js, src/math/Box3.js
//
//	AABB a = node->aabb;
//	AABB b = region;
//
//	if (b.max.x < a.min.x || b.min.x > a.max.x ||
//		b.max.y < a.min.y || b.min.y > a.max.y ||
//		b.max.z < a.min.z || b.min.z > a.max.z) {
//
//		return false;
//
//	}
//
//	return true;
//}
//
//bool intersects(dvec3 point, AABB region) {
//
//	if (point.x < region.min.x || point.x > region.max.x) {
//		return false;
//	}
//
//	if (point.y < region.min.y || point.y > region.max.y) {
//		return false;
//	}
//
//	if (point.z < region.min.z || point.z > region.max.z) {
//		return false;
//	}
//
//	return true;
//}