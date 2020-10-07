
#pragma once

#include <vector>
#include <functional>
#include<memory>
#include<mutex>
#include<thread>

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


vector<function<void(int64_t, uint8_t*)>> createAttributeHandlers(laszip_header* header, laszip_point* point, Attributes& attributes, Attributes outputAttributes) {

	vector<function<void(int64_t, uint8_t*)>> handlers;

	{ // STANDARD LAS ATTRIBUTES

		unordered_map<string, function<void(int64_t, uint8_t*)>> mapping;

		{ // POSITION
			int offset = attributes.getOffset("position");
			auto handler = [header, point, offset, attributes](int64_t index, uint8_t* data) {

				int32_t X, Y, Z;

				memcpy(&X, data + index * attributes.bytes + offset + 0, 4);
				memcpy(&Y, data + index * attributes.bytes + offset + 4, 4);
				memcpy(&Z, data + index * attributes.bytes + offset + 8, 4);

				double x = double(X) * attributes.posScale.x + attributes.posOffset.x;
				double y = double(Y) * attributes.posScale.y + attributes.posOffset.y;
				double z = double(Z) * attributes.posScale.z + attributes.posOffset.z;

				point->X = (x - header->x_offset) / header->x_scale_factor;
				point->Y = (y - header->y_offset) / header->y_scale_factor;
				point->Z = (z - header->z_offset) / header->z_scale_factor;
			};

			mapping["position"] = handler;
		}

		{ // RGB
			int offset = attributes.getOffset("rgb");
			auto handler = [point, offset, attributes](int64_t index, uint8_t* data) {
				auto& rgba = point->rgb;

				memcpy(&rgba, data + index * attributes.bytes + offset, 6);
			};

			mapping["rgb"] = handler;
		}

		{ // INTENSITY
			int offset = attributes.getOffset("intensity");
			auto handler = [point, offset, attributes](int64_t index, uint8_t* data) {
				memcpy(&point->intensity, data + index * attributes.bytes + offset, 2);
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



struct LasWriter : public Writer {

	string path;
	Attributes outputAttributes;

	laszip_POINTER laszip_writer;
	laszip_point* point;
	laszip_header header;

	int64_t numWrittenPoints = 0;

	AABB aabb;

	mutex mtx_write;

	LasWriter(string path, dvec3 scale, dvec3 offset, Attributes outputAttributes) {
		this->path = path;
		this->outputAttributes = outputAttributes;

		laszip_create(&laszip_writer);

		memset(&header, 0, sizeof(laszip_header));

		header.version_major = 1;
		header.version_minor = 4;
		header.header_size = 375;
		header.offset_to_point_data = 375;
		header.point_data_format = 2;

		header.min_x = aabb.min.x;
		header.min_y = aabb.min.y;
		header.min_z = aabb.min.z;
		header.max_x = aabb.max.x;
		header.max_y = aabb.max.y;
		header.max_z = aabb.max.z;

		header.x_scale_factor = scale.x;
		header.y_scale_factor = scale.y;
		header.z_scale_factor = scale.z;

		header.x_offset = offset.x;
		header.y_offset = offset.y;
		header.z_offset = offset.z;

		header.point_data_record_length = 26;
		//header.number_of_point_records = 111; // must be updated at the end
		header.extended_number_of_point_records = 111;

		laszip_BOOL request_writer = 1;
		laszip_request_compatibility_mode(laszip_writer, request_writer);

		laszip_set_header(laszip_writer, &header);

		laszip_open_writer(laszip_writer, path.c_str(), true);

		laszip_get_point_pointer(laszip_writer, &point);
	}

	void write(Attributes& inputAttributes, Node* node, shared_ptr<Buffer> data, int64_t numAccepted, int64_t numRejected) {

		lock_guard<mutex> lock(mtx_write);

		auto handlers = createAttributeHandlers(&header, point, inputAttributes, outputAttributes);

		int64_t numPoints = data->size / inputAttributes.bytes;

		int posOffset = inputAttributes.getOffset("position");

		dvec3 scale = inputAttributes.posScale;
		dvec3 offset = inputAttributes.posOffset;

		for (int64_t i = 0; i < numPoints; i++) {

			int32_t X, Y, Z;

			memcpy(&X, data->data_u8 + i * inputAttributes.bytes + posOffset + 0, 4);
			memcpy(&Y, data->data_u8 + i * inputAttributes.bytes + posOffset + 4, 4);
			memcpy(&Z, data->data_u8 + i * inputAttributes.bytes + posOffset + 8, 4);

			double x = double(X) * scale.x + offset.x;
			double y = double(Y) * scale.y + offset.y;
			double z = double(Z) * scale.z + offset.z;

			aabb.expand(x, y, z);

			for (auto& handler : handlers) {
				handler(i, data->data_u8);
			}

			numWrittenPoints++;
			laszip_write_point(laszip_writer);
			laszip_update_inventory(laszip_writer);
		}

	}

	void close() {

		// update header
		//header.number_of_point_records = numWrittenPoints;
		header.extended_number_of_point_records = numWrittenPoints;

		laszip_set_header(laszip_writer, &header);

		laszip_update_inventory(laszip_writer);

		laszip_close_writer(laszip_writer);
	}

};