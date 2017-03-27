
#include "StandardIncludes.h"

#include "PotreeReader.h"
#include "Point.h"

#include "pmath.h"
#include "stuff.h"


struct Segment{
	glm::dvec3 start;
	glm::dvec3 end;

};

struct Profile{



};


vector<Point> getPointsInAABB(string file, glm::dvec3 min, glm::dvec3 max){

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	PotreeReader *reader = new PotreeReader(file);
	vector<Point> points = reader->root->points();

	vector<Point> accepted;

	vector<PRNode*> intersectingNodes;
	stack<PRNode*> workload({reader->root});

	AABB box = {min, max};

	// nodes that intersect with box
	while(!workload.empty()){
		auto node = workload.top();
		workload.pop();

		intersectingNodes.push_back(node);

		for(auto child : node->children()){
			if(child != nullptr && child->boundingBox.intersects(box)){
				workload.push(child);
			}
		}
	}

	cout << "intersecting nodes: " << to_string(intersectingNodes.size()) << endl;

	int pointsProcessed = 0;

	for(auto node : intersectingNodes){
		for(auto &point : node->points()){
			pointsProcessed++;

			if(box.inside(point.position)){
				accepted.push_back(point);
			}
		}
	}

	std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();


	cout << "points processed: " << pointsProcessed << endl;
	cout << "points accepted: " << accepted.size() << endl;
	cout << "duration: " << milliseconds << " milliseconds" << endl;

	return accepted;
}

///
/// The box matrix maps a unit cube to the desired oriented cube.
/// The unit cube is assumed to have a size of 1/1/1 and it is 
/// centered around the origin, i.e. coordinates are [-0.5, 0.5]
///
/// algorithm: http://www.euclideanspace.com/maths/geometry/elements/intersection/twod/index.htm
/// 
vector<Point> getPointsInBox(PotreeReader *reader, dmat4 box, int minLevel, int maxLevel){

	cout << "== getPointsInBox() ==" << endl;

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

	cout << "intersecting nodes: " << intersectingNodes.size() << endl;
	cout << "points processed: " << pointsProcessed << endl;
	cout << "points accepted: " << accepted.size() << endl;
	cout << "duration: " << milliseconds << " milliseconds" << endl;

	return accepted;

}

vector<Point> getPointsInProfile(string file, vector<dvec2> polyline, double width, int minLevel, int maxLevel){

	PotreeReader *reader = new PotreeReader(file);
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

	for(Segment &segment : segments){
		auto box = segment.box;

		static int count = 0;
		string outfile = "./accepted_" + to_string(count++) + ".csv";

		cout << endl;
		vector<Point> points = getPointsInBox(reader, box, minLevel, maxLevel);
		cout << "saving result to: " << outfile << endl;

		{
			std::ofstream fout(outfile);
			fout << std::fixed;
			for(auto &point : points){
				fout << point.position.x 
					<< ", " << point.position.y
					<< ", " << point.position.z
					<< ", " << unsigned int(point.color.r)
					<< ", " << unsigned int(point.color.g)
					<< ", " << unsigned int(point.color.b)
					<< endl;
			}
			fout.close();
		}
	}

	vector<Point> accepted;

	return accepted;
}



int main(int argc, char* argv[]){

	cout.imbue(std::locale(""));

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
	
	cout << endl;
	vector<Point> points = getPointsInProfile(file, polyline, width, minLevel, maxLevel);


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