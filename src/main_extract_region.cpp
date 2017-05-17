
#include "PotreeFilters.h"

int main(int argc, char* argv[]){

	//cout.imbue(std::locale(""));
	//std::cout.rdbuf()->pubsetbuf( 0, 0 );

	Arguments args(argc, argv);

	if(args.hasKey("stdout")){
		_setmode( _fileno( stdout ),  _O_BINARY );
	}
	
	string file = args.get("", 0);
	string strBoxes = args.get("box", 0);
	double width = args.getDouble("width", 0);
	int minLevel = args.getInt("min-level", 0);
	int maxLevel = args.getInt("max-level", 0);


	dmat4 box;
	{
		strBoxes = replaceAll(strBoxes, " ", "");

		vector<string> tokens = split(strBoxes, ',');

		if(tokens.size() != 16){
			cerr << "Invalid box matrix sequence. Expected 16 tokens, found: " << tokens.size() << endl;
			exit(1);
		}

		vector<double> values;
		for(auto token : tokens){
			values.push_back(std::stod(token));
		}
		box = glm::make_mat4(values.data());
	}
	
	PotreeReader *reader = new PotreeReader(file);

	if(args.hasKey("estimate")){
		auto results = estimatePointsInBox(reader, box, minLevel, maxLevel);

		auto pointAttributes = PointAttributes({PointAttribute::POSITION_CARTESIAN});

		string header = createHeader({results}, pointAttributes);
		cout << header;

	}else if(args.hasKey("check-threshold")){
		int threshold = args.getInt("check-threshold", 0, 0);

		bool passes = checkThreshold(reader, box, minLevel, maxLevel, threshold);

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
		auto results = getPointsInBox(reader, box, minLevel, maxLevel);

		save(reader, {results}, args);
	}

	

	return 0;
}