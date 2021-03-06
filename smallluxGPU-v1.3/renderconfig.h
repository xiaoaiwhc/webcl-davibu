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

#ifndef _RENDERCONFIG_H
#define	_RENDERCONFIG_H

#include "scene.h"
#include "intersectiondevice.h"
#include "renderthread.h"

class RenderingConfig {
public:
	RenderingConfig(const bool lowLatency, const string &sceneFileName, const unsigned int w,
		const unsigned int h, const unsigned int nativeThreadCount,
		const bool useCPUs, const bool useGPUs,
		const unsigned int forceGPUWorkSize) {
		screenRefreshInterval = 100;

		Init(lowLatency, sceneFileName, w, h, nativeThreadCount, useCPUs, useGPUs,
			forceGPUWorkSize, 3);
	}

	RenderingConfig(const string &fileName) {
		cfg.insert(make_pair("image.width", "640"));
		cfg.insert(make_pair("image.height", "480"));
		cfg.insert(make_pair("batch.halttime", "0"));
		cfg.insert(make_pair("scene.file", "scenes/luxball.scn"));
		cfg.insert(make_pair("scene.fieldofview", "45"));
		cfg.insert(make_pair("opencl.latency.mode", "0"));
		cfg.insert(make_pair("opencl.nativethread.count", "0"));
		cfg.insert(make_pair("opencl.renderthread.count", "4"));
		cfg.insert(make_pair("opencl.cpu.use", "0"));
		cfg.insert(make_pair("opencl.gpu.use", "1"));
		cfg.insert(make_pair("opencl.gpu.workgroup.size", "64"));
		cfg.insert(make_pair("opencl.platform.index", "0"));
		cfg.insert(make_pair("opencl.devices.select", ""));
		cfg.insert(make_pair("opencl.devices.threads", "")); // Initialized when the GPU count is known
		cfg.insert(make_pair("screen.refresh.interval", "100"));
		cfg.insert(make_pair("screen.type", "3"));
		cfg.insert(make_pair("path.maxdepth", "3"));
		cfg.insert(make_pair("path.shadowrays", "1"));

		cerr << "Reading configuration file: " << fileName << endl;

		ifstream file(fileName.c_str(), ios::in);
		char buf[512], key[512], value[512];
		for (;;) {
			file.getline(buf, 512);
			if (file.eof())
				break;
			// Ignore comments
			if (buf[0] == '#')
				continue;

			const int count = sscanf(buf, "%s = %s", key, value);
			if (count != 2) {
				cerr << "Ingoring line in configuration file: [" << buf << "]" << endl;
				continue;
			}

			// Check if it is a valid key
			string skey(key);
			if (cfg.count(skey) != 1)
				cerr << "Ingoring unknown key in configuration file:  [" << skey << "]" << endl;
			else {
				cfg.erase(skey);
				cfg.insert(make_pair(skey, string(value)));
			}
		}

		cerr << "Configuration: " << endl;
		for (map<string, string>::iterator i = cfg.begin(); i != cfg.end(); ++i)
			cerr << "  " << i->first << " = " << i->second << endl;
	}

	~RenderingConfig() {
		for (size_t i = 0; i < renderThreads.size(); ++i)
			delete renderThreads[i];

		if (m2oDevice)
			delete m2oDevice;
		if (o2mDevice)
			delete o2mDevice;

		for (size_t i = 0; i < intersectionAllDevices.size(); ++i)
			delete intersectionAllDevices[i];

		delete scene;
		delete film;
	}

	void Init() {
		const bool lowLatency = (atoi(cfg.find("opencl.latency.mode")->second.c_str()) == 1);
		const string sceneFileName = cfg.find("scene.file")->second.c_str();
		const unsigned int w = atoi(cfg.find("image.width")->second.c_str());
		const unsigned int h = atoi(cfg.find("image.height")->second.c_str());
		const unsigned int nativeThreadCount = atoi(cfg.find("opencl.nativethread.count")->second.c_str());
		const bool useCPUs = (atoi(cfg.find("opencl.cpu.use")->second.c_str()) == 1);
		const bool useGPUs = (atoi(cfg.find("opencl.gpu.use")->second.c_str()) == 1);
		const unsigned int forceGPUWorkSize = atoi(cfg.find("opencl.gpu.workgroup.size")->second.c_str());
		const unsigned int filmType = atoi(cfg.find("screen.type")->second.c_str());
		const unsigned int oclPlatformIndex = atoi(cfg.find("opencl.platform.index")->second.c_str());
		const string oclDeviceConfig = cfg.find("opencl.devices.select")->second;
		const string oclDeviceThreads = cfg.find("opencl.devices.threads")->second;

		screenRefreshInterval = atoi(cfg.find("screen.refresh.interval")->second.c_str());

		Init(lowLatency, sceneFileName, w, h, nativeThreadCount,
			useCPUs, useGPUs, forceGPUWorkSize, filmType,
			oclPlatformIndex, oclDeviceThreads, oclDeviceConfig);

		StopAllDevice();
		for (size_t i = 0; i < renderThreads.size(); ++i)
			renderThreads[i]->ClearPaths();

		scene->camera->fieldOfView = atof(cfg.find("scene.fieldofview")->second.c_str());
		scene->maxPathDepth = atoi(cfg.find("path.maxdepth")->second.c_str());
		scene->shadowRayCount = atoi(cfg.find("path.shadowrays")->second.c_str());
		StartAllDevice();
	}

	void Init(const bool lowLatency, const string &sceneFileName, const unsigned int w,
		const unsigned int h, const unsigned int nativeThreadCount,
		const bool useCPUs, const bool useGPUs,
		const unsigned int forceGPUWorkSize, const unsigned int filmType,
		const unsigned int oclPlatformIndex = 0,
		const string &oclDeviceThreads = "", const string &oclDeviceConfig = "") {

		captionBuffer[0] = '\0';

		SetUpOpenCLPlatform(oclPlatformIndex);

		// Create the scene
		switch (filmType) {
			case 0:
				cerr << "Film type: StandardFilm" << endl;
				film = new StandardFilm(lowLatency, w, h);
				break;
			case 1:
				cerr << "Film type: BluredStandardFilm" << endl;
				film = new BluredStandardFilm(lowLatency, w, h);
				break;
			case 2:
				cerr << "Film type: GaussianFilm" << endl;
				film = new GaussianFilm(lowLatency, w, h);
				break;
			case 3:
				cerr << "Film type: FastGaussianFilm" << endl;
				film = new FastGaussianFilm(lowLatency, w, h);
				break;
			default:
				throw runtime_error("Requested an unknown film type");
		}
		scene = new Scene(lowLatency, sceneFileName, film);

		// Start OpenCL devices
		SetUpOpenCLDevices(lowLatency, useCPUs, useGPUs, forceGPUWorkSize, oclDeviceConfig);

		// Start Native threads
		for (unsigned int i = 0; i < nativeThreadCount; ++i) {
			NativeIntersectionDevice *device = new NativeIntersectionDevice(scene, lowLatency, i);
			intersectionCPUDevices.push_back(device);
		}

		const size_t deviceCount = intersectionCPUDevices.size()  + intersectionGPUDevices.size();
		if (deviceCount <= 0)
			throw runtime_error("Unable to find any appropiate IntersectionDevice");

		intersectionAllDevices.resize(deviceCount);
		if (intersectionGPUDevices.size() > 0)
			copy(intersectionGPUDevices.begin(), intersectionGPUDevices.end(),
					intersectionAllDevices.begin());
		if (intersectionCPUDevices.size() > 0)
			copy(intersectionCPUDevices.begin(), intersectionCPUDevices.end(),
					intersectionAllDevices.begin() + intersectionGPUDevices.size());

		// Create and start render threads
		const size_t gpuRenderThreadCount = ((oclDeviceThreads.length() == 0) || (intersectionGPUDevices.size() == 0)) ?
			(2 * intersectionGPUDevices.size()) : atoi(oclDeviceThreads.c_str());
		size_t renderThreadCount = intersectionCPUDevices.size() + gpuRenderThreadCount;
		cerr << "Starting "<< renderThreadCount << " render threads" << endl;
		if (gpuRenderThreadCount > 0) {
			if ((gpuRenderThreadCount == 1) && (intersectionGPUDevices.size() == 1)) {
				// Optimize the special case of one render thread and one GPU
				o2mDevice = NULL;
				m2oDevice = NULL;

				DeviceRenderThread *t = new DeviceRenderThread(1, intersectionGPUDevices[0], scene, lowLatency);
				renderThreads.push_back(t);
				t->Start();
			} else {
				// Create and start the virtual devices (only if htere is more than one GPUs)
				o2mDevice = new VirtualO2MIntersectionDevice(intersectionGPUDevices, scene, 0);
				m2oDevice = new VirtualM2OIntersectionDevice(gpuRenderThreadCount, o2mDevice, scene);

				for (size_t i = 0; i < gpuRenderThreadCount; ++i) {
					DeviceRenderThread *t = new DeviceRenderThread(i + 1, m2oDevice->GetVirtualDevice(i), scene, lowLatency);
					renderThreads.push_back(t);
					t->Start();
				}
			}
		} else {
			o2mDevice = NULL;
			m2oDevice = NULL;
		}

		for (size_t i = 0; i < intersectionCPUDevices.size(); ++i) {
			NativeRenderThread *t = new NativeRenderThread(gpuRenderThreadCount + i, intersectionCPUDevices[i], scene, lowLatency);
			renderThreads.push_back(t);
			t->Start();
		}

		film->StartSampleTime();
	}

	void SetUpOpenCLPlatform(const unsigned int oclPlatformIndex) {

		// Platform info
		VECTOR_CLASS<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		for (size_t i = 0; i < platforms.size(); ++i)
			cerr << "OpenCL Platform " << i << ": " <<
				platforms[i].getInfo<CL_PLATFORM_VENDOR > ().c_str() << endl;

		cerr << "Selected OpenCL Platform: " << oclPlatformIndex << endl;

		if (platforms.size() == 0)
			throw runtime_error("Unable to find an appropiate OpenCL platform");
		else if ((oclPlatformIndex < 0) || (oclPlatformIndex >= platforms.size())) {
			throw runtime_error("Selected OpenCL platform is not available");
		}

		platform = platforms[oclPlatformIndex];
	}

	void SetUpOpenCLDevices(const bool lowLatency, const bool useCPUs, const bool useGPUs,
		const unsigned int forceGPUWorkSize, const string &oclDeviceConfig) {

		// Get the list of devices available on the platform
		VECTOR_CLASS<cl::Device> devices;
		platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

		// Device info
		bool haveSelectionString = (oclDeviceConfig.length() > 0);
		if (haveSelectionString && (oclDeviceConfig.length() != devices.size())) {
			stringstream ss;
			ss << "OpenCL device selection string has the wrong length, must be " <<
					devices.size() << " instead of " << oclDeviceConfig.length();
			throw runtime_error(ss.str().c_str());
		}

		VECTOR_CLASS<cl::Device> selectedDevices;
		for (size_t i = 0; i < devices.size(); ++i) {
			cl_int type = devices[i].getInfo<CL_DEVICE_TYPE > ();
			cerr << "OpenCL Device name " << i << ": " <<
					devices[i].getInfo<CL_DEVICE_NAME > ().c_str() << endl;

			string stype;
			switch (type) {
				case CL_DEVICE_TYPE_ALL:
					stype = "TYPE_ALL";
					break;
				case CL_DEVICE_TYPE_DEFAULT:
					stype = "TYPE_DEFAULT";
					break;
				case CL_DEVICE_TYPE_CPU:
					stype = "TYPE_CPU";
					if (useCPUs && !haveSelectionString)
						selectedDevices.push_back(devices[i]);
					break;
				case CL_DEVICE_TYPE_GPU:
					stype = "TYPE_GPU";
					if (useGPUs && !haveSelectionString)
						selectedDevices.push_back(devices[i]);
					break;
				default:
					stype = "TYPE_UNKNOWN";
					break;
			}

			cerr << "OpenCL Device type " << i << ": " << stype << endl;
			cerr << "OpenCL Device units " << i << ": " <<
					devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS > () << endl;

			if (haveSelectionString && (oclDeviceConfig.at(i) == '1'))
				selectedDevices.push_back(devices[i]);
		}

		if (selectedDevices.size() == 0)
			cerr << "No OpenCL device selected" << endl;
		else {
			// Allocate devices
			for (size_t i = 0; i < selectedDevices.size(); ++i) {
				intersectionGPUDevices.push_back(new OpenCLIntersectionDevice(scene,
						lowLatency, i, selectedDevices[i], forceGPUWorkSize));
			}

			cerr << "OpenCL Devices used: ";
			for (size_t i = 0; i < intersectionGPUDevices.size(); ++i)
				cerr << "[" << intersectionGPUDevices[i]->GetName() << "]";
			cerr << endl;
		}
	}

	void ReInit(const bool reallocBuffers, const unsigned int w = 0, unsigned int h = 0) {
		// First stop all devices
		StopAllDevice();

		// Check if I have to reallocate buffers
		if (reallocBuffers)
			film->Init(w, h);
		else
			film->Reset();
		scene->camera->Update();

		// Restart all devices
		StartAllDevice();
	}

	void SetMaxPathDepth(const int delta) {
		// First stop all devices
		StopAllDevice();

		film->Reset();
		scene->maxPathDepth = max<unsigned int>(2, scene->maxPathDepth + delta);

		// Restart all devices
		StartAllDevice();
	}

	void SetShadowRays(const int delta) {
		// First stop all devices
		StopAllDevice();

		scene->shadowRayCount = max<unsigned int>(1, scene->shadowRayCount + delta);
		for (size_t i = 0; i < renderThreads.size(); ++i)
			renderThreads[i]->ClearPaths();

		// Restart all devices
		StartAllDevice();
	}

	map<string, string> cfg;

	const vector<IntersectionDevice *> &GetIntersectionDevices() { return intersectionAllDevices; }
	const vector<RenderThread *> &GetRenderThreads() { return renderThreads; }

	char captionBuffer[512];
	unsigned int screenRefreshInterval;

	Scene *scene;
	Film *film;

private:
	void StartAllDevice() {
		for (size_t i = 0; i < renderThreads.size(); ++i)
			renderThreads[i]->Start();
	}

	void StopAllDevice() {
		for (size_t i = 0; i < renderThreads.size(); ++i)
			renderThreads[i]->Interrupt();

		for (size_t i = 0; i < renderThreads.size(); ++i)
			renderThreads[i]->Stop();
	}

	cl::Platform platform;

	vector<RenderThread *> renderThreads;

	vector<IntersectionDevice *> intersectionGPUDevices;
	VirtualM2OIntersectionDevice *m2oDevice;
	VirtualO2MIntersectionDevice *o2mDevice;

	vector<NativeIntersectionDevice *> intersectionCPUDevices;

	vector<IntersectionDevice *> intersectionAllDevices;
};

#endif	/* _RENDERCONFIG_H */
