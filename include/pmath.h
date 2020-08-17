
#pragma once


#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/constants.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include "unsuck/unsuck.hpp"

using glm::dvec2;
using glm::dvec3;
using glm::dvec4;
using glm::dmat4;

dvec3 projectPoint(dvec3 point, dvec3 normal) {

	//dvec3 np = point - normal;
	double d = glm::dot(point, normal);
	dvec3 projected = point - normal * d;

	return projected;
}

struct AABB {
	dvec3 min = { Infinity, Infinity, Infinity };
	dvec3 max = { -Infinity, -Infinity, -Infinity };

	AABB() {

	}

	AABB(dvec3 min, dvec3 max) {
		this->min = min;
		this->max = max;
	}

	dvec3 size() {
		return max - min;
	}

	void expand(double x, double y, double z) {
		this->min.x = std::min(x, this->min.x);
		this->min.y = std::min(y, this->min.y);
		this->min.z = std::min(z, this->min.z);
		this->max.x = std::max(x, this->max.x);
		this->max.y = std::max(y, this->max.y);
		this->max.z = std::max(z, this->max.z);
	}

	vector<dvec3> vertices() {

		vector<dvec3> v = {
			{min.x, min.y, min.z},
			{min.x, min.y, max.z},
			{min.x, max.y, min.z},
			{min.x, max.y, max.z},
			{max.x, min.y, min.z},
			{max.x, min.y, max.z},
			{max.x, max.y, min.z},
			{max.x, max.y, max.z}
		};

		return v;
	}


};

AABB childAABB(AABB& aabb, int& index) {

	dvec3 min = aabb.min;
	dvec3 max = aabb.max;

	auto size = aabb.size();

	if ((index & 0b0001) > 0) {
		min.z += size.z / 2;
	} else {
		max.z -= size.z / 2;
	}

	if ((index & 0b0010) > 0) {
		min.y += size.y / 2;
	} else {
		max.y -= size.y / 2;
	}

	if ((index & 0b0100) > 0) {
		min.x += size.x / 2;
	} else {
		max.x -= size.x / 2;
	}

	return { min, max };
}

struct OrientedBox {
	dmat4 box;
	dmat4 boxInverse;
	vector<dvec3> vertices;
	vector<dvec3> axes;
	vector<vector<dvec3>> projections;
	vector<dvec2> projectedIntervalls;


	OrientedBox(dmat4 box) {
		this->box = box;
		this->boxInverse = glm::inverse(box);

		// INITIALIZE IN LOCAL SPACE
		vertices = {
			{-0.5, -0.5, -0.5},
			{-0.5, -0.5, +0.5},
			{-0.5, +0.5, -0.5},
			{-0.5, +0.5, +0.5},
			{+0.5, -0.5, -0.5},
			{+0.5, -0.5, +0.5},
			{+0.5, +0.5, -0.5},
			{+0.5, +0.5, +0.5}
		};

		axes = {
			{1, 0, 0},
			{0, 1, 0},
			{0, 0, 1}
		};

		// TRANSFORM TO WORLD SPACE
		for (int i = 0; i < vertices.size(); i++) {
			vertices[i] = box * glm::dvec4(vertices[i], 1.0);
		}

		for (int i = 0; i < axes.size(); i++) {
			dvec3 tOrigin = box * dvec4(0.0, 0.0, 0.0, 1.0);
			dvec3 tAxe = box * dvec4(axes[i], 1.0);
			axes[i] = glm::normalize(tAxe - tOrigin);
		}

		projections = {
			{axes[0], axes[1], axes[2]},
			{axes[1], axes[2], axes[0]},
			{axes[2], axes[0], axes[1]},
			{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
			{{0, 1, 0}, {0, 0, 1}, {1, 0, 0}},
			{{0, 0, 1}, {1, 0, 0}, {0, 1, 0}}
		};

		for (int i = 0; i < projections.size(); i++) {

			dvec2 intervall = { Infinity, -Infinity };

			for (auto& vertex : vertices) {
				auto& proj = projections[i];
				auto pr = projectPoint(projectPoint(vertex, proj[0]), proj[1]);
				double pi = glm::dot(pr, proj[2]);

				intervall[0] = std::min(intervall[0], pi);
				intervall[1] = std::max(intervall[1], pi);
			}

			projectedIntervalls.push_back(intervall);
		}

	}

	bool intersects(AABB& aabb) {
		vector<dvec2> projectedIntervallsAABB;

		for (int i = 0; i < projections.size(); i++) {

			dvec2 intervall = { Infinity, -Infinity };

			for (auto& vertex : aabb.vertices()) {
				auto& proj = projections[i];
				auto pr = projectPoint(projectPoint(vertex, proj[0]), proj[1]);
				double pi = glm::dot(pr, proj[2]);

				intervall[0] = std::min(intervall[0], pi);
				intervall[1] = std::max(intervall[1], pi);
			}

			projectedIntervallsAABB.push_back(intervall);
		}

		for (int i = 0; i < projectedIntervalls.size(); i++) {
			auto piOBB = projectedIntervalls[i];
			auto piAABB = projectedIntervallsAABB[i];

			if ((piAABB[1] < piOBB[0]) || (piAABB[0] > piOBB[1])) {
				// found a gap at one of the projections => no intersection
				return false;
			}

		}

		return true;
	}

	bool inside(dvec3& point) {
		auto p = boxInverse * dvec4(point, 1);

		bool inX = -0.5 <= p.x && p.x <= 0.5;
		bool inY = -0.5 <= p.y && p.y <= 0.5;
		bool inZ = -0.5 <= p.z && p.z <= 0.5;

		return inX && inY && inZ;
	}


};