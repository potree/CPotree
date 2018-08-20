
#include "PotreeReader.h"

const PointAttribute PointAttribute::POSITION_CARTESIAN		= PointAttribute(0, "POSITION_CARTESIAN",	3, 12);
const PointAttribute PointAttribute::POSITION_PROJECTED_PROFILE	= PointAttribute(1, "POSITION_PROJECTED_PROFILE",	2, 8);
const PointAttribute PointAttribute::COLOR_PACKED			= PointAttribute(2, "COLOR_PACKED",			4, 4);
const PointAttribute PointAttribute::RGB					= PointAttribute(3, "RGB",					3, 3);
const PointAttribute PointAttribute::INTENSITY				= PointAttribute(4, "INTENSITY",			1, 2);
const PointAttribute PointAttribute::CLASSIFICATION			= PointAttribute(5, "CLASSIFICATION",		1, 1);
const PointAttribute PointAttribute::NORMAL_SPHEREMAPPED	= PointAttribute(6, "NORMAL_SPHEREMAPPED",	2, 2);
const PointAttribute PointAttribute::NORMAL_OCT16			= PointAttribute(7, "NORMAL_OCT16",			2, 2);
const PointAttribute PointAttribute::NORMAL					= PointAttribute(8, "NORMAL",				3, 12);

PointAttribute PointAttribute::fromString(string name){
	if(name == "POSITION_CARTESIAN"){
		return PointAttribute::POSITION_CARTESIAN;
	}else if(name == "POSITION_PROJECTED_PROFILE"){
		return PointAttribute::POSITION_PROJECTED_PROFILE;
	}else if(name == "COLOR_PACKED"){
		return PointAttribute::COLOR_PACKED;
	}else if(name == "RGB"){
		return PointAttribute::RGB;
	}else if(name == "INTENSITY"){
		return PointAttribute::INTENSITY;
	}else if(name == "CLASSIFICATION"){
		return PointAttribute::CLASSIFICATION;
	}else if(name == "NORMAL_SPHEREMAPPED"){
		return PointAttribute::NORMAL_SPHEREMAPPED;
	}else if(name == "NORMAL_OCT16"){
		return PointAttribute::NORMAL_OCT16;
	}else if(name == "NORMAL"){
		return PointAttribute::NORMAL;
	}

	throw "Invalid PointAttribute name: '" + name + "'";
}

//bool operator==(const PointAttribute& lhs, const PointAttribute& rhs){
//	return lhs.ordinal == rhs.ordinal;
//}

vector<Point> PRNode::points(){
	if(!loaded){
		reader->load(this);
	}

	return _points;
}

vector<PRNode*> PRNode::children(){
	if(hierarchyLoaded){
		return _children;
	}else{
		reader->loadHierarchy(this);

		return _children;
	}
}


void PotreeReader::load(PRNode *node){
	size_t rIndex = file.rfind("cloud.js");
	string basePath = file.substr(0, rIndex);
	string fNode = basePath + "data/" + getHierarchyPath(node) + "/" + node->name + ".bin";

	vector<char> buffer;
	{
		ifstream in(fNode, std::ios::binary);
		int byteSize = fs::file_size(fNode);

		buffer.resize(byteSize, 0);
		in.read(reinterpret_cast<char*>(&buffer[0]), byteSize);
	}

	node->numPoints = buffer.size() / metadata.pointAttributes.byteSize;

	vector<Point> points;
	points.resize(node->numPoints);

	glm::dvec3 &scale = metadata.scale;
	glm::dvec3 &min = node->boundingBox.min;
	int offset = 0;
	for(int i = 0; i < node->numPoints; i++){

		Point &point = points[i];

		for(auto attribute : metadata.pointAttributes.attributes){

			if(attribute == PointAttribute::POSITION_CARTESIAN){
				auto coordinates = reinterpret_cast<unsigned int*>(&buffer[offset]);

				unsigned int ux = coordinates[0];
				unsigned int uy = coordinates[1];
				unsigned int uz = coordinates[2];

				double x = ux * scale.x + min.x;
				double y = uy * scale.y + min.y;
				double z = uz * scale.z + min.z;

				point.position.x = x;
				point.position.y = y;
				point.position.z = z;
			}else if(attribute == PointAttribute::COLOR_PACKED){
				auto colors = reinterpret_cast<unsigned char*>(&buffer[offset]);

				point.color.r = colors[0];
				point.color.g = colors[1];
				point.color.b = colors[2];
			}else if(attribute == PointAttribute::INTENSITY){
				auto intensity = reinterpret_cast<unsigned short*>(&buffer[offset])[0];

				point.intensity = intensity;
			}else if(attribute == PointAttribute::CLASSIFICATION){
				auto classification = reinterpret_cast<unsigned char*>(&buffer[offset])[0];

				point.classification = classification;
			}

			offset += attribute.byteSize;
		}
	}

	node->_points = points;
	node->loaded = true;
}












