//
//#include "StandardIncludes.h"
//#include "stuff.h"
//
//void saveLAS(PotreeReader *reader, vector<FilterResult> results, PointAttributes attributes, ostream *out){
//	//*out << "creating a las file";
//
//	unsigned int numPoints = 0;
//	for(auto &result : results){
//		numPoints += result.points.size();
//	}
//
//	vector<char> zeroes = vector<char>(8, 0);
//
//	out->write("LASF", 4);			// File Signature
//	out->write(zeroes.data(), 2);	// File Source ID
//	out->write(zeroes.data(), 2);	// Global Encoding
//	out->write(zeroes.data(), 4);	// Project ID data 1
//	out->write(zeroes.data(), 2);	// Project ID data 2
//	out->write(zeroes.data(), 2);	// Project ID data 3
//	out->write(zeroes.data(), 8);	// Project ID data 4
//
//	// Version Major
//	char versionMajor = 1;
//	out->write(reinterpret_cast<const char*>(&versionMajor), 1);				
//
//	// Version Minor
//	char versionMinor = 2;
//	out->write(reinterpret_cast<const char*>(&versionMinor), 1);				
//
//	out->write("PotreeElevationProfile          ", 32);	// System Identifier
//	out->write("PotreeElevationProfile          ", 32); // Generating Software
//
//	// File Creation Day of Year
//	unsigned short day = 0;
//	out->write(reinterpret_cast<const char*>(&day), 2);	
//
//	// File Creation Year
//	unsigned short year = 0;
//	out->write(reinterpret_cast<const char*>(&year), 2);	
//
//	// Header Size
//	unsigned short headerSize = 227;
//	out->write(reinterpret_cast<const char*>(&headerSize), 2);	
//
//	// Offset to point data
//	unsigned long offsetToData = 227;
//	out->write(reinterpret_cast<const char*>(&offsetToData), 4);	
//
//	// Number variable length records
//	out->write(zeroes.data(), 4);	
//
//	// Point Data Record Format
//	unsigned char pointFormat = 2;
//	out->write(reinterpret_cast<const char*>(&pointFormat), 1);	
//
//	// Point Data Record Length
//	unsigned short pointRecordLength = 26;
//	out->write(reinterpret_cast<const char*>(&pointRecordLength), 2);	
//
//	// Number of points
//	out->write(reinterpret_cast<const char*>(&numPoints), 4);
//
//	// Number of points by return 0 - 4
//	out->write(reinterpret_cast<const char*>(&numPoints), 4);
//	out->write(zeroes.data(), 4);
//	out->write(zeroes.data(), 4);
//	out->write(zeroes.data(), 4);
//	out->write(zeroes.data(), 4);
//
//	// XYZ scale factors
//	dvec3 scale = reader->metadata.scale;
//	out->write(reinterpret_cast<const char*>(&scale), 3 * 8);
//
//	// XYZ OFFSETS
//	dvec3 offsets = reader->metadata.boundingBox.min;
//	out->write(reinterpret_cast<const char*>(&offsets), 3 * 8);
//
//	// MAX X, MIN X, MAX Y, MIN Y, MAX Z, MIN Z
//	auto bb = reader->metadata.boundingBox;
//	out->write(reinterpret_cast<const char*>(&bb.max.x), 8);
//	out->write(reinterpret_cast<const char*>(&bb.min.x), 8);
//	out->write(reinterpret_cast<const char*>(&bb.max.y), 8);
//	out->write(reinterpret_cast<const char*>(&bb.min.y), 8);
//	out->write(reinterpret_cast<const char*>(&bb.max.z), 8);
//	out->write(reinterpret_cast<const char*>(&bb.min.z), 8);
//
//	vector<char> buffer(pointRecordLength, 0);
//
//	for(auto &result : results){
//	
//		dmat4 box = result.box;
//		OBB obb(box);
//		dvec3 localMin = dvec3(box * dvec4(-0.5, -0.5, -0.5, 1.0));
//		dvec3 lvx = dvec3(box * dvec4(+0.5, -0.5, -0.5, 1.0)) - localMin;
//	
//	
//	
//		for(Point &p : result.points){
//	
//			int *ixyz = reinterpret_cast<int*>(&buffer[0]);
//			unsigned short *intensity = reinterpret_cast<unsigned short*>(&buffer[12]);
//			unsigned short *rgb = reinterpret_cast<unsigned short*>(&buffer[20]);
//	
//			ixyz[0] = (p.position.x - bb.min.x) / scale.x;
//			ixyz[1] = (p.position.y - bb.min.y) / scale.y;
//			ixyz[2] = (p.position.z - bb.min.z) / scale.z;
//	
//			intensity[0] = p.intensity;
//	
//			rgb[0] = p.color.r;
//			rgb[1] = p.color.g;
//			rgb[2] = p.color.b;
//	
//			out->write(buffer.data(), pointRecordLength);
//		}
//	}
//
//	out->flush();
//
//
//
//
//
//
//
//}
//
//int main(int argc, char* argv[]){
//
//	
//
//}