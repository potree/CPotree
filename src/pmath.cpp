
#include "pmath.h"




OBB::OBB(dmat4 box){
	this->box = box;
	this->boxInverse = glm::inverse(box);

	// INITIALIZE IN LOCAL SPACE
	vertices = {
		{-0.5, -0.5, -0.5},
		{-0.5, -0.5, +0.5},
		{-0.5, +0.5, -0.5},
		{-0.5, +0.5, +0.5},
		{+0.5, -0.5, -0.5},
		{+0.5, -0.5, +0.5},
		{+0.5, +0.5, -0.5},
		{+0.5, +0.5, +0.5}
	};

	axes = {
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1}
	};

	// TRANSFORM TO WORLD SPACE
	for(size_t i = 0; i < vertices.size(); i++){
		vertices[i] = box * glm::dvec4(vertices[i], 1.0);
	}

	for(size_t i = 0; i < axes.size(); i++){
		dvec3 tOrigin = box * dvec4(0.0, 0.0, 0.0, 1.0);
		dvec3 tAxe = box * dvec4(axes[i], 1.0);
		axes[i] = glm::normalize(tAxe - tOrigin);
	}

	projections = {
		{axes[0], axes[1], axes[2]},
		{axes[1], axes[2], axes[0]},
		{axes[2], axes[0], axes[1]},
		{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
		{{0, 1, 0}, {0, 0, 1}, {1, 0, 0}},
		{{0, 0, 1}, {1, 0, 0}, {0, 1, 0}}
	};

	for(auto &proj : projections){

		dvec2 intervall = {infinity, -infinity};

		for(auto &vertex : vertices){
			auto pr = project(project(vertex, proj[0]), proj[1]);
			double pi = glm::dot(pr, proj[2]);
		
			intervall[0] = std::min(intervall[0], pi);
			intervall[1] = std::max(intervall[1], pi);
		}

		projectedIntervalls.push_back(intervall);
	}
}


bool OBB::intersects(AABB &aabb){

	vector<dvec2> projectedIntervallsAABB;

	for(auto &proj : projections){

		dvec2 intervall = {infinity, -infinity};

		for(auto &vertex : aabb.vertices()){
			auto pr = project(project(vertex, proj[0]), proj[1]);
			double pi = glm::dot(pr, proj[2]);
		
			intervall[0] = std::min(intervall[0], pi);
			intervall[1] = std::max(intervall[1], pi);
		}

		projectedIntervallsAABB.push_back(intervall);
	}

	for(size_t i = 0; i < projectedIntervalls.size(); i++){
		auto piOBB = projectedIntervalls[i];
		auto piAABB = projectedIntervallsAABB[i];

		if((piAABB[1] < piOBB[0]) || (piAABB[0] > piOBB[1])){
			// found a gap at one of the projections => no intersection
			return false;
		}

	}

	return true;
}




/**
 * index bits are in xyz order
 * 100 -> x=1, y=0, z=0
 * 011 -> x=0, y=1, z=1
 */
AABB childAABB(AABB &aabb, int &index){

	dvec3 min = aabb.min;
	dvec3 max = aabb.max;

	auto size = aabb.size();
	
	if((index & 0b0001) > 0){
		min.z += size.z / 2;
	}else{
		max.z -= size.z / 2;
	}
	
	if((index & 0b0010) > 0){
		min.y += size.y / 2;
	}else{
		max.y -= size.y / 2;
	}
	
	if((index & 0b0100) > 0){
		min.x += size.x / 2;
	}else{
		max.x -= size.x / 2;
	}

	return {min, max};
}

dvec3 project(dvec3 point, dvec3 normal){
	
	//dvec3 np = point - normal;
	double d = glm::dot(point, normal);
	dvec3 projected = point - normal * d;

	return projected;
}