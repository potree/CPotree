
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
using std::ostream;

struct JsonWriter : public Writer {

	string path;
	Attributes outputAttributes;

	int64_t numWrittenPoints = 0;

	bool geojson;
	int projection;

	//AABB aabb;

	mutex mtx_write;

	shared_ptr<ostream> stream;

	JsonWriter(string path, Attributes outputAttributes, bool geojson = false, int projection = 4326) {
		this->path = path;
		this->outputAttributes = outputAttributes;
		this->geojson = geojson;
		this->projection = projection;

		if (path == "stdout") {
			stream = std::shared_ptr<ostream>(&cout, [](void*) {});
		}
		else {
			stream = make_shared<ofstream>(path);
		}

		auto digits = 7; // std::numeric_limits<double>::max_digits10;

		*stream << std::setprecision(digits);
		stream->setf(ios::fixed);

		if (geojson) {
			*stream << "{ \"type\": \"FeatureCollection\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"EPSG:" << projection << "\" } }, \"features\": ";
		}

		*stream << "[" << endl;
	}

	void write(Node* node, shared_ptr<Points> points, int64_t numAccepted, int64_t numRejected) {

		lock_guard<mutex> lock(mtx_write);

		auto inputAttributes = points->attributes;

		int64_t numPoints = points->numPoints;
		for (int64_t i = 0; i < numPoints; i++) {

			*stream << "{ ";
			bool first = true;

			if (geojson) {
				dvec3 xyz = points->getPosition(i);
				*stream << "\"type\": \"Feature\", \"geometry\": { \"type\": \"Point\", \"coordinates\": [";
				*stream << xyz.x << ", " << xyz.y << ", " << xyz.z;
				*stream << "] }, \"properties\": { ";
			}

			for (auto& attribute : outputAttributes.list) {
				if (geojson && attribute.name == "position") continue; // omit geometry from properties

				if (!first) {
					*stream << ", ";
				}

				*stream << "\"" << replaceAll(attribute.name, " ", "_") << "\": ";

				if (attribute.numElements > 1) {
					*stream << "[";
				}

				if (attribute.name == "position") {
					dvec3 xyz = points->getPosition(i);
					*stream << xyz.x << ", " << xyz.y << ", " << xyz.z;
				}
				else if (attribute.name == "position_projected_profile") { 
					auto i32 = points->attributeBuffersMap[attribute.name]->data_i32;
					dvec2 xy = glm::dvec2(
						i32[2 * i + 0] * points->attributes.posScale.x, // X -> projected on X-Y plane
						i32[2 * i + 1] * points->attributes.posScale.z	// Y -> Z
					);
					*stream << xy.x << ", " << xy.y;
				}
				else {
					
					for (int j = 0; j < attribute.numElements; j++) {

						if (attribute.type == AttributeType::INT8) {
							auto i8 = points->attributeBuffersMap[attribute.name]->data_i8;
							int8_t value = i8[attribute.numElements * i + j];
							*stream << int(value);
						}
						else if (attribute.type == AttributeType::INT16) {
							auto i16 = points->attributeBuffersMap[attribute.name]->data_i16;
							int16_t value = i16[attribute.numElements * i + j];
							*stream << int(value);
						}
						else if (attribute.type == AttributeType::INT32) {
							auto i32 = points->attributeBuffersMap[attribute.name]->data_i32;
							int32_t value = i32[attribute.numElements * i + j];
							*stream << int(value);
						}
						else if (attribute.type == AttributeType::INT64) {
							auto i64 = points->attributeBuffersMap[attribute.name]->data_i64;
							int64_t value = i64[attribute.numElements * i + j];
							*stream << int(value);
						}
						else if (attribute.type == AttributeType::UINT8) {
							auto u8 = points->attributeBuffersMap[attribute.name]->data_u8;
							uint8_t value = u8[attribute.numElements * i + j];
							*stream << unsigned(value);
						}
						else if (attribute.type == AttributeType::UINT16) {
							auto u16 = points->attributeBuffersMap[attribute.name]->data_u16;
							uint16_t value = u16[attribute.numElements * i + j];
							*stream << unsigned(value);
						}
						else if (attribute.type == AttributeType::UINT32) {
							auto u32 = points->attributeBuffersMap[attribute.name]->data_u32;
							uint32_t value = u32[attribute.numElements * i + j];
							*stream << unsigned(value);
						}
						else if (attribute.type == AttributeType::UINT64) {
							auto u64 = points->attributeBuffersMap[attribute.name]->data_u64;
							uint64_t value = u64[attribute.numElements * i + j];
							*stream << unsigned(value);
						}
						else if (attribute.type == AttributeType::FLOAT) {
							auto f32 = points->attributeBuffersMap[attribute.name]->data_f32;
							float value = f32[attribute.numElements * i + j];
							*stream << value;
						}
						else if (attribute.type == AttributeType::DOUBLE) {
							auto f64 = points->attributeBuffersMap[attribute.name]->data_f64;
							double value = f64[attribute.numElements * i + j];
							*stream << value;
						}
						else {
							*stream << "null";
						}

						if (j < attribute.numElements - 1) {
							*stream << ", ";
						}
					}
				}

				if (attribute.numElements > 1) {
					*stream << "]";
				}
				
				first = false;
			}

			if (geojson) {
				*stream << " }";
			}

			*stream << " }";

			// if (i < numPoints - 1) {} not woking because of multiple threads
			*stream << "," << endl; // next object
		}

	}

	void close() {
		*stream << "null" << endl; // remove last comma
		*stream << "]";

		if (geojson) {
			*stream << " }";
		}

		//stream->close();
		stream = nullptr;
	}

};