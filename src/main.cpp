
#include "StandardIncludes.h"

#include "PotreeReader.h"
#include "Point.h"

#include "pmath.h"
#include "stuff.h"




void getPointsInBox(string file, glm::dvec3 min, glm::dvec3 max){

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

		//cout << node->name << endl;

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

		//cout << node->name << endl;

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

	std::ofstream fout("D:/temp/accepted.csv");
	fout << std::fixed;
	for(auto &point : accepted){
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


int main(){

	string file = "D:/dev/pointclouds/converted/nvidia/cloud.js";
	PotreeReader *reader = new PotreeReader(file);

	//vector<Point> points = reader->root->children()[0]->points();
	//
	//cout << "numPoints: " << points.size() << endl;
	//
	//std::ofstream fout("D:/temp/test.csv");
	//fout << std::fixed;
	//for(auto &point : points){
	//	fout << point.position.x 
	//		<< ", " << point.position.y
	//		<< ", " << point.position.z
	//		<< ", " << unsigned int(point.color.r)
	//		<< ", " << unsigned int(point.color.g)
	//		<< ", " << unsigned int(point.color.b)
	//		<< endl;
	//}
	//fout.close();
	//
	//cout << reader->root->boundingBox.toString() << endl;
	//cout << reader->root->children()[0]->boundingBox.toString() << endl;

	cout << "getPointsInBox" << endl;
	getPointsInBox(file,
		{591483.889, 4136465.249, -30.771},
		{591495.009, 4136487.14, 30.711});

	//getPointsInBox(file,
	//	{589519.250, 231339.250, 743.195}, 
	//	{589810.750, 231511.620, 781.875});

	//getPointsInBox(file,
	//	{589593.170, 231412.340, 763.945},
	//	{589642.290, 231459.130, 759.635});


	return 0;
}