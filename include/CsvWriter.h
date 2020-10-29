
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


vector<function<void(int64_t, uint8_t*)>> createAttributeHandlers(shared_ptr<ofstream> stream, Attributes& attributes, Attributes outputAttributes) {

	vector<function<void(int64_t, uint8_t*)>> handlers;

	{ // STANDARD LAS ATTRIBUTES

		unordered_map<string, function<void(int64_t, uint8_t*)>> mapping;

		{ // POSITION
			int offset = attributes.getOffset("position");
			auto handler = [stream, offset, attributes](int64_t index, uint8_t* data) {

				int32_t X, Y, Z;

				memcpy(&X, data + index * attributes.bytes + offset + 0, 4);
				memcpy(&Y, data + index * attributes.bytes + offset + 4, 4);
				memcpy(&Z, data + index * attributes.bytes + offset + 8, 4);

				double x = double(X) * attributes.posScale.x + attributes.posOffset.x;
				double y = double(Y) * attributes.posScale.y + attributes.posOffset.y;
				double z = double(Z) * attributes.posScale.z + attributes.posOffset.z;

				*stream << x << " " << y << " " << z;
			};

			mapping["position"] = handler;
		}

		{ // RGB
			int offset = attributes.getOffset("rgb");
			auto handler = [stream, offset, attributes](int64_t index, uint8_t* data) {
				uint16_t rgb[3];

				memcpy(&rgb, data + index * attributes.bytes + offset, 6);

				*stream << " " << rgb[0] << " " << rgb[1] << " " << rgb[2];
			};

			mapping["rgb"] = handler;
		}

		{ // INTENSITY
			int offset = attributes.getOffset("intensity");
			auto handler = [stream, offset, attributes](int64_t index, uint8_t* data) {
				uint16_t intensity;
				memcpy(&intensity, data + index * attributes.bytes + offset, 2);

				*stream << " " << intensity;
			};

			mapping["intensity"] = handler;
		}


		*stream << "#";
		for (auto& attribute : outputAttributes.list) {

			if (mapping.find(attribute.name) != mapping.end()) {
				*stream << attribute.name << " ";
				handlers.push_back(mapping[attribute.name]);
			}
		}
		*stream << endl;

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
		auto handlers = createAttributeHandlers(stream, inputAttributes, outputAttributes);

		int64_t numPoints = points->numPoints;

		cout << "TODO" << endl;
		exit(123);

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