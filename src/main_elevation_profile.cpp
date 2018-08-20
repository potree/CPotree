
#include "PotreeFilters.h"

int main(int argc, char* argv[]){

	//cout.imbue(std::locale(""));
	//std::cout.rdbuf()->pubsetbuf( 0, 0 );

	Arguments args(argc, argv);
#ifdef _WIN_32
	if(args.hasKey("stdout")){
		_setmode( _fileno( stdout ),  _O_BINARY );
	}
#endif

	string file = args.get("", 0);
	string strPolyline = args.get("coordinates", 0);
	double width = args.getDouble("width", 0);
	int minLevel = args.getInt("min-level", 0);
	int maxLevel = args.getInt("max-level", 0);

	vector<dvec2> polyline;
	{
		strPolyline = replaceAll(strPolyline, "},{", "|");
		strPolyline = replaceAll(strPolyline, " ", "");
		strPolyline = replaceAll(strPolyline, "{", "");
		strPolyline = replaceAll(strPolyline, "}", "");

		vector<string> polylineTokens = split(strPolyline, '|');
		for(string token : polylineTokens){
			vector<string> vertexTokens = split(token, ',');

			double x = std::stod(vertexTokens[0]);
			double y = std::stod(vertexTokens[1]);

			polyline.push_back({x, y});
		}
	}

	PotreeReader *reader = new PotreeReader(file);
	if(args.hasKey("estimate")){
		auto results = estimatePointsInProfile(reader, polyline, width, minLevel, maxLevel);

		auto pointAttributes = PointAttributes({PointAttribute::POSITION_PROJECTED_PROFILE});

		string header = createHeader(results, pointAttributes);
		cout << header;
		//savePotree(reader, results, pointAttributes, &cout);
	}else{
		auto results = getPointsInProfile(reader, polyline, width, minLevel, maxLevel);
		save(reader, results, args);
	}

	return 0;
}
