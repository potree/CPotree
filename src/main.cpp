
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
#include "Attributes.h"

using std::string; 
using std::function;
using std::shared_ptr;

struct AcceptedItem {
	Node* node;
	shared_ptr<Buffer> data;
};


int main(int argc, char** argv) {

	auto tStart = now();

	Arguments args(argc, argv);

	args.addArgument("source,i,", "input files");
	args.addArgument("output,o", "output file or directory, depending on target format");
	args.addArgument("area", "clip area");
	args.addArgument("output-format", "LAS, LAZ, POTREE");
	args.addArgument("min-level", "");
	args.addArgument("max-level", "");
	args.addArgument("get-candidates", "return number of candidate points");

	string strArea = args.get("area").as<string>();
	vector<string> sources = args.get("source").as<vector<string>>();
	string targetpath = args.get("output").as<string>();

	Area area = parseArea(strArea);

	auto stats = computeStats(sources);

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

		LasWriter writer(targetpath, scale, offset);

		for(string path : sources){

			filterPointcloud(path, area, [&writer](Attributes& sourceAttributes, Node* node, shared_ptr<Buffer> data){
				writer.write(sourceAttributes, node, data);
			});

		};

		writer.close();
	}


	printElapsedTime("duration", tStart);


	return 0;
}