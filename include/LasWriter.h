
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


vector<function<void(int64_t)>> createAttributeHandlers(laszip_header* header, laszip_point* point, shared_ptr<Points> points, Attributes outputAttributes) {

	vector<function<void(int64_t)>> handlers;

	auto& inputAttributes = points->attributes;

	{ // STANDARD LAS ATTRIBUTES

		unordered_map<string, function<void(int64_t)>> mapping;

		{ // POSITION
			auto attribute = inputAttributes.get("position");
			auto buff_position = points->attributeBuffersMap["position"];
			auto posScale = inputAttributes.posScale;
			auto posOffset = inputAttributes.posOffset;
			auto handler = [header, point, buff_position, attribute, posScale, posOffset](int64_t index) {

				int32_t X, Y, Z;

				memcpy(&X, buff_position->data_u8 + index * attribute->size + 0, 4);
				memcpy(&Y, buff_position->data_u8 + index * attribute->size + 4, 4);
				memcpy(&Z, buff_position->data_u8 + index * attribute->size + 8, 4);

				double x = double(X) * posScale.x + posOffset.x;
				double y = double(Y) * posScale.y + posOffset.y;
				double z = double(Z) * posScale.z + posOffset.z;

				point->X = (x - header->x_offset) / header->x_scale_factor;
				point->Y = (y - header->y_offset) / header->y_scale_factor;
				point->Z = (z - header->z_offset) / header->z_scale_factor;
			};

			mapping["position"] = handler;
		}

		{ // RGB
			auto attribute = inputAttributes.get("rgb");
			auto source = points->attributeBuffersMap["rgb"];
			auto handler = [point, source, attribute](int64_t index) {
				auto& rgba = point->rgb;

				if(source != nullptr){
					memcpy(&rgba, source->data_u8 + index * attribute->size, 6);
				}else{
					memset(&rgba, 0, 4 * sizeof(laszip_U16));
				}

			};

			mapping["rgb"] = handler;
		}

		{ // INTENSITY
			auto attribute = inputAttributes.get("intensity");
			auto source = points->attributeBuffersMap["intensity"];
			auto handler = [point, source, attribute](int64_t index) {
			
				if(source != nullptr){
					memcpy(&point->intensity, source->data_u8 + index * attribute->size, 2);
				}else{
					memset(&point->intensity, 0, sizeof(point->intensity));
				}

			};

			mapping["intensity"] = handler;
		}

		{ // CLASSIFICATION
			auto attribute = inputAttributes.get("classification");
			auto source = points->attributeBuffersMap["classification"];
			auto handler = [point, source, attribute](int64_t index) {
				uint8_t classification;

				if(source != nullptr){
					memcpy(&classification, source->data_u8 + index * attribute->size, 1);
				}else{
					classification = 0;
				}

				point->classification = classification;
			};

			mapping["classification"] = handler;
		}

		{ // RETURN NUMBER
			auto attribute = inputAttributes.get("return number");
			auto source = points->attributeBuffersMap["return number"];
			auto handler = [point, source, attribute](int64_t index) {
				uint8_t returnNumber;
				if (source != nullptr) {
					memcpy(&returnNumber, source->data_u8 + index * attribute->size, 1);
				}
				else {
					returnNumber = 0;
				}
				point->return_number = returnNumber;
			};
			mapping["return number"] = handler;
		}

		{ // NUMBER OF RETURNS
			auto attribute = inputAttributes.get("number of returns");
			auto source = points->attributeBuffersMap["number of returns"];
			auto handler = [point, source, attribute](int64_t index) {
				uint8_t numberofReturns;
				if (source != nullptr) {
					memcpy(&numberofReturns, source->data_u8 + index * attribute->size, 1);
				}
				else {
					numberofReturns = 0;
				}
				point->number_of_returns = numberofReturns;
			};
			mapping["number of returns"] = handler;
		}

		{ // USER DATA 
			auto attribute = inputAttributes.get("user data");
			auto source = points->attributeBuffersMap["user data"];
			auto handler = [point, source, attribute](int64_t index) {
				uint8_t userdata;
				if (source != nullptr) {
					memcpy(&userdata, source->data_u8 + index * attribute->size, 1);
				}
				else {
					userdata = 0;
				}
				point->user_data = userdata;
			};
			mapping["user data"] = handler;
		}

		{ // POINT SOURCE ID
			auto attribute = inputAttributes.get("point source id");
			auto source = points->attributeBuffersMap["point source id"];
			auto handler = [point, source, attribute](int64_t index) {
				if (source != nullptr) {
					memcpy(&point->point_source_ID, source->data_u8 + index * attribute->size, 2);
				}
				else {
					memset(&point->point_source_ID, 0, sizeof(point->point_source_ID));
				}
			};
			mapping["point source id"] = handler;
		}

		{ //GPS-TIME
			auto attribute = inputAttributes.get("gps-time");
			auto source = points->attributeBuffersMap["gps-time"];
			auto handler = [point, source, attribute](int64_t index) {
				double gpstime;
				if (source != nullptr) {
					memcpy(&gpstime, source->data_f64, 8);
				}
				else {
					gpstime = 0.00000;
				}
				point->gps_time = gpstime;
			};
			mapping["gps-time"] = handler;
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
		header.point_data_format = 3;

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

		header.point_data_record_length = 34;
		//header.number_of_point_records = 111; // must be updated at the end
		header.extended_number_of_point_records = 0;

		
		laszip_BOOL compress = path.ends_with(".laz") || path.ends_with(".LAZ");
		laszip_BOOL request_writer = 1;
		laszip_request_compatibility_mode(laszip_writer, request_writer);
		//laszip_set_chunk_size(laszip_writer, 50'000);

		laszip_set_header(laszip_writer, &header);

		laszip_open_writer(laszip_writer, path.c_str(), compress);

		laszip_get_point_pointer(laszip_writer, &point);
	}

	void write(Node* node, shared_ptr<Points> points, int64_t numAccepted, int64_t numRejected) {

		lock_guard<mutex> lock(mtx_write);

		auto& inputAttributes = points->attributes;
		auto handlers = createAttributeHandlers(&header, point, points, outputAttributes);

		int64_t numPoints = points->numPoints;

		dvec3 scale = inputAttributes.posScale;
		dvec3 offset = inputAttributes.posOffset;

		auto buff_position = points->attributeBuffersMap["position"];

		for (int64_t i = 0; i < numPoints; i++) {

			int32_t X, Y, Z;

			memcpy(&X, buff_position->data_u8 + i * 12 + 0, 4);
			memcpy(&Y, buff_position->data_u8 + i * 12 + 4, 4);
			memcpy(&Z, buff_position->data_u8 + i * 12 + 8, 4);

			double x = double(X) * scale.x + offset.x;
			double y = double(Y) * scale.y + offset.y;
			double z = double(Z) * scale.z + offset.z;

			aabb.expand(x, y, z);

			for (auto& handler : handlers) {
				handler(i);
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

		//laszip_update_inventory(laszip_writer);

		laszip_close_writer(laszip_writer);
	}

};
