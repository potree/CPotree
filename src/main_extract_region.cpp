
#include "PotreeFilters.h"

int main(int argc, char* argv[]){
	//cout.imbue(std::locale(""));
	//std::cout.rdbuf()->pubsetbuf( 0, 0 );

	Arguments args(argc, argv);
#ifdef _WIN32
	if(args.hasKey("stdout")){
		_setmode( _fileno( stdout ),  _O_BINARY );
	}
#endif

	string file = args.get("", 0);
	string strBoxes = args.get("box", 0);
	string metadata = args.get("metadata", 0);
	int minLevel = args.getInt("min-level", 0);
	int maxLevel = args.getInt("max-level", 0);

	vector<dmat4> boxes;
	{
		strBoxes = replaceAll(strBoxes, " ", "");

		vector<string> tokens = split(strBoxes, ',');

		if((tokens.size() % 16) != 0){
			cerr << "Invalid box matrix sequence. Expected a multiple of 16 tokens, found: " << tokens.size() << endl;
			exit(1);
		}

		vector<double> values;
		for(auto token : tokens){
			values.push_back(std::stod(token));
		}

		for (size_t i = 0; i < tokens.size() / 16; i++) {
			int first = 16 * i;
			int last = 16 * (i + 1);
			vector<double> boxValues = vector<double>(values.begin() + first, values.begin() + last);
			dmat4 box = glm::make_mat4(boxValues.data());
			boxes.push_back(box);
		}

	}

	PotreeReader *reader = new PotreeReader(file);

	if(args.hasKey("check-threshold")){
		int threshold = args.getInt("check-threshold", 0, 0);

		//bool passes = checkThreshold(reader, box, minLevel, maxLevel, threshold);
		bool passes = checkThreshold(reader, boxes, minLevel, maxLevel, threshold);

		if(passes){
			cout << R"(
			{
				"request": "CHECK_THRESHOLD",
				"result": "BELOW_THRESHOLD"
			})";
		}else{
			cout << R"(
			{
				"request": "CHECK_THRESHOLD",
				"result": "THRESHOLD_EXCEEDED"
			})";
		}
	}else{
		//auto results = getPointsInBox(reader, box, minLevel, maxLevel);
		auto results = getPointsInBoxes(reader, boxes, minLevel, maxLevel);

		save(reader, {results}, args);
	}



	return 0;
}
