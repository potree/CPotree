
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
/// in 3D, the cube is first projected to a plane and then to a line.
///
///
vector<Point> getPointsInBox(string file, dmat4 box){
	
	//// INITIALIZE IN LOCAL SPACE
	//vector<dvec3> vertices = {
	//	{-0.5, -0.5, -0.5},
	//	{-0.5, -0.5, +0.5},
	//	{-0.5, +0.5, -0.5},
	//	{-0.5, +0.5, +0.5},
	//	{+0.5, -0.5, -0.5},
	//	{+0.5, -0.5, +0.5},
	//	{+0.5, +0.5, -0.5},
	//	{+0.5, +0.5, +0.5}
	//};
	//
	//vector<dvec3> axes = {
	//	{1, 0, 0},
	//	{0, 1, 0},
	//	{0, 0, 1}
	//};
	//
	//// TRANSFORM TO WORLD SPACE
	//for(int i = 0; i < vertices.size(); i++){
	//	vertices[i] = box * glm::dvec4(vertices[i], 1.0);
	//}
	//
	//for(int i = 0; i < axes.size(); i++){
	//	dvec3 tOrigin = box * dvec4(0.0, 0.0, 0.0, 1.0);
	//	dvec3 tAxe = box * dvec4(axes[i], 1.0);
	//	axes[i] = glm::normalize(tAxe - tOrigin);
	//}
	//
	//vector<vector<dvec3>> projections = {
	//	{axes[0], axes[1], axes[2]},
	//	{axes[1], axes[2], axes[0]},
	//	{axes[2], axes[0], axes[1]},
	//	{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
	//	{{0, 1, 0}, {0, 0, 1}, {1, 0, 0}},
	//	{{0, 0, 1}, {1, 0, 0}, {0, 1, 0}}
	//};

	// Precompute projections of the box to all six lines
	// (3 from oriented box, 3 from axis-aligned bounding box of nodes)
	//vector<dvec2> projectedIntervalls;
	//for(int i = 0; i < projections.size(); i++){
	//
	//	dvec2 intervall = {infinity, -infinity};
	//
	//	for(auto &vertex : vertices){
	//		auto &proj = projections[i];
	//		auto pr = project(project(vertex, proj[0]), proj[1]);
	//		double pi = glm::dot(pr, proj[2]);
	//		
	//		intervall[0] = std::min(intervall[0], pi);
	//		intervall[1] = std::max(intervall[1], pi);
	//	}
	//
	//	projectedIntervalls.push_back(intervall);
	//}

	OBB obb(box);


	//// now traverse through octree, project each node's bounding box 
	//// to the same six test 
	//PotreeReader *reader = new PotreeReader(file);
	//vector<PRNode*> intersectingNodes;
	//stack<PRNode*> workload({reader->root});
	//
	//// nodes that intersect with box
	//while(!workload.empty()){
	//	auto node = workload.top();
	//	workload.pop();
	//
	//	intersectingNodes.push_back(node);
	//
	//	for(auto child : node->children()){
	//		if(child != nullptr && child->boundingBox.intersects(box)){
	//			workload.push(child);
	//		}
	//	}
	//}



	vector<Point> accepted;

	return accepted;

}



int main(){

	//string file = "D:/dev/pointclouds/converted/nvidia/cloud.js";
	string file = "D:/dev/pointclouds/converted/CA13/cloud.js";
	PotreeReader *reader = new PotreeReader(file);

	cout << "getPointsInBox" << endl;

	dvec3 min = {693'493.980, 3'916'086.719, 3.790};
	dvec3 size = {1'259.57, 41.99, 503.719};
	dmat4 box = glm::translate(dmat4(), min)
		* glm::scale(dmat4(), size) 
		* glm::translate(dmat4(), {0.5, 0.5, 0.5});

	getPointsInBox(file, box);

	//{
	//	dvec3 min = {1, 1, 1};
	//	dvec3 size = {10, 10, 10};
	//	dmat4 box = glm::translate(dmat4(), min)
	//		* glm::scale(dmat4(), size) 
	//		* glm::translate(dmat4(), {0.5, 0.5, 0.5});
	//
	//	AABB aabb(
	//		{1, 12, 1}, 
	//		{20, 20, 20});
	//
	//	OBB obb(box);
	//
	//	if(obb.intersects(aabb)){
	//		cout << "intersection!" << endl;
	//	}else{
	//		cout << "no intersection" << endl;
	//	}
	//}

	

	// CA13
	//auto points = getPointsInBox(file,
	//	{693493.980, 3916086.719, 3.790},
	//	{694753.550, 3916128.709, 507.509});
	//auto points = getPointsInAABB(file,
	//	{693493.980, 3916086.719, 3.790},
	//	{694753.550, 3916090.709, 507.509});
	//
	//{
	//	std::ofstream fout("D:/temp/accepted.csv");
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


	// NVIDIA
	//getPointsInAABB(file,
	//	{591483.889, 4136465.249, -30.771},
	//	{591495.009, 4136487.14, 30.711});

	//getPointsInAABB(file,
	//	{589519.250, 231339.250, 743.195}, 
	//	{589810.750, 231511.620, 781.875});

	//getPointsInAABB(file,
	//	{589593.170, 231412.340, 763.945},
	//	{589642.290, 231459.130, 759.635});


	return 0;
}