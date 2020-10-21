
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


	//vector<Attribute> list;

	//Attribute position("position", 12, 3, 4, AttributeType::INT32);
	//Attribute rgb("rgb", 6, 3, 2, AttributeType::UINT16);
	//Attribute intensity("intensity", 2, 1, 2, AttributeType::UINT16);
	//Attribute position_projected_profile("position_projected_profile", 8, 2, 4, AttributeType::INT32);

	//unordered_map<string, Attribute> mapping = {
	//	{"position", position},
	//	{"rgb", rgb},
	//	{"rgba", rgb},
	//	{"intensity", intensity},
	//};

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
		}



		//list.push_back(position);

		//vector<string> attributeNames = args.get("output-attributes").as<vector<string>>();

		//for (string attributeName : attributeNames) {

		//	if (mapping.find(attributeName) != mapping.end()) {
		//		list.push_back(mapping[attributeName]);
		//	} else {
		//		cout << "WARNING: could not find a handler for attribute '" << attributeName << "'. The attribute will be ignored.";
		//	}

		//}

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

Profile parseProfile(string strPolyline, double width) {

	Profile profile;

	{
		strPolyline = replaceAll(strPolyline, " ", "");
		strPolyline = replaceAll(strPolyline, "},{", "|");
		strPolyline = replaceAll(strPolyline, "{", "");
		strPolyline = replaceAll(strPolyline, "}", "");

		vector<string> polylineTokens = split(strPolyline, '|');
		for (string token : polylineTokens) {
			vector<string> vertexTokens = split(token, ',');

			double x = std::stod(vertexTokens[0]);
			double y = std::stod(vertexTokens[1]);

			//polyline.push_back({ x, y });
			profile.points.push_back({x, y, 0.0});
		}
	}

	profile.updateSegments();
	profile.width = width;

	return profile;
}

int main(int argc, char** argv) {

	auto tStart = now();

	Arguments args(argc, argv);

	args.addArgument("source,i,", "input files");
	args.addArgument("output,o", "output file or directory, depending on target format");
	args.addArgument("coordinates", "coordinates of the profile segments. in the form \"{x0,y0},{x1,y1},...\"");
	args.addArgument("width", "width of the profile");
	args.addArgument("output-format", "LAS, LAZ, POTREE");
	args.addArgument("min-level", "");
	args.addArgument("max-level", "");
	args.addArgument("output-attributes", "");
	args.addArgument("get-candidates", "return number of candidate points");

	if (!args.has("coordinates")) {
		GENERATE_ERROR_MESSAGE << "missing argument: --coordinates \"{x0,y0},{x1,y1},...\"" << endl;
		exit(123);
	}

	if (!args.has("width")) {
		GENERATE_ERROR_MESSAGE << "missing argument: --width <value>" << endl;
		exit(123);
	}

	string strCoordinates = args.get("coordinates").as<string>();
	vector<string> sources = args.get("source").as<vector<string>>();
	string targetpath = args.get("output").as<string>();
	double width = args.get("width").as<double>();
	int minLevel = args.get("min-level").as<int>(0);
	int maxLevel = args.get("max-level").as<int>(10'000);

	Profile profile = parseProfile(strCoordinates, width);
	Area area;
	area.profiles = { profile };

	sources = curateSources(sources);
	auto stats = computeStats(sources);

	Attributes outputAttributes = computeAttributes(args, sources);
	outputAttributes.posScale = {0.001, 0.001, 0.001};

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
		} else if (iEndsWith(targetpath, "potree")) {
			writer = make_shared<PotreeWriter>(targetpath, scale, offset, outputAttributes);
		} else if (targetpath == "stdout") {
			cout << "TODO: " << __FILE__ << ":" << __LINE__ << endl;
			//writer = make_shared<PotreeWriter>(targetpath, scale, offset, outputAttributes);
		} else {
			cout << "ERROR: unkown output format, extension not known: " << targetpath << endl;
		}

		int64_t totalAccepted = 0;
		int64_t totalRejected = 0;
		for (string path : sources) {

			// load points in nodes that intersect area, including points outside of that area
			loadPoints(path, area, minLevel, maxLevel, [&writer, &area, &profile](Node* node, shared_ptr<Points> points) {

				//stringstream ss;
				//ss << std::this_thread::get_id() << ": loadPoints() begin" << endl;
				//cout << ss.str();

				auto numAccepted = 0;
				auto numRejected = 0;

				// now filter out points that are outside the area
				// iterate through points and realign data so that accepted points are packed at the beginning

				auto& attributes = points->attributes;
				//int numAttributes = attributes.list.size();


				//auto rgb = points->attributeBuffersMap["rgb"];
				
				Attribute attribute_position_projected("position_projected_profile", 8, 2, 4, AttributeType::INT32);
				shared_ptr<Buffer> buffer_position_projected = make_shared<Buffer>(8 * points->numPoints);

				points->removeAttribute("position_projected_profile");
				points->addAttribute(attribute_position_projected, buffer_position_projected);

				for (int64_t i = 0; i < points->numPoints; i++) {
					dvec3 position = points->getPosition(i);

					bool isAccepted = false;

					double mileage = 0.0;
					for (auto& segment : profile.segments) {
						dvec3 projected = segment.proj * dvec4(position, 1.0);

						bool insideX = projected.x > 0.0 && projected.x < segment.length;
						bool insideDepth = projected.y >= -profile.width / 2.0 && projected.y <= profile.width / 2.0;
						bool inside = insideX && insideDepth;

						if (inside) {

							// write projected position to attribute

							int32_t X = (mileage + projected.x) / attributes.posScale.x;
							int32_t Z = position.z / attributes.posScale.z;

							buffer_position_projected->data_i32[2 * i + 0] = X;
							buffer_position_projected->data_i32[2 * i + 1] = Z;


							isAccepted = true;

							break;
						}

						mileage += segment.length;
					}

					//bool isInside = intersects(position, area);
					//bool niceColor = rgb->data_u16[3 * i + 1] > (100 << 8);

					if (isAccepted/* && niceColor*/) {

						for (int j = 0; j < attributes.list.size(); j++) {
							auto& attribute = attributes.list[j];
							auto& buffer = points->attributeBuffers[j];

							auto from = buffer->data_u8 + i * attribute.size;
							auto to = buffer->data_u8 + numAccepted * attribute.size;
							memcpy(to, from, attribute.size);
						}

						numAccepted++;
					} else {
						numRejected++;
					}

				}

				{// update points properties

					for (int i = 0; i < attributes.list.size(); i++) {
						auto& attribute = attributes.list[i];
						auto& buffer = points->attributeBuffers[i];

						buffer->size = attribute.size * numAccepted;
					}

					points->numPoints = numAccepted;
				}

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


	printElapsedTime("duration", tStart);


	return 0;
}