#pragma once

#include "StandardIncludes.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


using glm::dvec3;

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

AABB childAABB(AABB &aabb, int &index);