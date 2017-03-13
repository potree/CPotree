
#include "pmath.h"




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