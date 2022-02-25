
#include <string>
#include <functional>
#include <algorithm>
#include <execution>
#include <atomic>
#include <mutex>
#include <regex>
#include <memory>
#include <set>

#include "json/json.hpp"

#include "pmath.h"
#include "unsuck/unsuck.hpp"
#include "arguments/Arguments.hpp"
#include "CPotree.h"

#include "filter.h"

#include "PotreeLoader.h"
#include "LasWriter.h"
#include "CsvWriter.h"
#include "JsonWriter.h"
#include "PotreeWriter.h"
#include "Attributes.h"

using std::set;
using std::string; 
using std::function;
using std::shared_ptr;

using json = nlohmann::json;

struct AcceptedItem {
	Node* node;
	shared_ptr<Buffer> data;
};

Attributes computeAttributes(Arguments& args, vector<string> sources) {

	vector<Attribute> list;
	unordered_map<string, Attribute> map;

	for (string path : sources) {
		string metadataPath = path + "/metadata.json";
		string txtMetadata = readTextFile(metadataPath);
		json jsMetadata = json::parse(txtMetadata);

		auto jsAttributes = jsMetadata["attributes"];
		for (auto jsAttribute : jsAttributes) {

			Attribute attribute;

			attribute.name = jsAttribute["name"];
			attribute.size = jsAttribute["size"];
			attribute.numElements = jsAttribute["numElements"];
			attribute.elementSize = jsAttribute["elementSize"];
			attribute.type = typenameToType(jsAttribute["type"]);
			
			attribute.min.x = jsAttribute["min"][0];
			attribute.max.x = jsAttribute["max"][0];
			
			if (jsAttribute["min"].size() > 1) {
				attribute.min.y = jsAttribute["min"][1];
				attribute.max.y = jsAttribute["max"][1];
			}

			if (jsAttribute["min"].size() > 2) {
				attribute.min.z = jsAttribute["min"][2];
				attribute.max.z = jsAttribute["max"][2];
			}

			bool alreadyAdded = map.find(attribute.name) != map.end();
			if (!alreadyAdded) {
				list.push_back(attribute);
				map[attribute.name] = attribute;
			}

		}

	}

	vector<Attribute> chosen;

	if (args.has("output-attributes")) {

		vector<string> attributeNames = args.get("output-attributes").as<vector<string>>();

		if (attributeNames[0] != "position") {
			attributeNames.insert(attributeNames.begin(), "position");
		}

		for (string attributeName : attributeNames) {
			if (map.find(attributeName) != map.end()) {
				chosen.push_back(map[attributeName]);
			}
//    else {
//	  	cout << "WARNING: could not find a handler for attribute '" << attributeName << "'. The attribute will be ignored.";
//    }
		}

	} else {
		chosen = list;
	}

	if (map.find("position_projected_profile") == map.end()) {
		Attribute position_projected_profile("position_projected_profile", 8, 2, 4, AttributeType::INT32);
		chosen.push_back(position_projected_profile);
	}

	Attributes attributes(chosen);

	return attributes;
}

vector<string> curateSources(vector<string> sources) {
	vector<string> curated;

	bool hasError = false;
	for (string path : sources) {

		bool isMetadataFile = fs::path(path).filename() == "metadata.json";
		bool isDirectory = fs::is_directory(path);
		bool hasMetadataFile = fs::is_regular_file(path + "/metadata.json");

		if (isMetadataFile) {
			string metadataFolder = fs::path(path).parent_path().string();
			curated.push_back(metadataFolder);
		} else if (isDirectory && hasMetadataFile) {
			curated.push_back(path);
		} else {
			cout << "ERROR: not a valid potree file path '" << path << "'" << endl;
			hasError = true;
		}

	}

	if (hasError) {
		exit(123);
	}

	return curated;
}


int main(int argc, char** argv) {

	auto tStart = now();

	Arguments args(argc, argv);

	args.addArgument("source,i,", "input files (path to metadata.json)");
	args.addArgument("output,o", "output file, depending on target format. When not set, the result will be written in the stdout.");
	args.addArgument("area", "clip area, specified as unit cube matrix \"{tx,rz,ry,0,-rz,ty,rx,0,-ry,-rx,tz,0,dx,dy,dz,1}\"");
	args.addArgument("output-format", "LAS, LAZ, CSV, JSON, POTREE (default");
	args.addArgument("min-level", "the result will contain points starting from this level");
	args.addArgument("max-level", "the result will contain points up to this level");
	args.addArgument("output-attributes", "position, rgb, intensity, classification, ... Default: same as input point cloud");
	args.addArgument("get-candidates", "return number of candidate points");

	if (!args.has("area")) {
		GENERATE_ERROR_MESSAGE << "missing argument: --area \"{tx,rz,ry,0,-rz,ty,rx,0,-ry,-rx,tz,0,dx,dy,dz,1}\"" << endl;
		exit(123);
	}


	string strArea = args.get("area").as<string>();
	vector<string> sources = args.get("source").as<vector<string>>();
	string targetpath = args.get("output").as<string>();
	if (targetpath == "") targetpath = "stdout";
	string format = args.get("output-format").as<string>();
	int minLevel = args.get("min-level").as<int>(0);
	int maxLevel = args.get("max-level").as<int>(10'000);

	strArea = replaceAll(strArea, " ", "");
	strArea = replaceAll(strArea, "},{", "),(");
	strArea = replaceAll(strArea, "{", "matrix(");
	strArea = replaceAll(strArea, "}", ")");
	Area area = parseArea(strArea);

	sources = curateSources(sources);
	auto stats = computeStats(sources);

	Attributes outputAttributes = computeAttributes(args, sources);
	outputAttributes.posScale = {0.001, 0.001, 0.001};

	auto min = stats.aabb.min;
	auto max = stats.aabb.max;

	if (args.has("get-candidates")) {

		int64_t numCandidates = 0;
		for (string path : sources) {
			numCandidates += getNumCandidates(path, area, minLevel, maxLevel);
		};

		cout << formatNumber(numCandidates) << endl;
	} else {

		auto [scale, offset] = computeScaleOffset(stats.aabb, stats.minScale);

		shared_ptr<Writer> writer;

		if (format == "LAS" || iEndsWith(targetpath, "las")) {
			writer = make_shared<LasWriter>(targetpath, scale, offset, outputAttributes, false);
		} else if (format == "LAZ" || iEndsWith(targetpath, "laz")) {
			writer = make_shared<LasWriter>(targetpath, scale, offset, outputAttributes, true);
		} else if (format == "JSON" || iEndsWith(targetpath, "json")) {
			writer = make_shared<JsonWriter>(targetpath, outputAttributes, false);
		} else if (format == "CSV" || iEndsWith(targetpath, "csv")) {
			writer = make_shared<CsvWriter>(targetpath, outputAttributes, ",", ",");
		} else if (format == "POTREE" || iEndsWith(targetpath, "potree")) {
			writer = make_shared<PotreeWriter>(targetpath, scale, offset, outputAttributes);
		} else {
			cout << "ERROR: unkown output format, extension not known: " << targetpath << endl;
		}

		int64_t totalAccepted = 0;
		int64_t totalRejected = 0;
		for (string path : sources) {

			filterPointcloud(path, area, minLevel, maxLevel, [&writer, tStart, &totalAccepted, &totalRejected](Node* node, shared_ptr<Points> points, int64_t numAccepted, int64_t numRejected){

				totalAccepted += numAccepted;
				totalRejected += numRejected;

				writer->write(node, points, numAccepted, numRejected);

				//{ // DEBUG MESSAGE
				//	stringstream ss;
				//	ss << std::this_thread::get_id() << ": loadPoints() end" << endl;
				//	cout << ss.str();
				//}

			});

		};

		//cout << "#accepted: " << totalAccepted << ", #rejected: " << totalRejected << endl;

		writer->close();
	}


	//printElapsedTime("duration", tStart);


	return 0;
}