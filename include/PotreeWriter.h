
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


vector<function<void(int64_t, uint8_t*)>> createAttributeHandlers(shared_ptr<ofstream> stream, Attributes& attributes) {

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
		for (auto& attribute : attributes.list) {

			if (mapping.find(attribute.name) != mapping.end()) {
				*stream << attribute.name << " ";
				handlers.push_back(mapping[attribute.name]);
			}
		}
		*stream << endl;

	}

	return handlers;
}


struct PotreeWriter : public Writer {

	string path;
	Attributes outputAttributes;

	AABB aabb;

	int64_t numAccepted = 0; 
	int64_t numRejected = 0;
	int64_t nodesProcessed = 0;


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

		auto buff_position = points->attributesData["position"];

		for(int64_t i = 0; i < numAccepted; i++){
			int32_t X, Y, Z;

			memcpy(&X, buff_position->data_u8 + i * 12 + 0, 4);
			memcpy(&Y, buff_position->data_u8 + i * 12 + 4, 4);
			memcpy(&Z, buff_position->data_u8 + i * 12 + 8, 4);

			double x = double(X) * scale.x + offset.x;
			double y = double(Y) * scale.y + offset.y;
			double z = double(Z) * scale.z + offset.z;

			aabb.expand(x, y, z);
		}

		this->numAccepted += numAccepted;
		this->numRejected += numRejected;
		nodesProcessed++;

		backlog.push_back(task);
	}

	void close() {
		double scale = 0.001;

		ofstream stream;
		stream.open(path, ios::binary);
		
		string header = createHeader(scale);

		int headerSize = header.size();
		stream.write(reinterpret_cast<const char*>(&headerSize), 4);
		stream.write(header.c_str(), headerSize);

		for (auto& task : backlog) {
			for (int64_t i = 0; i < task.numAccepted; i++) {

				auto points = task.points;
				auto inputAttributes = points->attributes;
				auto handlers = createAttributeHandlers(inputAttributes, outputAttributes, points, stream);

				for (auto& handler : handlers) {
					handler(i);
				}

			}
		}


		stream.close();
	}

	vector<function<void(int64_t)>> createAttributeHandlers(Attributes& sourceAttributes, Attributes targetAttributes, shared_ptr<Points> points, ofstream& target) {

		vector<function<void(int64_t)>> handlers;

		{ // STANDARD LAS ATTRIBUTES

			unordered_map<string, function<void(int64_t)>> mapping;

			{ // POSITION
				int offset = sourceAttributes.getOffset("position");
				auto buff_position = points->attributesData["position"];
				auto handler = [offset, sourceAttributes, targetAttributes, buff_position, &target](int64_t index) {

					int32_t X, Y, Z;

					memcpy(&X, buff_position->data_u8 + index * sourceAttributes.bytes + offset + 0, 4);
					memcpy(&Y, buff_position->data_u8 + index * sourceAttributes.bytes + offset + 4, 4);
					memcpy(&Z, buff_position->data_u8 + index * sourceAttributes.bytes + offset + 8, 4);

					double x = double(X) * sourceAttributes.posScale.x + sourceAttributes.posOffset.x;
					double y = double(Y) * sourceAttributes.posScale.y + sourceAttributes.posOffset.y;
					double z = double(Z) * sourceAttributes.posScale.z + sourceAttributes.posOffset.z;

					int32_t targetX = (x - targetAttributes.posOffset.x) / targetAttributes.posScale.x;
					int32_t targetY = (y - targetAttributes.posOffset.y) / targetAttributes.posScale.y;
					int32_t targetZ = (z - targetAttributes.posOffset.z) / targetAttributes.posScale.z;

					target.write(reinterpret_cast<const char*>(&targetX), 4);
					target.write(reinterpret_cast<const char*>(&targetY), 4);
					target.write(reinterpret_cast<const char*>(&targetZ), 4);
				};

				mapping["position"] = handler;
			}

			{ // RGB
				int offset = sourceAttributes.getOffset("rgb");
				auto buff_rgb = points->attributesData["rgb"];
				auto handler = [offset, sourceAttributes, targetAttributes, buff_rgb, &target](int64_t index) {
					uint16_t rgb[3];

					memcpy(&rgb, buff_rgb->data_u8 + index * sourceAttributes.bytes + offset, 6);

					target.write(reinterpret_cast<const char*>(&rgb), 6);
				};

				mapping["rgb"] = handler;
			}

			{ // INTENSITY
				int offset = sourceAttributes.getOffset("intensity");
				auto buff_intensity = points->attributesData["intensity"];
				auto handler = [offset, sourceAttributes, targetAttributes, buff_intensity, &target](int64_t index) {
					uint16_t intensity;

					memcpy(&intensity, buff_intensity->data_u8 + index * sourceAttributes.bytes + offset, 2);

					target.write(reinterpret_cast<const char*>(&intensity), 2);
				};

				mapping["intensity"] = handler;
			}


			for (auto& attribute : outputAttributes.list) {

				if (mapping.find(attribute.name) != mapping.end()) {
					handlers.push_back(mapping[attribute.name]);
				}
			}

		}

		return handlers;
	}


	string createHeader(double scale) {

		double durationMS = now() * 1000.0;

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
			header += "\t\"boundingBox\": {\n";
			header += "\t\t\"lx\": " + to_inf_safe_json(aabb.min.x) + ",\n";
			header += "\t\t\"ly\": " + to_inf_safe_json(aabb.min.y) + ",\n";
			header += "\t\t\"lz\": " + to_inf_safe_json(aabb.min.z) + ",\n";
			header += "\t\t\"ux\": " + to_inf_safe_json(aabb.max.x) + ",\n";
			header += "\t\t\"uy\": " + to_inf_safe_json(aabb.max.y) + ",\n";
			header += "\t\t\"uz\": " + to_inf_safe_json(aabb.max.z) + "\n";
			header += "\t},\n";

			header += "\t\"pointAttributes\": [\n";

			for (int i = 0; i < outputAttributes.list.size(); i++) {
				auto attribute = outputAttributes.list[i];
				header += "\t\t\"" + attribute.name + "\"";

				if (i < outputAttributes.list.size() - 1) {
					header += ",\n";
				} else {
					header += "\n";
				}

			}

			header += "\t],\n";

			header += "\t\"bytesPerPoint\": " + to_string(outputAttributes.bytes) + ",\n";
			header += "\t\"scale\": " + to_string(scale) + "\n";

			header += "}\n";
		}

		return header;

	}

};