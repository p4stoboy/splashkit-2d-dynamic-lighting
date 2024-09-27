// opencl_wrapper.cpp
#include "include/opencl_wrapper.h"
#include <fstream>
#include <iostream>

// Helper functions for color conversion
int color_to_int(const color& c) {
    return (static_cast<int>(c.r * 255) << 24) |
           (static_cast<int>(c.g * 255) << 16) |
           (static_cast<int>(c.b * 255) << 8) |
           static_cast<int>(c.a * 255);
}

color int_to_color(int c) {
    return color{
            static_cast<float>((c >> 24) & 0xFF) / 255.0f,
            static_cast<float>((c >> 16) & 0xFF) / 255.0f,
            static_cast<float>((c >> 8) & 0xFF) / 255.0f,
            static_cast<float>(c & 0xFF) / 255.0f
    };
}

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
            case CL_DEVICE_TYPE_CPU:
                std::cout << "CPU" << std::endl;
                break;
            case CL_DEVICE_TYPE_GPU:
                std::cout << "GPU" << std::endl;
                break;
            case CL_DEVICE_TYPE_ACCELERATOR:
                std::cout << "Accelerator" << std::endl;
                break;
            default:
                std::cout << "Other" << std::endl;
                break;
        }
        std::cout << "Device vendor: " << device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
        std::cout << "Device version: " << device.getInfo<CL_DEVICE_VERSION>() << std::endl;
        std::cout << "Driver version: " << device.getInfo<CL_DRIVER_VERSION>() << std::endl;
        std::cout << "Device compute units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
        std::cout << "Device global memory: " << device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / (1024 * 1024) << " MB" << std::endl;
        std::cout << "Device max clock frequency: " << device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << " MHz" << std::endl;

        // Create context
        context = cl::Context(device);

        // Create command queue
        queue = cl::CommandQueue(context, device);

        // Read kernel source
        std::string kernelSource = readKernelSource("lighting_kernels.cl");

        // Create program
        program = cl::Program(context, kernelSource);

        try {
            program.build({device});
        } catch (cl::Error& error) {
            if (error.err() == CL_BUILD_PROGRAM_FAILURE) {
                std::string build_log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
                std::cerr << "OpenCL kernel build log:" << std::endl
                          << build_log << std::endl;
            }
            throw error;
        }

        // Create kernel
        kernel = cl::Kernel(program, "calculate_lighting");

    } catch (cl::Error& e) {
        std::cerr << "OpenCL error: " << e.what() << " (" << e.err() << ")" << std::endl;
        exit(1);
    }
}

void OpenCLWrapper::createBuffers(size_t gridSize) {
    try {
        gridHeightsBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, gridSize * sizeof(cl_int));
        radialLightsBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, MAX_RADIAL_LIGHTS * sizeof(RadialLight));
        baseColorsBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, gridSize * sizeof(cl_int));
        positionsBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY, gridSize * 2 * sizeof(cl_float));
        colorsBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY, gridSize * 3 * sizeof(cl_float));
        torchBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(Torch));

        currentGridSize = gridSize;

        std::cout << "Buffers created successfully:" << std::endl;
        std::cout << "  gridSize: " << gridSize << std::endl;
        std::cout << "  gridHeightsBuffer size: " << gridSize * sizeof(cl_int) << " bytes" << std::endl;
        std::cout << "  radialLightsBuffer size: " << MAX_RADIAL_LIGHTS * sizeof(RadialLight) << " bytes" << std::endl;
        std::cout << "  baseColorsBuffer size: " << gridSize * sizeof(cl_int) << " bytes" << std::endl;
        std::cout << "  positionsBuffer size: " << gridSize * 2 * sizeof(cl_float) << " bytes" << std::endl;
        std::cout << "  colorsBuffer size: " << gridSize * 3 * sizeof(cl_float) << " bytes" << std::endl;
        std::cout << "  torchBuffer size: " << sizeof(Torch) << " bytes" << std::endl;
    } catch (cl::Error& e) {
        std::cerr << "OpenCL error in createBuffers: " << e.what() << " (" << e.err() << ")" << std::endl;
        throw;
    }
}

void OpenCLWrapper::ensureBufferSize(size_t gridSize) {
    if (currentGridSize != gridSize) {
        std::cout << "Recreating buffers due to size change. Old size: " << currentGridSize << ", New size: " << gridSize << std::endl;
        createBuffers(gridSize);
    }
}

OpenCLWrapper::GridRenderData OpenCLWrapper::calculateLighting(const Grid& grid, const std::vector<RadialLight>& lights, const Torch& torch, bool torch_on) {
    try {
        size_t gridSize = static_cast<size_t>(grid.width) * grid.height;

        ensureBufferSize(gridSize);

        // Write grid heights to buffer
        std::vector<cl_int> gridHeights(gridSize);
        for (size_t i = 0; i < gridSize; ++i) {
            gridHeights[i] = static_cast<cl_int>(grid.cells[i].height);
        }
        queue.enqueueWriteBuffer(gridHeightsBuffer, CL_TRUE, 0, gridSize * sizeof(cl_int), gridHeights.data());

        // Write radial lights to buffer
        queue.enqueueWriteBuffer(radialLightsBuffer, CL_TRUE, 0, lights.size() * sizeof(RadialLight), lights.data());

        // Write torch data to buffer
        queue.enqueueWriteBuffer(torchBuffer, CL_TRUE, 0, sizeof(Torch), &torch);

        // Write base colors to buffer
        std::vector<cl_int> baseColors(gridSize);
        for (size_t i = 0; i < gridSize; ++i) {
            baseColors[i] = color_to_int(grid.cells[i].base_color);
        }
        queue.enqueueWriteBuffer(baseColorsBuffer, CL_TRUE, 0, gridSize * sizeof(cl_int), baseColors.data());

        // Set kernel arguments
        kernel.setArg(0, gridHeightsBuffer);
        kernel.setArg(1, radialLightsBuffer);
        kernel.setArg(2, baseColorsBuffer);
        kernel.setArg(3, positionsBuffer);
        kernel.setArg(4, colorsBuffer);
        kernel.setArg(5, torchBuffer);
        kernel.setArg(6, static_cast<cl_int>(lights.size()));
        kernel.setArg(7, static_cast<cl_int>(grid.width));
        kernel.setArg(8, static_cast<cl_int>(grid.height));
        kernel.setArg(9, static_cast<cl_float>(AMBIENT_LIGHT));
        kernel.setArg(10, static_cast<cl_int>(torch_on));
        kernel.setArg(11, static_cast<cl_int>(CELL_SIZE));

        // Execute the kernel
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(grid.width, grid.height));

        // Read back the results
        GridRenderData renderData;
        renderData.positions.resize(gridSize * 2);
        renderData.colors.resize(gridSize * 3);

        cl::Event positionEvent, colorEvent;
        queue.enqueueReadBuffer(positionsBuffer, CL_FALSE, 0, gridSize * 2 * sizeof(cl_float), renderData.positions.data(), nullptr, &positionEvent);
        queue.enqueueReadBuffer(colorsBuffer, CL_FALSE, 0, gridSize * 3 * sizeof(cl_float), renderData.colors.data(), nullptr, &colorEvent);

        positionEvent.wait();
        colorEvent.wait();

        // Debug output
        std::cout << "GridRenderData size: positions=" << renderData.positions.size()
                  << ", colors=" << renderData.colors.size() << std::endl;

        return renderData;

    } catch (cl::Error& e) {
        std::cerr << "OpenCL error in calculateLighting: " << e.what() << " (" << e.err() << ")" << std::endl;
        std::cerr << "Error occurred during: " << getOpenCLErrorDescription(e.err()) << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Standard exception in calculateLighting: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in calculateLighting" << std::endl;
    }

    return GridRenderData(); // Return empty data in case of error
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

std::string OpenCLWrapper::readKernelSource(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file: " + filename);
    }
    return std::string(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
}
