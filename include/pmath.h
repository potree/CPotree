#pragma once

#include "StandardIncludes.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>


using glm::dvec2;
using glm::dvec3;
using glm::dvec4;
using glm::dmat4;

static const double infinity = numeric_limits<double>::infinity();

struct AABB{
	dvec3 min = {infinity, infinity, infinity};
	dvec3 max = {-infinity, -infinity, -infinity};
	
	AABB(){
	
	}

	AABB(dvec3 min, dvec3 max){
		this->min = min;
		this->max = max;
	}

	dvec3 size(){
		return max - min;
	}

	vector<dvec3> vertices(){
		
		vector<dvec3> v = {
			{min.x, min.y, min.z},
			{min.x, min.y, max.z},
			{min.x, max.y, min.z},
			{min.x, max.y, max.z},
			{max.x, min.y, min.z},
			{max.x, min.y, max.z},
			{max.x, max.y, min.z},
			{max.x, max.y, max.z}
		};

		return v;
	}

	string toString(){
		string msg = "{{" + to_string(min.x) + ", " + to_string(min.y) + ", " + to_string(min.z) + "}, "
			+ "{" + to_string(max.x) + ", " + to_string(max.y) + ", " + to_string(max.z) + "}}";

		return msg;
	}

	bool intersects(AABB box){
		
		// box test from three.js, src/math/Box3.js

		if ( box.max.x < this->min.x || box.min.x > this->max.x ||
				 box.max.y < this->min.y || box.min.y > this->max.y ||
				 box.max.z < this->min.z || box.min.z > this->max.z ) {

			return false;

		}

		return true;
	}

	bool inside(dvec3 &point){
		if(point.x < min.x || point.x > max.x){
			return false;
		}
	
		if(point.y < min.y || point.y > max.y){
			return false;
		}

		return true;
	}
};

struct OBB{

	dmat4 box;
	vector<dvec3> vertices;
	vector<dvec3> axes;
	vector<vector<dvec3>> projections;
	vector<dvec2> projectedIntervalls;

	OBB(dmat4 box);

	bool intersects(AABB &aabb);
};

AABB childAABB(AABB &aabb, int &index);

dvec3 project(dvec3 point, dvec3 normal);



