#include "include/opencl_wrapper.h"
#include <fstream>
#include <iostream>

OpenCLWrapper::OpenCLWrapper() {}

OpenCLWrapper::~OpenCLWrapper() {}

void OpenCLWrapper::initialize() {
    try {
        // Get available platforms
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        // Select the first platform
        cl::Platform platform = platforms[0];

        // Get available devices
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

        // Select the first device
        cl::Device device = devices[0];

        // Print device information
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << "Device type: ";
        switch (device.getInfo<CL_DEVICE_TYPE>()) {
            case CL_DEVICE_TYPE_CPU: std::cout << "CPU" << std::endl; break;
            case CL_DEVICE_TYPE_GPU: std::cout << "GPU" << std::endl; break;
            case CL_DEVICE_TYPE_ACCELERATOR: std::cout << "Accelerator" << std::endl; break;
            default: std::cout << "Other" << std::endl; break;
        }
        std::cout << "Device vendor: " << device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
        std::cout << "Device version: " << device.getInfo<CL_DEVICE_VERSION>() << std::endl;
        std::cout << "Driver version: " << device.getInfo<CL_DRIVER_VERSION>() << std::endl;

        // Create context
        context = cl::Context(device);

        // Create command queue
        queue = cl::CommandQueue(context, device);

        // Read kernel source
        std::string kernelSource = readKernelSource("lighting_kernels.cl");

        // Create program
        program = cl::Program(context, kernelSource);

        // Build program
        program.build({device});

        // Create kernels
        torchKernel = cl::Kernel(program, "calculate_torch_lighting");
        radialKernel = cl::Kernel(program, "calculate_radial_lighting");

    } catch (cl::Error& e) {
        std::cerr << "OpenCL error: " << e.what() << " (" << e.err() << ")" << std::endl;
        exit(1);
    }
}

void OpenCLWrapper::createBuffers(int gridSize) {
    gridHeightsBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, gridSize * sizeof(int));
    lightLevelsBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, gridSize * sizeof(int));
    torchBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(Torch));
    radialLightsBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, MAX_RADIAL_LIGHTS * sizeof(RadialLight));
}

void OpenCLWrapper::calculateLighting(Grid& grid, const std::vector<RadialLight>& lights, const Torch& torch, bool torch_on) {
    try {
        int gridSize = grid.width * grid.height;

        if (!gridHeightsBuffer()) {
            createBuffers(gridSize);
        }

        std::vector<cl_int> gridHeights(gridSize);
        std::vector<cl_int> lightLevels(gridSize, 0);

        for (int i = 0; i < gridSize; ++i) {
            gridHeights[i] = static_cast<cl_int>(grid.cells[i].height);
        }

        queue.enqueueWriteBuffer(gridHeightsBuffer, CL_TRUE, 0, gridSize * sizeof(cl_int), gridHeights.data());
        queue.enqueueWriteBuffer(lightLevelsBuffer, CL_TRUE, 0, gridSize * sizeof(cl_int), lightLevels.data());
        queue.enqueueWriteBuffer(radialLightsBuffer, CL_TRUE, 0, lights.size() * sizeof(RadialLight), lights.data());

        radialKernel.setArg(0, lightLevelsBuffer);
        radialKernel.setArg(1, gridHeightsBuffer);
        radialKernel.setArg(2, radialLightsBuffer);
        radialKernel.setArg(3, static_cast<cl_int>(lights.size()));
        radialKernel.setArg(4, static_cast<cl_int>(grid.width));
        radialKernel.setArg(5, static_cast<cl_int>(grid.height));

        queue.enqueueNDRangeKernel(radialKernel, cl::NullRange, cl::NDRange(grid.width, grid.height));

        if (torch_on) {
            queue.enqueueWriteBuffer(torchBuffer, CL_TRUE, 0, sizeof(Torch), &torch);

            torchKernel.setArg(0, lightLevelsBuffer);
            torchKernel.setArg(1, gridHeightsBuffer);
            torchKernel.setArg(2, torchBuffer);
            torchKernel.setArg(3, static_cast<cl_int>(grid.width));
            torchKernel.setArg(4, static_cast<cl_int>(grid.height));

            queue.enqueueNDRangeKernel(torchKernel, cl::NullRange, cl::NDRange(grid.width, grid.height));
        }

        queue.enqueueReadBuffer(lightLevelsBuffer, CL_TRUE, 0, gridSize * sizeof(cl_int), lightLevels.data());

        for (int i = 0; i < gridSize; ++i) {
            grid.cells[i].light_level = lightLevels[i];
        }

    } catch (cl::Error& e) {
        std::cerr << "OpenCL error in calculateLighting: " << e.what() << " (" << e.err() << ")" << std::endl;
        std::cerr << "Error occurred during: " << getOpenCLErrorDescription(e.err()) << std::endl;
    }
}

std::string OpenCLWrapper::readKernelSource(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file: " + filename);
    }
    return std::string(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
}

std::string OpenCLWrapper::getOpenCLErrorDescription(cl_int error) {
    switch (error) {
        case CL_INVALID_KERNEL_ARGS: return "Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION: return "Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE: return "Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE: return "Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET: return "Invalid global offset";
        case CL_OUT_OF_RESOURCES: return "Out of resources";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "Memory object allocation failure";
        case CL_INVALID_EVENT_WAIT_LIST: return "Invalid event wait list";
        case CL_OUT_OF_HOST_MEMORY: return "Out of host memory";
        default: return "Unknown error";
    }
}
