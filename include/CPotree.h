#pragma once

#include <execution>
#include <thread>
#include <mutex>

#include "pmath.h"
#include "PotreeLoader.h"

using std::mutex;
using std::lock_guard;

struct Stats{
	AABB aabb;
	dvec3 minScale = { Infinity, Infinity, Infinity };

};


Stats computeStats(vector<string> sources){

	Stats stats;
	mutex mtx_stats;

	auto parallel = std::execution::par_unseq;
	for_each(parallel, sources.begin(), sources.end(), [&stats, &mtx_stats](string source) {

		auto loader = PotreeLoader::load(source);

		auto metadata = loader.metadata;

		lock_guard<mutex> lock(mtx_stats);

		stats.aabb.expand(metadata.aabb);

		stats.minScale.x = std::min(stats.minScale.x, metadata.scale.x);
		stats.minScale.y = std::min(stats.minScale.y, metadata.scale.y);
		stats.minScale.z = std::min(stats.minScale.z, metadata.scale.z);

	});

	return stats;
};

struct ScaleOffset {
	dvec3 scale;
	dvec3 offset;
};


ScaleOffset computeScaleOffset(AABB aabb, dvec3 targetScale) {
	
	dvec3 offset = aabb.center();
	dvec3 scale = targetScale;
	dvec3 size = aabb.size();

	double min_scale_x = size.x / pow(2.0, 31.0);
	double min_scale_y = size.y / pow(2.0, 31.0);
	double min_scale_z = size.z / pow(2.0, 31.0);

	scale.x = std::max(scale.x, min_scale_x);
	scale.y = std::max(scale.y, min_scale_y);
	scale.z = std::max(scale.z, min_scale_z);

	ScaleOffset scaleOffset;
	scaleOffset.scale = scale;
	scaleOffset.offset = offset;

	return scaleOffset;
}


