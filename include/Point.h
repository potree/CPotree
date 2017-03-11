#pragma once

#include <glm/glm.hpp>


struct Point{

	glm::dvec3 position = {0.0, 0.0, 0.0};
	glm::u8vec3 color = {255, 255, 255};
	unsigned short intensity = 0;
	unsigned char classification = 0;
	unsigned char returnNumber = 0;
	unsigned char numberOfReturns = 0;
	unsigned short pointSourceID = 0;

	Point(){
	
	}



};