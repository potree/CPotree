
#pragma once

#include <vector>
#include "pmath.h"

using std::vector;

using glm::dvec2;
using glm::dvec3;
using glm::dvec4;
using glm::dmat4;

struct AreaMinMax {
	dvec3 min = { -Infinity, -Infinity, -Infinity };
	dvec3 max = { Infinity, Infinity, Infinity };
};

struct Profile {

	struct Segment {
		dvec3 start;
		dvec3 end;
		double length = 0.0;
		dmat4 proj;
	};

	vector<dvec3> points;
	double width = 0.0;
	vector<Segment> segments;

	void updateSegments() {
		for (int i = 0; i < points.size() - 1; i++) {
			auto start = points[i + 0];
			auto end = points[i + 1];

			dvec3 delta = end - start;
			double length = glm::length(delta);
			double angle = glm::atan(delta.y, delta.x);

			// project to x: along profile/segment, y: depth, and z: elevation
			dmat4 proj = glm::rotate(dmat4(), -angle, {0.0, 0.0, 1.0})
				* glm::translate(dmat4(), {-start.x, -start.y, 0.0});

			Segment segment;
			segment.start = start;
			segment.end = end;
			segment.length = glm::distance(start, end);
			segment.proj = proj;

			segments.push_back(segment);
		}
	}

	// see https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line#Line_defined_by_two_points
	// see three.js: https://github.com/mrdoob/three.js/blob/3292d6ddd99228be9c9bd152376cc0f5e0fbe489/src/math/Line3.js#L91
	bool inside(dvec3 point) {

		for (int i = 0; i < points.size() - 1; i++) {

			bool insideTestProj = false;
			
			auto segment = segments[i];
			auto projected = segment.proj * dvec4(point, 1.0);

			bool insideX = projected.x > 0.0 && projected.x < segment.length;
			bool insideDepth = projected.y >= -width / 2.0 && projected.y <= width / 2.0;
			bool inside = insideX && insideDepth;

			insideTestProj = inside;
			
			
			if (insideTestProj) {
				return true;
			}

		}

		return false;
	}

	bool intersects(AABB aabb) {

		for (int i = 0; i < points.size() - 1; i++) {
			auto p0 = points[i + 0];
			auto p1 = points[i + 1];
			auto cz = (aabb.min.z + aabb.max.z) / 2.0;

			dvec3 start = { p0.x, p0.y, cz };
			dvec3 end = { p1.x, p1.y, cz };
			dvec3 delta = end - start;
			double length = glm::length(delta);
			double angle = glm::atan(delta.y, delta.x);

			dvec3 size = { length, width, 2.0 * (aabb.max.z - aabb.min.z)};

			dmat4 box = glm::translate(dmat4(), start)
				* glm::rotate(dmat4(), angle, { 0.0, 0.0, 1.0 })
				* glm::scale(dmat4(), size)
				* glm::translate(dmat4(), { 0.5, 0.0, 0.0 });

			auto ob = OrientedBox(box);
			
			bool I = ob.intersects(aabb);

			if (I) {
				return true;
			}

		}

		return false;

	}
};


struct Area {
	vector<AreaMinMax> minmaxs;
	vector<OrientedBox> orientedBoxes;
	vector<Profile> profiles;
};

bool wtfTest() {
	return false;
}

bool intersects(Node* node, Area& area) {
	auto a = node->aabb;

	for (auto& b : area.minmaxs) {
		if (b.max.x < a.min.x || b.min.x > a.max.x ||
			b.max.y < a.min.y || b.min.y > a.max.y ||
			b.max.z < a.min.z || b.min.z > a.max.z) {

			continue;
		} else {
			return true;
		}
	}

	for (auto& box : area.orientedBoxes) {
		if (box.intersects(a)) {
			return true;
		}
	}

	for (auto& profile : area.profiles) {
		if (profile.intersects(a)) {
			return true;
		}
	}

	return false;
}

bool intersects(dvec3 point, Area& area) {

	for (auto minmax : area.minmaxs) {
		if (point.x < minmax.min.x || point.x > minmax.max.x) {
			continue;
		} else if (point.y < minmax.min.y || point.y > minmax.max.y) {
			continue;
		} else if (point.z < minmax.min.z || point.z > minmax.max.z) {
			continue;
		}

		return true;
	}

	for (auto& orientedBoxes : area.orientedBoxes) {
		if (orientedBoxes.inside(point)) {
			return true;
		}
	}

	for (auto& profile : area.profiles) {
		if (profile.inside(point)) {
			return true;
		}
	}

	return false;
}