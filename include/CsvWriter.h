
#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <fstream>

#include "laszip/laszip_api.h"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/constants.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include "unsuck/unsuck.hpp"
#include "Attributes.h"
#include "Node.h"

#include "Writer.h"

using glm::dvec2;
using glm::dvec3;
using glm::dvec4;
using glm::dmat4;

using std::vector;
using std::function;
using std::shared_ptr;
using std::mutex;
using std::lock_guard;
using std::ofstream;


vector<function<void(int64_t)>> createAttributeHandlers(shared_ptr<ofstream> stream, shared_ptr<Points> points, Attributes outputAttributes) {

	vector<function<void(int64_t)>> handlers;

	auto& inputAttributes = points->attributes;

	{ // STANDARD LAS ATTRIBUTES

		unordered_map<string, function<void(int64_t)>> mapping;

		{ // POSITION
			auto attribute = inputAttributes.get("position");
			auto buff_position = points->attributeBuffersMap["position"];
			auto posScale = inputAttributes.posScale;
			auto posOffset = inputAttributes.posOffset;
			auto handler = [stream, buff_position, attribute, posScale, posOffset](int64_t index) {

				int32_t X, Y, Z;

				memcpy(&X, buff_position->data_u8 + index * attribute->size + 0, 4);
				memcpy(&Y, buff_position->data_u8 + index * attribute->size + 4, 4);
				memcpy(&Z, buff_position->data_u8 + index * attribute->size + 8, 4);

				double x = double(X) * posScale.x + posOffset.x;
				double y = double(Y) * posScale.y + posOffset.y;
				double z = double(Z) * posScale.z + posOffset.z;

				*stream << x << ", " << y << ", " << z;
			};

			mapping["position"] = handler;
		}

		 { // RGB
		 	auto attribute = inputAttributes.get("rgb");
			auto source = points->attributeBuffersMap["rgb"];
		 	auto handler = [stream, source, attribute](int64_t index) {

				if(source){
					uint16_t rgb[3];

					memcpy(&rgb, source->data_u8 + index * attribute->size, attribute->size);

					uint32_t R = rgb[0] > 255 ? rgb[0] / 256 : rgb[0];
					uint32_t G = rgb[1] > 255 ? rgb[1] / 256 : rgb[1];
					uint32_t B = rgb[2] > 255 ? rgb[2] / 256 : rgb[2];

					*stream << R << ", " << G << ", " << B;
				}else{
					*stream << "0, 0, 0";
				}

		 		
		 	};

		 	mapping["rgb"] = handler;
		 }

		{ // INTENSITY
			auto attribute = inputAttributes.get("intensity");
			auto source = points->attributeBuffersMap["intensity"];
			auto handler = [stream, source, attribute](int64_t index) {
				if(source){
					uint16_t intensity;

					memcpy(&intensity, source->data_u8 + index * attribute->size, attribute->size);

					*stream << intensity;
				}else{
					*stream << "0, 0, 0";
				}
			};

			mapping["intensity"] = handler;
		}

		{ // CLASSIFICATION
			auto attribute = inputAttributes.get("classification");
			auto source = points->attributeBuffersMap["classification"];
			auto handler = [stream, source, attribute](int64_t index) {
				if(source){
					uint8_t classification;

					memcpy(&classification, source->data_u8 + index * attribute->size, attribute->size);

					*stream << classification;
				}else{
					*stream << "0, 0, 0";
				}
			};

			mapping["classification"] = handler;
		}


		// *stream << "#";
		for (auto& attribute : outputAttributes.list) {

			if (mapping.find(attribute.name) != mapping.end()) {
				// *stream << attribute.name << " ";
				handlers.push_back(mapping[attribute.name]);
			}
		}
		// *stream << endl;

	}

	return handlers;
}



struct CsvWriter : public Writer {

	string path;
	Attributes outputAttributes;

	int64_t numWrittenPoints = 0;

	//AABB aabb;

	mutex mtx_write;

	shared_ptr<ofstream> stream;

	CsvWriter(string path, Attributes outputAttributes) {
		this->path = path;
		this->outputAttributes = outputAttributes;

		stream = make_shared<ofstream>();

		stream->open(path);

		auto digits = std::numeric_limits<double>::max_digits10;

		*stream << std::setprecision(digits);
		stream->setf(ios::fixed);
	}

	void write(Node* node, shared_ptr<Points> points, int64_t numAccepted, int64_t numRejected) {

		lock_guard<mutex> lock(mtx_write);

		auto inputAttributes = points->attributes;
		auto handlers = createAttributeHandlers(stream, points, outputAttributes);

		int64_t numPoints = points->numPoints;


		for (int64_t i = 0; i < numPoints; i++) {

			//for(auto& handler : handlers){
			for(int j = 0; j < handlers.size(); j++){
				auto& handler = handlers[j];
				handler(i);

				if(j < handlers.size() - 1){
					*stream << ", "; 
				}
			}

			*stream << "\n";
		}


		// for(int i = 0; i < numPoints; i++){
		
		// 	string line = std::format("");

		// }


		//cout << "TODO" << endl;
		//exit(123);

		//int posOffset = inputAttributes.getOffset("position");

		//dvec3 scale = inputAttributes.posScale;
		//dvec3 offset = inputAttributes.posOffset;

		//for (int64_t i = 0; i < numPoints; i++) {

		//	for (auto& handler : handlers) {
		//		handler(i, data->data_u8);
		//	}

		//	if (i < numPoints - 1) {
		//		*stream << endl;
		//	}
		//}

	}

	void close() {
		stream->close();
		stream = nullptr;
	}

};