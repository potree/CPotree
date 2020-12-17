
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


struct PotreeWriter_v1 : public Writer {

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

	PotreeWriter_v1(string path, dvec3 scale, dvec3 offset, Attributes outputAttributes) {
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

		ostream* stream = nullptr;

		if (path == "stdout") {
			stream = &cout;
		} else {
			ofstream* ofs = new ofstream();
			ofs->open(path, ios::binary);

			stream = ofs;
		}
		
		string header = createHeader();

		int headerSize = header.size();
		stream->write(reinterpret_cast<const char*>(&headerSize), 4);
		stream->write(header.c_str(), headerSize);

		outputAttributes.posOffset = aabb.min;

		for (auto& task : backlog) {
			auto points = task.points;

			for (int64_t i = 0; i < points->numPoints; i++) {

				for (auto& attribute : outputAttributes.list) {

					
					if(attribute.name == "position"){ 
						// reencode position with new scale and offset
						dvec3 scale = outputAttributes.posScale;
						dvec3 offset = aabb.min;

						auto buffer = points->attributeBuffersMap["position"];
						auto i32 = buffer->data_i32;

						dvec3 xyz = points->getPosition(i);

						int32_t X = (xyz.x - offset.x) / scale.x;
						int32_t Y = (xyz.y - offset.y) / scale.y;
						int32_t Z = (xyz.z - offset.z) / scale.z;

						stream->write(reinterpret_cast<const char*>(&X), 4);
						stream->write(reinterpret_cast<const char*>(&Y), 4);
						stream->write(reinterpret_cast<const char*>(&Z), 4);
						
					}else if(attribute.name == "position_projected_profile"){ 
						// reencode position_projected_profile with new scale and offset
						dvec3 scale = outputAttributes.posScale;
						dvec3 offset = aabb.min;

						auto buffer = points->attributeBuffersMap["position_projected_profile"];
						auto i32 = buffer->data_i32;

						auto scaleIn = points->attributes.posScale;
						auto scaleOut = outputAttributes.posScale;

						auto X = i32[2 * i + 0];
						auto Z = i32[2 * i + 1];

						int32_t X1 = (X * scaleIn[0]) / scaleOut[0];
						int32_t Z1 = (Z * scaleIn[2]) / scaleOut[2];
						
						stream->write(reinterpret_cast<const char*>(&X1), 4);
						stream->write(reinterpret_cast<const char*>(&Z1), 4);
					}else if (attribute.name == "rgb") {
						auto buffer = points->attributeBuffersMap["rgb"];
						auto u16 = buffer->data_u16;

						uint16_t R = u16[3 * i + 0];
						uint16_t G = u16[3 * i + 1];
						uint16_t B = u16[3 * i + 2];

						uint8_t r = R > 255 ? R / 256 : R;
						uint8_t g = G > 255 ? G / 256 : G;
						uint8_t b = B > 255 ? B / 256 : B;

						stream->write(reinterpret_cast<const char*>(&r), 1);
						stream->write(reinterpret_cast<const char*>(&g), 1);
						stream->write(reinterpret_cast<const char*>(&b), 1);
					} else if (attribute.name == "intensity") {
						auto buffer = points->attributeBuffersMap["intensity"];
						auto intensity = buffer->data_u16[i];

						stream->write(reinterpret_cast<const char*>(&intensity), 2);
					} else if (attribute.name == "classification") {
						auto buffer = points->attributeBuffersMap["classification"];
						auto classification = buffer->data_u8[i];

						stream->write(reinterpret_cast<const char*>(&classification), 1);
					}


					//bool hasAttribute = points->attributes.get(attribute.name);
					//if (hasAttribute) {
					//	auto buffer = points->attributeBuffersMap[attribute.name];

					//	if (buffer->size > points->numPoints* attribute.size) {
					//		int debug = 10;
					//	}
					//	stream->write(buffer->data_char, buffer->size);
					//} else {
					//	Buffer buffer(attribute.size * points->numPoints);
					//	stream->write(buffer.data_char, buffer.size);
					//}
				
				}
			}
		}

		if (path == "stdout") {
			// dont do anything
		} else {
			ofstream* ofs = (ofstream*)stream;
			ofs->close();
		}

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

			{ // BOUNDING BOX
				header += "\t\"boundingBox\": {\n";
				header += "\t\t\"lx\": " + to_inf_safe_json(aabb.min.x) + ",\n";
				header += "\t\t\"ly\": " + to_inf_safe_json(aabb.min.y) + ",\n";
				header += "\t\t\"lz\": " + to_inf_safe_json(aabb.min.z) + ",\n";
				header += "\t\t\"ux\": " + to_inf_safe_json(aabb.max.x) + ",\n";
				header += "\t\t\"uy\": " + to_inf_safe_json(aabb.max.y) + ",\n";
				header += "\t\t\"uz\": " + to_inf_safe_json(aabb.max.z) + "\n";
				header += "\t},\n";
			}

			int bytesPerPoint = 0;
			{// ATTRIBUTES
				header += "\t\"pointAttributes\": [\n";

				for (int i = 0; i < outputAttributes.list.size(); i++) {
					auto attribute = outputAttributes.list[i];

					if(attribute.name == "position"){
						header += t(3) + s("POSITION_CARTESIAN");
						bytesPerPoint += 12;
					}else if(attribute.name == "rgb"){
						header += t(3) + s("RGB");
						bytesPerPoint += 3;
					}else if(attribute.name == "intensity"){
						header += t(3) + s("INTENSITY");
						bytesPerPoint += 2;
					} else if (attribute.name == "classification") {
						header += t(3) + s("CLASSIFICATION");
						bytesPerPoint += 1;
					} else if(attribute.name == "position_projected_profile"){
						header += t(3) + s("POSITION_PROJECTED_PROFILE");
						bytesPerPoint += 8;
					}else{
						continue;
					}

					if (i < outputAttributes.list.size() - 1) {
						header += ",\n";
					} else {
						header += "\n";
					}

				}

				header += "\t],\n";
			}

			header += "\t\"bytesPerPoint\": " + to_string(bytesPerPoint) + ",\n";
			header += "\t\"scale\": " + to_string(outputAttributes.posScale.x) + "\n";

			header += "}\n";
		}

		return header;

	}

};