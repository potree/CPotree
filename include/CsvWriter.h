
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

struct CsvWriter : public Writer {

	string path;
	Attributes outputAttributes;

	int64_t numWrittenPoints = 0;

	//AABB aabb;

	string delim;
	string value_delim;

	mutex mtx_write;

	shared_ptr<ostream> stream;

	CsvWriter(string path, Attributes outputAttributes, string delim = ",", string value_delim = ",") {
		this->path = path;
		this->outputAttributes = outputAttributes;
		this->delim = delim;
		this->value_delim = value_delim;

		if (path == "stdout") {
			stream = std::shared_ptr<ostream>(&cout, [](void*) {});
		}
		else {
			stream = make_shared<ofstream>(path);
		}

		auto digits = 7; // std::numeric_limits<double>::max_digits10;

		*stream << std::setprecision(digits);
		stream->setf(ios::fixed);

		// create header
		for (int i = 0; i < outputAttributes.list.size(); i++) {
			auto attribute = outputAttributes.list[i];

			if (value_delim != delim) {
				*stream << "\"" << attribute.name << "\"";
			}
			else { // extract attribute values as columns
				if (attribute.name == "position") {
					*stream << "x" << value_delim << "y" << value_delim << "z";
				}
				else if (attribute.name == "position_projected_profile") {
					*stream << "u" << value_delim << "v";
				}
				else if (attribute.name == "rgb") {
					*stream << "r" << value_delim << "g" << value_delim << "b";
				}
				else {
					for (int i = 0; i < attribute.numElements; i++) {
						*stream << "\"" << attribute.name << (attribute.numElements > 1 ?  " " + (i + 1) : "") << "\"";
					}
				}
			}

			if (i < outputAttributes.list.size() - 1) {
				*stream << delim;
			}
		}
	}

	void write(Node* node, shared_ptr<Points> points, int64_t numAccepted, int64_t numRejected) {

		lock_guard<mutex> lock(mtx_write);

		auto inputAttributes = points->attributes;

		int64_t numPoints = points->numPoints;
		for (int64_t i = 0; i < numPoints; i++) {
			*stream << endl; // start new line

			bool first = true;

			for (auto& attribute : outputAttributes.list) {
				if (!first) {
					*stream << delim; // attribute delimiter
				}

				if (value_delim != delim && attribute.numElements > 1) {
					*stream << "\"";
				}

				if (attribute.name == "position") {
					dvec3 xyz = points->getPosition(i);
					*stream << xyz.x << value_delim << xyz.y << value_delim << xyz.z;
				}
				else if (attribute.name == "position_projected_profile") {
					auto i32 = points->attributeBuffersMap[attribute.name]->data_i32;
					dvec2 xy = glm::dvec2(
						i32[2 * i + 0] * points->attributes.posScale.x, // X -> projected on X-Y plane
						i32[2 * i + 1] * points->attributes.posScale.z	// Y -> Z
					);
					*stream << xy.x << value_delim << xy.y;
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
							*stream << "";
						}

						if (j < attribute.numElements - 1) {
							*stream << value_delim; // multi attribute delimiter
						}
					}
				}

				if (value_delim != delim && attribute.numElements > 1) {
					*stream << "\"";
				}

				first = false;
			}

		}

	}

	void close() {
		//stream->close();
		stream = nullptr;
	}

};