// opencl_wrapper.h
#pragma once

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>
#include <string>
#include <vector>
#include "types.h"

class OpenCLWrapper {
public:
    struct GridRenderData {
        std::vector<float> positions;
        std::vector<float> colors;
    };

    OpenCLWrapper();
    ~OpenCLWrapper();

    void initialize();
    GridRenderData calculateLighting(const Grid& grid, const std::vector<RadialLight>& lights, const Torch& torch, bool torch_on);
    void createBuffers(size_t gridSize);
    void ensureBufferSize(size_t gridSize);

private:
    size_t currentGridSize;
    cl::Context context;
    cl::CommandQueue queue;
    cl::Program program;
    cl::Kernel kernel;
    cl::Buffer positionsBuffer;
    cl::Buffer colorsBuffer;
    cl::Buffer gridHeightsBuffer;
    cl::Buffer lightLevelsBuffer;
    cl::Buffer torchBuffer;
    cl::Buffer radialLightsBuffer;
    cl::Buffer baseColorsBuffer;
    cl::Buffer outputColorsBuffer;

    std::string readKernelSource(const std::string& filename);
    std::string getOpenCLErrorDescription(cl_int error);
};

// Helper functions
int color_to_int(const color& c);
color int_to_color(int c);
