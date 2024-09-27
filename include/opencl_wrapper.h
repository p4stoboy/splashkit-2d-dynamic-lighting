#pragma once

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>
#include <string>
#include <vector>
#include "types.h"

class OpenCLWrapper {
public:
    OpenCLWrapper();
    ~OpenCLWrapper();

    void initialize();
    void calculateLighting(Grid& grid, const std::vector<RadialLight>& lights, const Torch& torch, bool torch_on);

private:
    cl::Context context;
    cl::CommandQueue queue;
    cl::Program program;
    cl::Kernel torchKernel;
    cl::Kernel radialKernel;
    cl::Buffer gridHeightsBuffer;
    cl::Buffer lightLevelsBuffer;
    cl::Buffer torchBuffer;
    cl::Buffer radialLightsBuffer;

    void createBuffers(int gridSize);
    std::string readKernelSource(const std::string& filename);
    std::string getOpenCLErrorDescription(cl_int error);
};
