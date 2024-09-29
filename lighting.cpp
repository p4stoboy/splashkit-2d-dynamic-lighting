#include "./include/types.h"
#include "splashkit.h"
#include <cmath>
#include <algorithm>


double calculate_breathing_radius(double base_radius, double total_time) {
    return base_radius + std::sin(total_time * BREATHING_SPEED) * BREATHING_MAGNITUDE;
}

void update_torch(Torch& torch, const Player& player, double total_time) {
    torch.position = player.position;
    torch.direction = {cos(player.heading), sin(player.heading)};
    torch.current_radius = calculate_breathing_radius(torch.base_radius, total_time);
}

void update_grid_lighting(const std::vector<RadialLight>& lights, const Torch& torch, bool torch_on, OpenCLWrapper& openclWrapper) {
    try {
        write_line("Number of lights: " + std::to_string(lights.size()));
        write_line(string("Torch on: ") + (torch_on ? "yes" : "no"));

        openclWrapper.calculateLighting(lights, torch, torch_on);

        write_line("Grid lighting calculation completed.");
    } catch (const cl::Error& e) {
        write_line("OpenCL error in update_grid_lighting: " + string(e.what()) + " (" + std::to_string(e.err()) + ")");
    } catch (const std::exception& e) {
        write_line("Standard exception in update_grid_lighting: " + string(e.what()));
    } catch (...) {
        write_line("Unknown exception in update_grid_lighting");
    }
}

color apply_lighting(color base_color, int light_level) {
    double luminosity = AMBIENT_LIGHT + (1.0 - AMBIENT_LIGHT) * (static_cast<double>(light_level) / LIGHT_LEVELS);
    int r = static_cast<int>(luminosity * red_of(base_color));
    int g = static_cast<int>(luminosity * green_of(base_color));
    int b = static_cast<int>(luminosity * blue_of(base_color));
    return rgba_color(r, g, b, 255);
}

void update_radial_light_mover(RadialLight& light, int gridWidth, int gridHeight, double deltaTime) {
    const double SPEED = 5.0; // Constant speed, adjust as needed

    // Move the light
    light.position.x += light.velocity.x * SPEED * deltaTime;
    light.position.y += light.velocity.y * SPEED * deltaTime;

    // Check for collisions with grid edges
    if (light.position.x < 0 || light.position.x >= gridWidth) {
        light.velocity.x = -light.velocity.x;
        light.position.x = std::max(0.0, std::min(static_cast<double>(gridWidth - 0.01), light.position.x));
    }

    if (light.position.y < 0 || light.position.y >= gridHeight) {
        light.velocity.y = -light.velocity.y;
        light.position.y = std::max(0.0, std::min(static_cast<double>(gridHeight - 0.01), light.position.y));
    }
}

void update_radial_light_movers(std::vector<RadialLight>& lights, int gridWidth, int gridHeight, double deltaTime) {
    for (auto& light : lights) {
        update_radial_light_mover(light, gridWidth, gridHeight, deltaTime);
    }
}
