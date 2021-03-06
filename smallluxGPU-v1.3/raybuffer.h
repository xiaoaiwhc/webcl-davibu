/***************************************************************************
 *   Copyright (C) 1998-2009 by David Bucciarelli (davibu@interfree.it)    *
 *                                                                         *
 *   This file is part of SmallLuxGPU.                                     *
 *                                                                         *
 *   SmallLuxGPU is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *  SmallLuxGPU is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   and Lux Renderer website : http://www.luxrender.net                   *
 ***************************************************************************/

#ifndef _RAYBUFFER_H
#define	_RAYBUFFER_H

//#include <boost/thread/mutex.hpp>
//#include <boost/thread/condition_variable.hpp>

#include <vector>
#include <deque>
#include "ray.h"

class PathIntegrator;

// Must be a multiple of 1024 (for AMD CPU device) and 64 (for most GPUs)
#define RAY_BUFFER_SIZE (65536)

typedef struct {
	float t;
	float b1, b2; // Barycentric coordinates of the hit point
	unsigned int index;
} RayHit;

class RayBuffer {
public:
	RayBuffer(const size_t bufferSize) : size(bufferSize), currentFreeRayIndex(0) {
		rays = new Ray[size];
		rayHits = new RayHit[size];
	}

	~RayBuffer() {
		delete rays;
		delete rayHits;
	}

	void PushUserData(size_t data) {
		userData.push_back(data);
	}

	size_t GetUserData() {
		return userData.back();
	}

	size_t PopUserData() {
		size_t data = userData.back();
		userData.pop_back();

		return data;
	}

	size_t GetUserDataCount() {
		return userData.size();
	}

	void ResetUserData() {
		userData.clear();
	}

	void Reset() {
		currentFreeRayIndex = 0;
	}

	size_t ReserveRay() {
		return currentFreeRayIndex++;
	}

	size_t AddRay(const Ray &ray) {
		rays[currentFreeRayIndex] = ray;

		return currentFreeRayIndex++;
	}

	const RayHit *GetRayHit(const size_t index) const {
		return &rayHits[index];
	}

	size_t GetSize() const {
		return size;
	}

	size_t GetRayCount() const {
		return currentFreeRayIndex;
	}

	bool IsFull() {
		return (currentFreeRayIndex >= size);
	}

	size_t LeftSpace() {
		return size - currentFreeRayIndex;
	}

	Ray *GetRayBuffer() const {
		return rays;
	}

	RayHit *GetHitBuffer() {
		return rayHits;
	}

private:
	size_t size;
	size_t currentFreeRayIndex;
	vector<size_t> userData;

	Ray *rays;
	RayHit *rayHits;
};

// NOTE: this class must be thread safe
class RayBufferQueue {
public:

	RayBufferQueue() {
	}

	~RayBufferQueue() {
	}

	void Clear() {
		//boost::unique_lock<boost::mutex> lock(queueMutex);

		queue.clear();
	}

	size_t Size() {
		//boost::unique_lock<boost::mutex> lock(queueMutex);

		return queue.size();
	}

	void Push(RayBuffer *rayBuffer) {
		{
			//boost::unique_lock<boost::mutex> lock(queueMutex);
			queue.push_back(rayBuffer);
		}

		//condition.notify_all();
	}

	RayBuffer *Pop() {
		//boost::unique_lock<boost::mutex> lock(queueMutex);

		//while (queue.size() < 1) {
			// Wait for a new buffer to arrive
			//condition.wait(lock);
		//}

		RayBuffer *rayBuffer = queue.front();
		queue.pop_front();
		return rayBuffer;
	}

private:
	//boost::mutex queueMutex;
	//boost::condition_variable condition;

	std::deque<RayBuffer *> queue;
};

#endif	/* _RAYBUFFER_H */
