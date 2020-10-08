
#include <string>
#include <functional>
#include <algorithm>
#include <execution>
#include <atomic>
#include <mutex>
#include <regex>
#include <memory>

#include "pmath.h"
#include "unsuck/unsuck.hpp"
#include "arguments/Arguments.hpp"
#include "CPotree.h"

#include "filter.h"

#include "PotreeLoader.h"
#include "LasWriter.h"
#include "CsvWriter.h"
#include "PotreeWriter.h"
#include "Attributes.h"

using std::string; 
using std::function;
using std::shared_ptr;

struct AcceptedItem {
	Node* node;
	shared_ptr<Buffer> data;
};

Attributes computeAttributes(Arguments& args) {

	vector<Attribute> list;

	Attribute position("position", 12, 3, 4, AttributeType::INT32);
	Attribute rgb("rgb", 6, 3, 2, AttributeType::UINT16);
	Attribute intensity("intensity", 2, 1, 2, AttributeType::UINT16);

	unordered_map<string, Attribute> mapping = {
		{"position", position},
		{"rgb", rgb},
		{"rgba", rgb},
		{"intensity", intensity},
	};

	if (args.has("output-attributes")) {

		list.push_back(position);

		vector<string> attributeNames = args.get("output-attributes").as<vector<string>>();

		for (string attributeName : attributeNames) {

			if (mapping.find(attributeName) != mapping.end()) {
				list.push_back(mapping[attributeName]);
			} else {
				cout << "WARNING: could not find a handler for attribute '" << attributeName << "'. The attribute will be ignored.";
			}

		}

	} else {

		list.push_back(position);
		list.push_back(rgb);
		list.push_back(intensity);
	}

	Attributes attributes(list);

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
		} else if(isDirectory && hasMetadataFile) {
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

	printElapsedTime("00", tStart);

	Arguments args(argc, argv);

	args.addArgument("source,i,", "input files");
	args.addArgument("output,o", "output file or directory, depending on target format");
	args.addArgument("area", "clip area");
	args.addArgument("output-format", "LAS, LAZ, POTREE");
	args.addArgument("min-level", "");
	args.addArgument("max-level", "");
	args.addArgument("output-attributes", "");
	args.addArgument("get-candidates", "return number of candidate points");

	string strArea = args.get("area").as<string>();
	vector<string> sources = args.get("source").as<vector<string>>();
	string targetpath = args.get("output").as<string>();
	int minLevel = args.get("min-level").as<int>(0);
	int maxLevel = args.get("max-level").as<int>(10'000);

	Area area = parseArea(strArea);

	sources = curateSources(sources);
	auto stats = computeStats(sources);

	Attributes outputAttributes = computeAttributes(args);

	auto min = stats.aabb.min;
	auto max = stats.aabb.max;

	if (args.has("get-candidates")) {
		int64_t numCandidates = 0;
		for (string path : sources) {
			numCandidates += getNumCandidates(path, area);
		};

		cout << formatNumber(numCandidates) << endl;
	} else {

		auto [scale, offset] = computeScaleOffset(stats.aabb, stats.minScale);

		shared_ptr<Writer> writer;

		if (iEndsWith(targetpath, "las") || iEndsWith(targetpath, "laz")) {
			writer = make_shared<LasWriter>(targetpath, scale, offset, outputAttributes);
		} else if (iEndsWith(targetpath, "csv")) {
			writer = make_shared<CsvWriter>(targetpath, outputAttributes);
		}  else if (iEndsWith(targetpath, "potree")) {
			writer = make_shared<PotreeWriter>(targetpath, scale, offset, outputAttributes);
		} else {
			cout << "ERROR: unkown output format, extension not known: " << targetpath << endl;
		}


		int64_t totalAccepted = 0;
		int64_t totalRejected = 0;
		for(string path : sources){

			filterPointcloud(path, area, minLevel, maxLevel, [&writer, tStart, &totalAccepted, &totalRejected](Node* node, shared_ptr<Points> points, int64_t numAccepted, int64_t numRejected){

				totalAccepted += numAccepted;
				totalRejected += numRejected;

				writer->write(node, points, numAccepted, numRejected);
			});

		};

		cout << "#accepted: " << totalAccepted << ", #rejected: " << totalRejected << endl;

		writer->close();
	}


	printElapsedTime("duration", tStart);


	return 0;
}