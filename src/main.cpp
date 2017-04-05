
#include <io.h>
#include <fcntl.h>

#include "StandardIncludes.h"

#include "PotreeReader.h"
#include "Point.h"

#include "pmath.h"
#include "stuff.h"

//struct PointsInBox{
//	vector<Point> points;
//	int pointsProcessed = 0;
//	int nodesProcessed = 0;
//};

//struct PointsInProfile{
//	vector<Point> points;
//	int pointsProcessed = 0;
//	int nodesProcessed = 0;
//};

struct FilterResult{
	dmat4 box;
	vector<Point> points;
	int pointsProcessed = 0;
	int nodesProcessed = 0;
	int durationMS = 0;
};

///
/// The box matrix maps a unit cube to the desired oriented cube.
/// The unit cube is assumed to have a size of 1/1/1 and it is 
/// centered around the origin, i.e. coordinates are [-0.5, 0.5]
///
/// algorithm: http://www.euclideanspace.com/maths/geometry/elements/intersection/twod/index.htm
/// 
FilterResult getPointsInBox(PotreeReader *reader, dmat4 box, int minLevel, int maxLevel){

	//cout << "== getPointsInBox() ==" << endl;

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	
	OBB obb(box);
	
	vector<PRNode*> intersectingNodes;
	stack<PRNode*> workload({reader->root});
	
	// nodes that intersect with box
	while(!workload.empty()){
		auto node = workload.top();
		workload.pop();
	
		intersectingNodes.push_back(node);
	
		for(auto child : node->children()){
			if(child != nullptr && obb.intersects(child->boundingBox) && child->level <= maxLevel){
				workload.push(child);
			}
		}
	}

	vector<Point> accepted;

	int pointsProcessed = 0;

	for(auto node : intersectingNodes){
		if(node->level < minLevel){
			continue;
		}

		for(auto &point : node->points()){
			pointsProcessed++;

			if(obb.inside(point.position)){
				accepted.push_back(point);
			}
		}
	}

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

	//cout << "intersecting nodes: " << intersectingNodes.size() << endl;
	//cout << "points processed: " << pointsProcessed << endl;
	//cout << "points accepted: " << accepted.size() << endl;
	//cout << "duration: " << milliseconds << " milliseconds" << endl;

	FilterResult result;
	result.box = box;
	result.points = accepted;
	result.pointsProcessed = pointsProcessed;
	result.nodesProcessed = intersectingNodes.size();

	return result;

}

vector<FilterResult> getPointsInProfile(PotreeReader *reader, vector<dvec2> polyline, double width, int minLevel, int maxLevel){

	auto bb = reader->metadata.boundingBox;

	struct Segment{
		dvec2 start;
		dvec2 end;
		dmat4 box;
	};
	
	vector<Segment> segments;
	for(int i = 0; i < polyline.size() - 1; i++){
		dvec3 start = {polyline[i].x, polyline[i].y, bb.center().z};
		dvec3 end = {polyline[i + 1].x, polyline[i + 1].y, bb.center().z};
		dvec3 delta = end - start;
		double length = glm::length(delta);
		double angle = glm::atan(delta.y, delta.x);

		dvec3 size = {length, width, bb.size().z};

		dmat4 box = glm::translate(dmat4(), start)
			* glm::rotate(dmat4(), angle, {0.0, 0.0, 1.0}) 
			* glm::scale(dmat4(), size) 
			* glm::translate(dmat4(), {0.5, 0.0, 0.0});
		
		Segment segment = {start, end, box};
		segments.push_back(segment);
	}

	vector<FilterResult> results;

	for(Segment &segment : segments){
		auto box = segment.box;
		FilterResult result;

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		auto pointsInBox = getPointsInBox(reader, box, minLevel, maxLevel);

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

		result.box = pointsInBox.box;
		result.points.insert(result.points.end(), pointsInBox.points.begin(), pointsInBox.points.end());
		result.pointsProcessed += pointsInBox.pointsProcessed;
		result.nodesProcessed += pointsInBox.nodesProcessed;
		result.durationMS = milliseconds;

		results.push_back(result);

		//cout << "saving result to: " << outfile << endl;
		//
		//{
		//	std::ofstream fout(outfile);
		//	fout << std::fixed;
		//	for(auto &point : points){
		//		fout << point.position.x 
		//			<< ", " << point.position.y
		//			<< ", " << point.position.z
		//			<< ", " << unsigned int(point.color.r)
		//			<< ", " << unsigned int(point.color.g)
		//			<< ", " << unsigned int(point.color.b)
		//			<< endl;
		//	}
		//	fout.close();
		//}
	}

	return results;
}

void writeToDisk(){

}

void writeToSTDOUT(PotreeReader *reader, vector<FilterResult> results){

	double scale = 0.001;

	dvec3 min = {infinity, infinity, infinity};
	dvec3 max = {-infinity, -infinity, -infinity};
	int pointsAccepted = 0;
	int pointsProcessed = 0;
	int nodesProcessed = 0;
	int durationMS = 0;

	for(auto &result : results){

		pointsAccepted += result.points.size();
		pointsProcessed += result.pointsProcessed;
		nodesProcessed += result.nodesProcessed;
		durationMS += result.durationMS;

		for(auto &p : result.points){
			min.x = std::min(min.x, p.position.x);
			min.y = std::min(min.y, p.position.y);
			min.z = std::min(min.z, p.position.z);

			max.x = std::max(max.x, p.position.x);
			max.y = std::max(max.y, p.position.y);
			max.z = std::max(max.z, p.position.z);
		}
	}

	auto attributes = reader->metadata.pointAttributes.attributes;
	attributes.push_back(PointAttribute::POSITION_PROJECTED_PROFILE);
	auto pointAttributes = PointAttributes(attributes);

	{ // HEADER
		string header;
		header += "{\n";
		header += "\t\"points\": " + to_string(pointsAccepted) + ",\n";
		header += "\t\"pointsProcessed\": " + to_string(pointsProcessed) + ",\n";
		header += "\t\"nodesProcessed\": " + to_string(nodesProcessed) + ",\n";
		header += "\t\"durationMS\": " + to_string(durationMS) + ",\n";

		header += "\t\"boundingBox\": {\n";
		header += "\t\t\"lx\": " + to_string(min.x) + ",\n";
		header += "\t\t\"ly\": " + to_string(min.y) + ",\n";
		header += "\t\t\"lz\": " + to_string(min.z) + ",\n";
		header += "\t\t\"ux\": " + to_string(max.x) + ",\n";
		header += "\t\t\"uy\": " + to_string(max.y) + ",\n";
		header += "\t\t\"uz\": " + to_string(max.z) + "\n";
		header += "\t},\n";

		header += "\t\"pointAttributes\": [\n";

		for(int i = 0; i < attributes.size(); i++){
			auto attribute = attributes[i];
			header += "\t\t\"" + attribute.name + "\"";

			if(i < attributes.size() - 1){
				header += ",\n";
			}else{
				header += "\n";
			}
			
		}

		//header += "\t\t\"POSITION_CARTESIAN\",\n";
		//header += "\t\t\"RGB\"\n";

		header += "\t],\n";

		header += "\t\"bytesPerPoint\": " + to_string(pointAttributes.byteSize) + ",\n";
		header += "\t\"scale\": " + to_string(scale) + "\n";

		header += "}\n";

		int headerSize = header.size();
		cout.write(reinterpret_cast<const char*>(&headerSize), 4);
		cout.write(header.c_str(), header.size());
	}
	
	double mileage = 0.0;
	for(auto &result : results){

		dmat4 box = result.box;
		OBB obb(box);
		dvec3 localMin = dvec3(box * dvec4(-0.5, -0.5, -0.5, 1.0));
		dvec3 lvx = dvec3(box * dvec4(+0.5, -0.5, -0.5, 1.0)) - localMin;
		//dvec3 lvy = dvec3(box * dvec4(-0.5, +0.5, -0.5, 1.0)) - localMin;
		//dvec3 lvz = dvec3(box * dvec4(-0.5, -0.5, +0.5, 1.0)) - localMin;

		for(Point &p : result.points){
			for(auto attribute : pointAttributes.attributes){

				if(attribute == PointAttribute::POSITION_CARTESIAN){
					unsigned int ux = (p.position.x - min.x) / scale;
					unsigned int uy = (p.position.y - min.y) / scale;
					unsigned int uz = (p.position.z - min.z) / scale;
	
					cout.write(reinterpret_cast<const char *>(&ux), 4);
					cout.write(reinterpret_cast<const char *>(&uy), 4);
					cout.write(reinterpret_cast<const char *>(&uz), 4);
				}else if(attribute == PointAttribute::POSITION_PROJECTED_PROFILE){
					
					
					dvec3 lp = p.position - localMin;

					double dx = glm::dot(lp, obb.axes[0]) + mileage;
					double dy = glm::dot(lp, obb.axes[1]);
					double dz = glm::dot(lp, obb.axes[2]);
					
					unsigned int ux = dx / scale;
					//unsigned int uy = dy / scale;
					unsigned int uz = dz / scale;

					cout.write(reinterpret_cast<const char *>(&ux), 4);
					//cout.write(reinterpret_cast<const char *>(&uy), 4);
					cout.write(reinterpret_cast<const char *>(&uz), 4);
				}else if(attribute == PointAttribute::COLOR_PACKED){
					cout.write(reinterpret_cast<const char*>(&p.color), 3);
					char value = 0;
					cout.write(reinterpret_cast<const char*>(&value), 1);
				}else if(attribute == PointAttribute::INTENSITY){
					cout.write(reinterpret_cast<const char*>(&p.intensity), sizeof(p.intensity));
				}else if(attribute == PointAttribute::CLASSIFICATION){
					cout.write(reinterpret_cast<const char*>(&p.classification), sizeof(p.classification));
				}else{
					static long long int filler = 0;
					cout.write(reinterpret_cast<const char*>(&filler), attribute.byteSize);
				}

			}

		}

		mileage += glm::length(lvx);
	}
}


int main(int argc, char* argv[]){

	//cout.imbue(std::locale(""));
	//std::cout.rdbuf()->pubsetbuf( 0, 0 );
	_setmode( _fileno( stdout ),  _O_BINARY );

	string file = argv[1];
	string strPolyline = argv[2];
	string strWidth = argv[3];
	string strMinLevel = argv[4];
	string strMaxLevel = argv[5];

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

	double width = std::stod(strWidth);
	int minLevel = std::stoi(strMinLevel);
	int maxLevel = std::stoi(strMaxLevel);
	
	//cout << endl;
	//std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	PotreeReader *reader = new PotreeReader(file);

	auto results = getPointsInProfile(reader, polyline, width, minLevel, maxLevel);

	//std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	//auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

	//auto &points = pointsInProfile.points;
	//cout << "number of points: " << points.size() << endl;


	writeToSTDOUT(reader, results);






	////string file = "D:/dev/pointclouds/converted/nvidia/cloud.js";
	//string file = "D:/dev/pointclouds/converted/CA13/cloud.js";
	//PotreeReader *reader = new PotreeReader(file);
	//
	//// CA13
	//vector<dvec2> polyline = {
	//	{693'550.968, 3'915'914.169},
	//	{693'890.618, 3'916'387.819},
	//	{694'584.820, 3'916'458.180},
	//	{694'786.239, 3'916'307.199}};
	//getPointsInProfile(file, polyline, 14.0, 0, 6);



	return 0;
}