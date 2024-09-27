// main.cpp
#include "include/types.h"
#include "include/opencl_wrapper.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <deque>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include "splashkit.h"

// OpenGL-related globals
GLuint gridVAO, gridVBO, gridColorVBO;
GLuint gridShaderProgram;

// Grid shader sources
const char* gridVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 fragColor;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    fragColor = aColor;
}
)";

const char* gridFragmentShaderSource = R"(
#version 330 core
in vec3 fragColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(fragColor, 1.0);
}
)";

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Check for vertex shader compile errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Check for fragment shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void initializeOpenGL() {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 fragColor;
    uniform float screenWidth;
    uniform float screenHeight;
    uniform float cellSize;
    void main() {
        vec2 screenPos = aPos * cellSize;
        gl_Position = vec4(screenPos.x / (screenWidth / 2) - 1.0, 1.0 - screenPos.y / (screenHeight / 2), 0.0, 1.0);
        gl_PointSize = cellSize;
        fragColor = aColor;
    }
)";
    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 fragColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(fragColor, 1.0);
    }
)";

    gridShaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &positionVBO);
    glGenBuffers(1, &colorVBO);

    glBindVertexArray(gridVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, gridColorVBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void render_frame(const Grid& grid, const Player& player, const std::vector<RadialLight>& radial_lights,
                  const std::vector<Particle>& particles, const Torch& torch, bool torch_on, OpenCLWrapper& openclWrapper) {
    std::cout << "Entering render_frame" << std::endl;

    OpenCLWrapper::GridRenderData gridData = openclWrapper.calculateLighting(grid, radial_lights, torch, torch_on);

    std::cout << "GridRenderData received: positions=" << gridData.positions.size()
              << ", colors=" << gridData.colors.size() << std::endl;

    if (gridData.positions.empty() || gridData.colors.empty()) {
        std::cerr << "Error: GridRenderData is empty" << std::endl;
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(gridShaderProgram);

    // Set screen size uniforms
    GLint screenWidthLoc = glGetUniformLocation(gridShaderProgram, "screenWidth");
    GLint screenHeightLoc = glGetUniformLocation(gridShaderProgram, "screenHeight");
    if (screenWidthLoc != -1 && screenHeightLoc != -1) {
        glUniform1f(screenWidthLoc, static_cast<float>(SCREEN_WIDTH));
        glUniform1f(screenHeightLoc, static_cast<float>(SCREEN_HEIGHT));
    } else {
        std::cerr << "Error: Unable to set screen size uniforms" << std::endl;
    }

    GLint cellSizeLoc = glGetUniformLocation(gridShaderProgram, "cellSize");
    if (cellSizeLoc != -1) {
        glUniform1f(cellSizeLoc, static_cast<float>(CELL_SIZE));
    } else {
        std::cerr << "Error: Unable to set cell size uniform" << std::endl;
    }

    glBindVertexArray(gridVAO);

    // Update position VBO
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, gridData.positions.size() * sizeof(float), gridData.positions.data(), GL_DYNAMIC_DRAW);

    // Update color VBO
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, gridData.colors.size() * sizeof(float), gridData.colors.data(), GL_DYNAMIC_DRAW);

    // Draw all grid cells in one call
    glDrawArrays(GL_POINTS, 0, grid.width * grid.height);

    glBindVertexArray(0);

    std::cout << "Finished rendering grid cells" << std::endl;

    // Debug output
    std::cout << "Grid size: " << grid.width << "x" << grid.height << std::endl;
    std::cout << "Cell size: " << CELL_SIZE << std::endl;

    std::cout << "Exiting render_frame" << std::endl;
}

std::vector<RadialLight> create_radial_lights(int num_lights, int grid_width, int grid_height) {
    std::vector<RadialLight> lights;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> x_dis(0, grid_width - 1);
    std::uniform_real_distribution<> y_dis(0, grid_height - 1);
    std::uniform_int_distribution<> intensity_dis(1, LIGHT_LEVELS);
    std::uniform_real_distribution<> radius_dis(10.0, 30.0);

    for (int i = 0; i < num_lights; ++i) {
        lights.push_back(RadialLight{
                {x_dis(gen), y_dis(gen)},
                static_cast<float>(intensity_dis(gen)),
                radius_dis(gen),
                {1, 0.5},  // Initial velocity
                100  // Height
        });
    }
    return lights;
}

int main() {
    window w1 = open_window("Lighting Demo", SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_Window* sdl_window = (SDL_Window *)get_window_handle(w1);
    if (!sdl_window) {
        std::cerr << "Failed to get SDL window handle" << std::endl;
        return 0;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);

    hide_mouse();
    load_sound_effect("gunshot", "gun_shot_1.wav");
    load_sound_effect("hit", "bullet_hit_1.wav");

    OpenCLWrapper openclWrapper;
    openclWrapper.initialize();

    initializeOpenGL();
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    Grid grid = create_grid(GRID_WIDTH, GRID_HEIGHT);
    Player player = {{GRID_WIDTH / 2.0, GRID_HEIGHT / 2.0}, {0, 0}, 0, 100};
    std::vector<RadialLight> radial_lights = create_radial_lights(10, GRID_WIDTH, GRID_HEIGHT);
    Torch torch = {{player.position.x, player.position.y}, {1, 0}, TORCH_RADIUS, TORCH_RADIUS};
    std::vector<Bullet> bullets;
    std::vector<Particle> particles;

    auto start_time = std::chrono::high_resolution_clock::now();
    auto last_frame_time = start_time;

    const int BENCHMARK_FRAMES = 60;
    std::deque<double> frame_times;

    bool torch_on = true;

    while (!quit_requested() && player.health > 0) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Calculate delta time
        std::chrono::duration<double> delta_duration = frame_start - last_frame_time;
        double delta_time = delta_duration.count();
        last_frame_time = frame_start;

        double total_time = std::chrono::duration<double>(frame_start - start_time).count();

        process_events();

        update_player(player, grid);
        update_torch(torch, player, total_time);
        update_bullets(bullets, particles, grid);
        update_particles(particles);
        update_radial_light_movers(radial_lights, grid, delta_time);

        if (mouse_down(LEFT_BUTTON) && player.cooldown == 0) {
            create_bullet(bullets, player);
        }

        if (key_typed(T_KEY)) {
            torch_on = !torch_on;
        }

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Render the frame
        render_frame(grid, player, radial_lights, particles, torch, torch_on, openclWrapper);

        // Swap buffers
        SDL_GL_SwapWindow(sdl_window);

        // Benchmarking calculations
        auto frame_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> frame_duration = frame_end - frame_start;

        frame_times.push_back(frame_duration.count());
        if (frame_times.size() > BENCHMARK_FRAMES) {
            frame_times.pop_front();
        }

        double average_frame_time = std::accumulate(frame_times.begin(), frame_times.end(), 0.0) / frame_times.size();
        double fps = 1000.0 / average_frame_time;

        // Display benchmark information
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2)
            << "Avg Frame Time: " << average_frame_time << " ms | FPS: " << fps;
        std::cout << oss.str() << std::endl;
//        draw_text(oss.str(), COLOR_WHITE, 10, SCREEN_HEIGHT - 30);
    }

    // Cleanup
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteBuffers(1, &gridColorVBO);
    glDeleteProgram(gridShaderProgram);

    SDL_GL_DeleteContext(gl_context);
    close_window(w1);

    return 0;
}
