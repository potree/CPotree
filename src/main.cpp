
#include "StandardIncludes.h"

#include "PotreeReader.h"
#include "Point.h"

#include <glm/gtc/constants.hpp>


int main(){

	string file = "D:/dev/pointclouds/converted/vol_total/cloud.js";
	PotreeReader *reader = new PotreeReader(file);

	cout << "spacing: " << reader->metadata.spacing << endl;
	
	cout << reader->root->children[0]->name << endl;

	vector<Point> points = reader->root->points();

	cout << "numPoints: " << points.size() << endl;

	auto point = points[0];

	cout << point.position.x 
		<< ", " << point.position.y
		<< ", " << point.position.z
		<< endl;
	

	return 0;
}