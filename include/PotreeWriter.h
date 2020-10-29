
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


struct PotreeWriter : public Writer {

	string path;
	Attributes outputAttributes;

	AABB aabb;

	int64_t numAccepted = 0; 
	int64_t numRejected = 0;
	int64_t nodesProcessed = 0;

	mutex mtx_write;

	struct Task {
		shared_ptr<Points> points;
		int64_t numAccepted = 0;
	};

	vector<Task> backlog;

	PotreeWriter(string path, dvec3 scale, dvec3 offset, Attributes outputAttributes) {
		this->path = path;
		this->outputAttributes = outputAttributes;
	}

	void write(Node* node, shared_ptr<Points> points, int64_t numAccepted, int64_t numRejected) {

		auto inputAttributes = points->attributes;

		Task task;
		task.points = points;
		task.numAccepted = numAccepted;

		dvec3 scale = inputAttributes.posScale;
		dvec3 offset = inputAttributes.posOffset;

		auto buff_position = points->attributeBuffersMap["position"];

		AABB aabbBatch;
		for(int64_t i = 0; i < numAccepted; i++){
			int32_t X, Y, Z;

			memcpy(&X, buff_position->data_u8 + i * 12 + 0, 4);
			memcpy(&Y, buff_position->data_u8 + i * 12 + 4, 4);
			memcpy(&Z, buff_position->data_u8 + i * 12 + 8, 4);

			double x = double(X) * scale.x + offset.x;
			double y = double(Y) * scale.y + offset.y;
			double z = double(Z) * scale.z + offset.z;

			aabbBatch.expand(x, y, z);
		}

		lock_guard<mutex> lock(mtx_write);

		if (numAccepted > 0) {
			aabb.expand(aabbBatch.min);
			aabb.expand(aabbBatch.max);

			backlog.push_back(task);
		}

		this->numAccepted += numAccepted;
		this->numRejected += numRejected;
		nodesProcessed++;

	}

	void close() {

		ofstream stream;
		stream.open(path, ios::binary);
		
		string header = createHeader();

		int headerSize = header.size();
		stream.write(reinterpret_cast<const char*>(&headerSize), 4);
		stream.write(header.c_str(), headerSize);

		outputAttributes.posOffset = aabb.min;

		for (auto& attribute : outputAttributes.list) {
			for (auto& task : backlog) {
				auto points = task.points;

				// reencode position with new scale and offset
				if(attribute.name == "position"){ 
					dvec3 scale = outputAttributes.posScale;
					dvec3 offset = aabb.min;

					auto buffer = points->attributeBuffersMap["position"];
					auto i32 = buffer->data_i32;

					for (int64_t i = 0; i < points->numPoints; i++) {
						dvec3 xyz = points->getPosition(i);

						i32[3 * i + 0] = (xyz.x - offset.x) / scale.x;
						i32[3 * i + 1] = (xyz.y - offset.y) / scale.y;
						i32[3 * i + 2] = (xyz.z - offset.z) / scale.z;
					}
				}

				// reencode position_projected_profile with new scale and offset
				if(attribute.name == "position_projected_profile"){ 
					dvec3 scale = outputAttributes.posScale;
					dvec3 offset = aabb.min;

					auto buffer = points->attributeBuffersMap["position_projected_profile"];
					auto i32 = buffer->data_i32;

					auto scaleIn = points->attributes.posScale;
					auto scaleOut = outputAttributes.posScale;

					for (int64_t i = 0; i < points->numPoints; i++) {

						auto X = i32[2 * i + 0];
						auto Z = i32[2 * i + 1];

						int32_t X1 = (X * scaleIn[0]) / scaleOut[0];
						int32_t Z1 = (Z * scaleIn[2]) / scaleOut[2];

						i32[2 * i + 0] = X1;
						i32[2 * i + 1] = Z1;
					}
				}


				bool hasAttribute = points->attributes.get(attribute.name);
				if (hasAttribute) {
					auto buffer = points->attributeBuffersMap[attribute.name];

					if (buffer->size > points->numPoints* attribute.size) {
						int debug = 10;
					}
					stream.write(buffer->data_char, buffer->size);
				} else {
					Buffer buffer(attribute.size * points->numPoints);
					stream.write(buffer.data_char, buffer.size);
				}
				
			}
		}

		stream.close();
	}


	string createHeader() {

		double durationMS = now() * 1000.0;

		auto s = [](string str) {
			return "\"" + str + "\"";
		};

		auto t = [](int numTabs) {
			return string(numTabs, '\t');
		};

		auto d = [](double value) {
			auto digits = std::numeric_limits<double>::max_digits10;

			std::stringstream ss;
			ss << std::setprecision(digits);
			ss << value;

			return ss.str();
		};

		string header;
		{ // HEADER
			header += "{\n";
			header += "\t\"points\": " + to_string(numAccepted) + ",\n";
			header += "\t\"pointsProcessed\": " + to_string(numAccepted + numRejected) + ",\n";
			header += "\t\"nodesProcessed\": " + to_string(nodesProcessed) + ",\n";
			header += "\t\"durationMS\": " + to_string(durationMS) + ",\n";

			auto to_inf_safe_json = [](double number) -> string {
				if (number == Infinity) {
					return "\"Infinity\"";
				} else if (number == -Infinity) {
					return "\"-Infinity\"";
				} else {
					return to_string(number);
				}
			};

			// BOUNDING BOX
			header += t(1) + s("boundingBox") + ": {\n";
			header += t(2) + s("min") + ": [" + d(aabb.min.x) + ", " + d(aabb.min.y) + ", " + d(aabb.min.z) + "],\n";
			header += t(2) + s("max") + ": [" + d(aabb.max.x) + ", " + d(aabb.max.y) + ", " + d(aabb.max.z) + "],\n";
			header += t(1) + "},\n";

			header += "\t\"attributes\": [\n";

			for (int i = 0; i < outputAttributes.list.size(); i++) {
				auto attribute = outputAttributes.list[i];

				if (i == 0) {
					header += t(2) + "{\n";
				}

				header += t(3) + s("name") + ": " + s(attribute.name) + ",\n";
				header += t(3) + s("description") + ": \"\",\n";
				header += t(3) + s("size") + ": " + std::to_string(attribute.size) + ",\n";
				header += t(3) + s("numElements") + ": " + std::to_string(attribute.numElements) + ",\n";
				header += t(3) + s("elementSize") + ": " + std::to_string(attribute.elementSize) + ",\n";
				header += t(3) + s("type") + ": " + s(getAttributeTypename(attribute.type)) + ",\n";

				if (i < outputAttributes.list.size() - 1) {
					header += "\t\t},{\n";
				} else {
					header += "\t\t}\n";
				}

			}

			header += "\t],\n";

			header += "\t\"bytesPerPoint\": " + to_string(outputAttributes.bytes) + ",\n";
			//header += "\t\"scale\": " + to_string(scale) + "\n";
			header += t(1) + s("scale") + ": [ "
				+ d(outputAttributes.posScale.x) + ", "
				+ d(outputAttributes.posScale.y) + ", "
				+ d(outputAttributes.posScale.z) + "],\n";
			//header += t(1) + s("offset") + ": [ " 
			//	+ d(outputAttributes.posOffset.x) + ", "
			//	+ d(outputAttributes.posOffset.y) + ", "
			//	+ d(outputAttributes.posOffset.z) + "],\n";
			//dvec3 scale = outputAttributes.posScale;
			//dvec3 offset = aabb.min;
			

			header += "}\n";
		}

		return header;

	}

};