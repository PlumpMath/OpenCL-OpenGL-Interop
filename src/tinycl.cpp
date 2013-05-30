#define __CL_ENABLE_EXCEPTIONS

#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <GL/glew.h>
#include <CL/opencl.h>
#include <CL/cl.hpp>
#include <util.h>
#include "tinycl.h"

#ifdef __linux__
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#elseif defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#endif

CL::TinyCL::TinyCL(DEVICE dev, bool interop){
	if (interop)
		selectInteropDevice(dev);
	else
		selectDevice(dev);
}
cl::Program CL::TinyCL::loadProgram(std::string file){
	cl::Program program;
	try {
		std::string content = Util::readFile(file);
		cl::Program::Sources source(1, std::make_pair(content.c_str(), content.size()));
		program = cl::Program(mContext, source);
		program.build(mDevices);
		return program;
	}
	catch (const cl::Error &e){
		std::cout << "Program Error: " << e.what() 
			<< " code: " << e.err() << std::endl;
		if (e.err() == CL_BUILD_PROGRAM_FAILURE){
			std::cout << "Build error occured, error log:\n" 
				<< program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(mDevices.at(0)) 
				<< std::endl;
		}
	}
	throw std::runtime_error("Failed to load program");
}
cl::Kernel CL::TinyCL::loadKernel(const cl::Program &prog, std::string kernel){
	try {
		return cl::Kernel(prog, kernel.c_str());
	}
	catch (const cl::Error &e){
		std::cout << "Error getting kernel: " << e.what() 
			<< " code: " << e.err() << std::endl;
	}
	throw std::runtime_error("Failed to load kernel");
}
cl::Buffer CL::TinyCL::buffer(MEM mem, size_t dataSize, void *data, bool blocking,
	const std::vector<cl::Event> *dependencies, cl::Event *notify)
{
	try {
		cl::Buffer buf(mContext, mem, dataSize);
		if (data != nullptr)
			mQueue.enqueueWriteBuffer(buf, blocking, 0, dataSize, data, dependencies, notify);
		return buf;
	}
	catch (const cl::Error &e){
		std::cout << "Error creating buffer: " << e.what()
			<< " code: " << e.err() << std::endl;
		throw e;
	}
}
cl::BufferGL CL::TinyCL::bufferGL(MEM mem, GL::VertexBuffer &vbo){
	return cl::BufferGL(mContext, mem, vbo);
}
cl::Image2D CL::TinyCL::image2d(MEM mem, cl::ImageFormat format, int w, int h, cl::size_t<3> origin,
	cl::size_t<3> region, void *pixels, bool blocking, const std::vector<cl::Event> *dependencies, 
	cl::Event *notify)
{
	try {
		//I think this is the error line since I get it even when passing no pixels
		//Intel error, would this error be because I'm using OpenCL 1.2 vs. 1.1?
		//the 1.1 spec makes no mention of the error CL_INVALID_IMAGE_DESCRIPTOR so I think it may be
		//But it's only when writing to GPU? Very strange
		cl::Image2D img(mContext, mem, format, w, h);
		if (pixels != nullptr)
			mQueue.enqueueWriteImage(img, blocking, origin, region, 0, 0, pixels,
				dependencies, notify);
		return img;
	}
	catch (const cl::Error &e){
		std::cout << "Error creating image2d: " << e.what()
			<< " code: " << e.err() << std::endl;
		throw e;
	}
}
#ifdef CL_VERSION_1_2
cl::ImageGL CL::TinyCL::imageFromTexture(MEM mem, GL::Texture &tex){
	return cl::ImageGL(mContext, mem, GL_TEXTURE_2D, 0, tex);
}
#else
cl::Image2DGL CL::TinyCL::imageFromTexture(MEM mem, GL::Texture &tex){
	return cl::Image2DGL(mContext, mem, GL_TEXTURE_2D, 0, tex);
}
#endif
void CL::TinyCL::writeData(const cl::Buffer &b, size_t dataSize, void *data, bool blocking,
	const std::vector<cl::Event> *dependencies, cl::Event *notify)
{
	mQueue.enqueueWriteBuffer(b, blocking, 0, dataSize, data, dependencies, notify);
}
void CL::TinyCL::writeData(const cl::Image &img, cl::size_t<3> origin, cl::size_t<3> region, void *pixels,
	bool blocking, const std::vector<cl::Event> *dependencies, cl::Event *notify)
{
	mQueue.enqueueWriteImage(img, blocking, origin, region, 0, 0, pixels, dependencies, notify);
}
void CL::TinyCL::readData(const cl::Buffer &buf, size_t dataSize, void *data, size_t offset,
	bool blocking, const std::vector<cl::Event> *dependencies, cl::Event *notify)
{
	try {
		mQueue.enqueueReadBuffer(buf, blocking, offset, dataSize, data, dependencies, notify);
	}
	catch (const cl::Error &e){
		std::cout << "Error reading buffer: " << e.what()
			<< " code: " << e.err() << std::endl;
	}
}
void CL::TinyCL::readData(const cl::Image &img, cl::size_t<3> origin, cl::size_t<3> region, void *pixels,
	 bool blocking, const std::vector<cl::Event> *dependencies, cl::Event *notify)
{
	try {
		mQueue.enqueueReadImage(img, blocking, origin, region, 0, 0, pixels, dependencies, notify);
	}
	catch (const cl::Error &e){
		std::cout << "Error reading buffer: " << e.what()
			<< " code: " << e.err() << std::endl;
	}
}
void CL::TinyCL::runKernel(const cl::Kernel &kernel, cl::NDRange local, cl::NDRange global, cl::NDRange offset,
	bool blocking, const std::vector<cl::Event> *dependencies, cl::Event *notify)
{
	try {
		mQueue.enqueueNDRangeKernel(kernel, offset, global, local, dependencies, notify);
	}
	catch (const cl::Error &e){
		std::cout << "Error running nd range kernel: " << e.what()
			<< " code: " << e.err() << std::endl;
	}
}
void CL::TinyCL::selectDevice(DEVICE dev){
	try {
		cl::Platform::get(&mPlatforms);
		//Find the first platform that is the device type we want
		for (int i = 0; mDevices.empty(); ++i){
			try {
				mPlatforms.at(i).getDevices(dev, &mDevices);
			}
			catch (const cl::Error &e){
				//If the device we want isn't in this platform, keep digging
				if (e.err() == CL_DEVICE_NOT_FOUND)
					continue;
				else 
					throw e;
			}
		}
		//Still being kind of lazy and assuming that only one device is on each platform,
		//or at least that the first device is the best
		std::cout << "Device info--\n" << "Name: " << mDevices.at(0).getInfo<CL_DEVICE_NAME>()
			<< "\nVendor: " << mDevices.at(0).getInfo<CL_DEVICE_VENDOR>() 
			<< "\nDriver Version: " << mDevices.at(0).getInfo<CL_DRIVER_VERSION>() 
			<< "\nDevice Profile: " << mDevices.at(0).getInfo<CL_DEVICE_PROFILE>() 
			<< "\nDevice Version: " << mDevices.at(0).getInfo<CL_DEVICE_VERSION>()
			<< "\nMax Work Group Size: " << mDevices.at(0).getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>()
			<< std::endl;
		mContext = cl::Context(mDevices);
		mQueue = cl::CommandQueue(mContext, mDevices.at(0));
	}
	catch (const cl::Error &e){
		std::cout << "Error selecting device: " << e.what() 
			<< " code: " << e.err() << std::endl;
		throw e;
	}
}
void CL::TinyCL::selectInteropDevice(DEVICE dev){
	try {
		//We assume only the first device and platform will be used
		//This is after all a lazy implementation
		cl::Platform::get(&mPlatforms);
		//Query the devices for the type desired
		mPlatforms.at(0).getDevices(dev, &mDevices);
#ifdef _WIN32
		cl_context_properties properties[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
			CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)(mPlatforms[0])(),
			0
		};
#elif defined(__linux__)
		cl_context_properties properties[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
			CL_WGL_HDC_KHR, (cl_context_properties)glXGetCurrentDisplay(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)(mPlatforms[0])(),
			0
		};
#elif defined(__APPLE__)
		CGLContextObj glContext = CGLGetCurrentContext();
		CGLShareGroupObj shareGroup = CGLGetShareGroup(glContext);
		cl_context_properties properties[] = {
			CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
			(cl_context_properties)shareGroup,
		};
#endif

		mContext = cl::Context(mDevices, properties);
		//Grab the OpenGL device
		mDevices = mContext.getInfo<CL_CONTEXT_DEVICES>();
		mQueue = cl::CommandQueue(mContext, mDevices.at(0));
		std::cout << "OpenCL Info:" 
			<< "\nName: " << mDevices.at(0).getInfo<CL_DEVICE_NAME>()
			<< "\nVendor: " << mDevices.at(0).getInfo<CL_DEVICE_VENDOR>() 
			<< "\nDriver Version: " << mDevices.at(0).getInfo<CL_DRIVER_VERSION>() 
			<< "\nDevice Profile: " << mDevices.at(0).getInfo<CL_DEVICE_PROFILE>() 
			<< "\nDevice Version: " << mDevices.at(0).getInfo<CL_DEVICE_VERSION>()
			<< "\nMax Work Group Size: " << mDevices.at(0).getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>()
			<< std::endl;
	}
	catch (const cl::Error &e){
		std::cout << "Error selecting GL interop device: " << e.what() 
			<< " code: " << e.err() << std::endl;
		throw e;
	}

}
int CL::TinyCL::preferredWorkSize(const cl::Kernel &kernel){
	return kernel.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(mDevices.at(0));
}
int CL::TinyCL::maxWorkGroupSize(const cl::Kernel &kernel){
	return kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(mDevices.at(0));
}
