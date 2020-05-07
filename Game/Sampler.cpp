#include "pch.h"
#include <glm/glm.hpp>

#define NOISE_STATIC 1
#include <noise/noise.h>
#include <algorithm>
#include <future>
#include <thread>
#include <assert.h>

#include "Sampler.h"

namespace Sampler
{

int index3D(int x, int y, int z, int dimensionLength)
{
	return x + dimensionLength * (y + dimensionLength * z);
}
int index3D(glm::ivec3 coord, int dimensionLength)
{
	return index3D(coord.x, coord.y, coord.z, dimensionLength);
}

float mod(float x, float y)
{
	return x - y * floor(x / y);
}
float repeatAxis(float p, float c)
{
	return mod(p, c) - 0.5f * c;
}
float Sphere(const glm::vec3& worldPosition, const glm::vec3& origin, float radius)
{
	float val = glm::length(worldPosition - origin) - radius; // non repeating
	return std::clamp(val, -1.f, 1.f);
}

float Box(const glm::vec3& p, const glm::vec3& size)
{
	glm::vec3 d = abs(p) - size;
	glm::vec3 maxed(0);
	for (int i = 0; i < 3; i++)
	{
		maxed[i] = std::max(d[i], 0.f);
	}
	return glm::length(maxed) + std::min(std::max(d.x, std::max(d.y, d.z)), 0.f);
}

float ensureZeroCrossing(float originalValue, const glm::vec3& worldPosition, unsigned int size)
{
	float maxLen = size / 2;
	glm::vec3 center(maxLen, maxLen, maxLen);
	float fraction = glm::length(center - worldPosition) / maxLen;
	fraction = fraction > 1 ? 1 : fraction;

	float invBorderDistance = 1 - fraction;

	float target = 2000.1;
	float targetSign = originalValue > 0 ? -target : target; // target a little past zero so we don't hit zero crossing exactly at border but a bit before

	targetSign *= fraction;

	float result = originalValue * invBorderDistance + targetSign;
	return result;
}

float BumpySphere(const glm::vec3& worldPosition, const glm::vec3& origin, float radius, unsigned int size)
{
	glm::vec3 noisePos = glm::vec3(worldPosition.x, worldPosition.y * 6, worldPosition.z);
	float val = glm::length(worldPosition - origin) - radius + 4 * Noise(noisePos); 
	val = std::clamp(val, -1.f, 1.f);

	val = ensureZeroCrossing(val, worldPosition, size);
	return val;
}

float Noise(const glm::vec3& p)
{
	static noise::module::Perlin noiseModule;
	double epsilon = 0.500f;
	float divider = 13.f;
	float value = (float)noiseModule.GetValue(p.x / divider  + epsilon, p.y / divider + epsilon, p.z / divider + epsilon);

	divider = 100.f;
	value += (float)noiseModule.GetValue(p.x / divider  + epsilon, p.y / divider + epsilon, p.z / divider + epsilon);

	divider = 400.f;
	value += (float)noiseModule.GetValue(p.x / divider  + epsilon, p.y / divider + epsilon, p.z / divider + epsilon);

	//if (p.y > 20.f)
	//{
	//	divider = 200.f;
	//	value -= (float)noiseModule.GetValue(p.x / divider  + epsilon, p.y / divider + epsilon, p.z / divider + epsilon);
	//	value -= 1.0f;
	//}
	//if (p.y > 40.f)
	//{
	//	value += 1.5f;
	//}
	return value;
}

float Waves(const glm::vec3& p)
{
	//printf("density at (%f, %f, %f) is = %f\n", p.x, p.y, p.z, value);
	//std::cout << "position: " << p.x << std::endl;
	//printf("position %f %f %f\n", p.x, p.y, p.z);
	//return sin(p.x * 1.0) + cos(p.y * 1.0) + p.z - 2;

	//float value = sin(p.x * 1.0f) / 1.f + cos(p.y * 1.0f) / 1.f + p.z - 5.50f;
	float value = sin(p.x * 0.5f) / 0.3f + p.y - 5.50f;

	//return sin(p.x) + cos(p.z) + p.y;
	return value;
}

float Plane(const glm::vec3& p)
{
	return p.x - 0.00001f *p.y;
}

// Size parameter instead of max coordinate to enforce same size in all coordinate axes for easier indexing
std::vector<float> AsyncCache(glm::ivec3 min, int segmentStart, int sampleCount, int size)
{
	std::vector<float> samples(sampleCount);
	/*
	for (int x = min.x; x < min.x + range; x++)
	{
		for (int y = min.y; y < min.y + range; y++)
		{
			for (int z = min.z; z < min.z + range; z++)
			{
				int index = index3D(x - min.x, y - min.y, z - min.z, range);
				samples[index] = Noise(glm::vec3(x, y, z), noiseModule);
			}
		}
	}
	*/

	for (int i = 0; i < sampleCount; i++)
	{
		int idx = i + segmentStart;
		int x = idx % size;
		int y = (idx / size) % size;
		int z = idx / (size * size);
		//samples[i] = Noise(min + glm::ivec3(x, y, z), noiseModule);
		samples[i] = Density(min + glm::ivec3(x, y, z), size);
	}
	return samples;
}
std::vector<std::vector<float>> BuildCache(const glm::ivec3 min, const unsigned size)
{
	unsigned idxCount = size * size * size;
	unsigned concurrentThreadsSupported = std::thread::hardware_concurrency();

	// perf-wise no sense in computing less than thousands of samples per thread, but for debugging it's nice
	unsigned threads = std::min(concurrentThreadsSupported, idxCount / 8); 

	unsigned samplesPerSegment = idxCount / threads;

	unsigned extras = size % threads;
	//assert(size % threads == 0);

	std::vector<std::future<std::vector<float>>> futureSamples;
	std::vector<std::vector<float>> results;
	for (unsigned i = 0; i < threads; i++)
	{
		int segmentStart = i * (idxCount / threads);
		futureSamples.push_back(std::async(
			std::launch::async,
			AsyncCache,
			min,
			segmentStart,
			i < threads-1 ? samplesPerSegment : samplesPerSegment + extras,
			size));
	}

	for (auto& future : futureSamples)
	{
		results.push_back(future.get());
	}

	return results;
}

float SampleCache(const std::vector<std::vector<float>>& cache, const int coordinate)
{
	int cacheCount = cache.size();
	int cacheLength = cache[0].size();
	int cacheIdx = coordinate / cacheLength;
	cacheIdx = cacheIdx >= cacheCount ? cacheCount - 1 : cacheIdx;
	int idx = coordinate % cacheLength;
	return cache[cacheIdx][idx];
}

float SampleCache(const std::vector<std::vector<float>>& cache, const glm::ivec3 min, const int size, const glm::ivec3 coordinate)
{
	//printf("pos %i %i %i \n", coordinate.x, coordinate.y, coordinate.z);
	return SampleCache(cache, index3D(coordinate - min, size));
}

float Density(const glm::vec3 pos, const unsigned int size)
{
	//static noise::module::Perlin noiseModule;
	//printf("density %f %f %f \n", pos.x, pos.y, pos.z);

	//glm::vec3 repeat(15, 15, 15);
	//glm::vec3 repeatPos(
	//	repeatAxis(pos.x, repeat.x),
	//	repeatAxis(pos.y, repeat.y),
	//	repeatAxis(pos.z, repeat.z)
	//);

	//return glm::length(pos - origin) - radius; // repeating
	//return Noise(pos, noiseModule);
	return BumpySphere(pos, glm::vec3(8, 8, 8), 4.0, size);

	//return Box(pos - glm::vec3(16,16,16), glm::vec3(128, 8, 8));
	//return Box(repeatPos - glm::vec3(0,0,0), glm::vec3(5, 5, 5));

	//return Plane(pos);
	//return Waves(pos);
}

float Sample(const glm::vec3 pos, const unsigned int size)
{
	float value = Density(pos, size);
	//printf("position (%f %f %f) value = %f \n", pos.x, pos.y, pos.z, value);
	//return value >= 0.0f;
	return value;
}


}