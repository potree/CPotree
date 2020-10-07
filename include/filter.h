
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
#include "brotli/decode.h"


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

struct Points {
	Attributes attributes;
	unordered_map<string, shared_ptr<Buffer>> attributesData;

	int64_t numPoints;
};

uint32_t dealign24b(uint32_t mortoncode) {
	// see https://stackoverflow.com/questions/45694690/how-i-can-remove-all-odds-bits-in-c

	// input alignment of desired bits
	// ..a..b..c..d..e..f..g..h..i..j..k..l..m..n..o..p
	uint32_t x = mortoncode;

	//          ..a..b..c..d..e..f..g..h..i..j..k..l..m..n..o..p                     ..a..b..c..d..e..f..g..h..i..j..k..l..m..n..o..p 
	//          ..a.....c.....e.....g.....i.....k.....m.....o...                     .....b.....d.....f.....h.....j.....l.....n.....p 
	//          ....a.....c.....e.....g.....i.....k.....m.....o.                     .....b.....d.....f.....h.....j.....l.....n.....p 
	x = ((x & 0b001000001000001000001000) >> 2) | ((x & 0b000001000001000001000001) >> 0);
	//          ....ab....cd....ef....gh....ij....kl....mn....op                     ....ab....cd....ef....gh....ij....kl....mn....op
	//          ....ab..........ef..........ij..........mn......                     ..........cd..........gh..........kl..........op
	//          ........ab..........ef..........ij..........mn..                     ..........cd..........gh..........kl..........op
	x = ((x & 0b000011000000000011000000) >> 4) | ((x & 0b000000000011000000000011) >> 0);
	//          ........abcd........efgh........ijkl........mnop                     ........abcd........efgh........ijkl........mnop
	//          ........abcd....................ijkl............                     ....................efgh....................mnop
	//          ................abcd....................ijkl....                     ....................efgh....................mnop
	x = ((x & 0b000000001111000000000000) >> 8) | ((x & 0b000000000000000000001111) >> 0);
	//          ................abcdefgh................ijklmnop                     ................abcdefgh................ijklmnop
	//          ................abcdefgh........................                     ........................................ijklmnop
	//          ................................abcdefgh........                     ........................................ijklmnop
	x = ((x & 0b000000000000000000000000) >> 16) | ((x & 0b000000000000000011111111) >> 0);

	// sucessfully realigned! 
	//................................abcdefghijklmnop

	return x;
}

shared_ptr<Points> readNode(bool isBrotliEncoded, Attributes& attributes, string octreePath, Node* node) {

	auto points = make_shared<Points>();

	points->attributes = attributes;
	points->numPoints = node->numPoints;

	auto data = readBinaryFile(octreePath, node->byteOffset, node->byteSize);

	if (isBrotliEncoded) {

		size_t encoded_size = node->byteSize;
		const uint8_t* encoded_buffer = data.data();

		thread_local int64_t decoded_buffer_size = 1024 * 1024;
		thread_local uint8_t* decoded_buffer = reinterpret_cast<uint8_t*>(malloc(decoded_buffer_size));

		size_t decoded_size = decoded_buffer_size;

		bool success = false;
		int numAttempts = 0;
		while (!success && numAttempts < 10) {
			numAttempts++;

			auto status = BrotliDecoderDecompress(encoded_size, encoded_buffer, &decoded_size, decoded_buffer);

			if (status == BROTLI_DECODER_RESULT_ERROR) {
				decoded_buffer_size = 2 * decoded_buffer_size;
				decoded_size = decoded_buffer_size;
				free(decoded_buffer);
				decoded_buffer = reinterpret_cast<uint8_t*>(malloc(decoded_buffer_size));
			} else if(status == BROTLI_DECODER_RESULT_SUCCESS){
				success = true;
			}
		}

		if (!success) {
			GENERATE_ERROR_MESSAGE << "ERROR: failed to decode compressed node after " << numAttempts << " attempts" << endl;
			exit(123);
		}

		int64_t offset = 0;
		for (auto &attribute : attributes.list) {

			int64_t attributeDataSize = attribute.size * node->numPoints;
			string name = attribute.name;

			auto buffer = make_shared<Buffer>(attributeDataSize);

			if (attribute.name == "position") {

				// special case because position is stored as 96 bit morton code

				for (int64_t i = 0; i < points->numPoints; i++) {

					uint32_t mc_0, mc_1, mc_2, mc_3;
					memcpy(&mc_0, decoded_buffer + offset + 16 * i +  4, 4);
					memcpy(&mc_1, decoded_buffer + offset + 16 * i +  0, 4);
					memcpy(&mc_2, decoded_buffer + offset + 16 * i + 12, 4);
					memcpy(&mc_3, decoded_buffer + offset + 16 * i +  8, 4);

					int64_t X = dealign24b((mc_3 & 0x00FFFFFF) >> 0)
						| (dealign24b(((mc_3 >> 24) | (mc_2 << 8)) >> 0) << 8);

					int64_t Y = dealign24b((mc_3 & 0x00FFFFFF) >> 1)
						| (dealign24b(((mc_3 >> 24) | (mc_2 << 8)) >> 1) << 8);

					int64_t Z = dealign24b((mc_3 & 0x00FFFFFF) >> 2)
						| (dealign24b(((mc_3 >> 24) | (mc_2 << 8)) >> 2) << 8);

					if (mc_1 != 0 || mc_2 != 0) {
						X = X | (dealign24b((mc_1 & 0x00FFFFFF) >> 0) << 16)
							| (dealign24b(((mc_1 >> 24) | (mc_0 << 8)) >> 0) << 24);

						Y = Y | (dealign24b((mc_1 & 0x00FFFFFF) >> 1) << 16)
							| (dealign24b(((mc_1 >> 24) | (mc_0 << 8)) >> 1) << 24);

						Z = Z | (dealign24b((mc_1 & 0x00FFFFFF) >> 2) << 16)
							| (dealign24b(((mc_1 >> 24) | (mc_0 << 8)) >> 2) << 24);
					}

					int32_t X32 = X;
					int32_t Y32 = Y;
					int32_t Z32 = Z;

					memcpy(buffer->data_u8 + 12 * i + 0, &X32, 4);
					memcpy(buffer->data_u8 + 12 * i + 4, &Y32, 4);
					memcpy(buffer->data_u8 + 12 * i + 8, &Z32, 4);

				}
				


			} else {
				
				memcpy(buffer->data, decoded_buffer + offset, attributeDataSize);

			}

			points->attributesData[name] = buffer;
			offset += attributeDataSize;
		}

	} else {


	}




	

	return points;
}


void filterPointcloud(string path, Area area, int minLevel, int maxLevel, function<void(Node*, shared_ptr<Points>, int64_t, int64_t)> callback) {

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
	for_each(parallel, clippedNodes.begin(), clippedNodes.end(), [&jsMetadata, octreePath, &attributes, scale, offset, &area, &mtx_accept, &checked, &accepted, &callback](Node* node) {

		bool isBrotliEncoded = jsMetadata["encoding"] == "BROTLI";
		auto points = readNode(isBrotliEncoded, attributes, octreePath, node);

		int64_t numAccepted = 0;
		int64_t numRejected = 0;

		vector<int64_t> acceptedIndices;

		auto aPosition = points->attributes.get("position");
		auto buf_position = points->attributesData["position"];
		for (int64_t i = 0; i < points->numPoints; i++) {
			int64_t byteOffset = i * 12;

			int32_t ix, iy, iz;
			memcpy(&ix, buf_position->data_u8 + byteOffset + 0, 4);
			memcpy(&iy, buf_position->data_u8 + byteOffset + 4, 4);
			memcpy(&iz, buf_position->data_u8 + byteOffset + 8, 4);

			double x = double(ix) * scale.x + offset.x;
			double y = double(iy) * scale.y + offset.y;
			double z = double(iz) * scale.z + offset.z;

			dvec3 point = { x, y, z };

			if (intersects(point, area)) {

				acceptedIndices.push_back(i);

				numAccepted++;
			} else {
				numRejected++;
			}
		}

		// pack accepted points to front, remove rejected, adjust (claimed) buffer size

		for (auto& attribute : points->attributes.list) {

			shared_ptr<Buffer> data = points->attributesData[attribute.name];
			int64_t targetOffset = 0;

			for (int64_t acceptedIndex : acceptedIndices) {
				int64_t sourceOffset = acceptedIndex * attribute.size;

				memcpy(data->data_u8 + targetOffset, data->data_u8 + sourceOffset, attribute.size);

				targetOffset += attribute.size;
			}

			data->size = numAccepted * attribute.size;
		}

		points->numPoints = numAccepted;

		
		{
			lock_guard<mutex> lock(mtx_accept);

			

			callback(node, points, numAccepted, numRejected);
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