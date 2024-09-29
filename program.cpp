#include "include/types.h"
#include "splashkit.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <omp.h>
#include <deque>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <algorithm>

OpenCLWrapper openclWrapper;

void initialize_lighting() {
    openclWrapper.initialize();
}

std::vector<RadialLight> create_radial_lights(int num_lights, int grid_width, int grid_height) {
    write_line("Creating radial lights...");
    write_line("Number of lights: " + std::to_string(num_lights));
    write_line("Grid dimensions: " + std::to_string(grid_width) + "x" + std::to_string(grid_height));

    std::vector<RadialLight> lights;
    write_line("Created empty lights vector.");

    write_line("Creating random device...");
    std::random_device rd;
    write_line("Random device created.");

    write_line("Creating random generator...");
    std::mt19937 gen(rd());
    write_line("Random generator created.");

    write_line("Creating distributions...");
    std::uniform_real_distribution<> x_dis(0, grid_width - 1);
    write_line("X distribution created.");
    std::uniform_real_distribution<> y_dis(0, grid_height - 1);
    write_line("Y distribution created.");
    std::uniform_int_distribution<> intensity_dis(1, LIGHT_LEVELS);
    write_line("Intensity distribution created.");
    std::uniform_real_distribution<> radius_dis(10.0, 30.0);
    write_line("Radius distribution created.");
    std::uniform_int_distribution<> height_dis(static_cast<int>(HeightLevel::CEILING), static_cast<int>(HeightLevel::RADIAL));
    write_line("Height distribution created.");
    write_line("All distributions created successfully.");

    for (int i = 0; i < num_lights; ++i) {
        write_line("Creating light " + std::to_string(i + 1));

        write_line("Generating random values...");
        write_line("Generating X...");
        double x = x_dis(gen);
        write_line("X generated: " + std::to_string(x));
        write_line("Generating Y...");
        double y = y_dis(gen);
        write_line("Y generated: " + std::to_string(y));
        write_line("Generating intensity...");
        double intensity = static_cast<double>(intensity_dis(gen));
        write_line("Intensity generated: " + std::to_string(intensity));
        write_line("Generating radius...");
        double radius = radius_dis(gen);
        write_line("Radius generated: " + std::to_string(radius));
        write_line("Generating height...");
        int height = height_dis(gen);
        write_line("Height generated: " + std::to_string(height));
        write_line("All random values generated successfully.");

        write_line("Creating RadialLight object...");
        RadialLight light{
                {x, y},
                intensity,
                radius,
                {1, 0.5},  // Initial velocity
                height
        };
        write_line("RadialLight object created successfully.");

        write_line("Light created: pos(" + std::to_string(light.position.x) + ", " + std::to_string(light.position.y) +
                   "), intensity: " + std::to_string(light.intensity) +
                   ", radius: " + std::to_string(light.radius) +
                   ", height: " + std::to_string(light.height));

        write_line("Adding light to vector...");
        lights.push_back(light);
        write_line("Light added to vector successfully.");
    }

    write_line("Radial lights creation completed.");
    return lights;
}

void render_frame(const Grid& grid, const Player& player, const std::vector<Particle>& particles, bool torch_on) {
    clear_screen(COLOR_BLACK);
    // Render grid
    for (int y = 0; y < grid.height; ++y) {
        for (int x = 0; x < grid.width; ++x) {
            int index = y * grid.width + x;
            color cell_color = apply_lighting(grid.cells[index].base_color, grid.cells[index].light_level);
            fill_rectangle(cell_color, x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        }
    }

    render_player(player);
    render_particles(particles);
    draw_crosshair();

    draw_text("Health: " + std::to_string(player.health), COLOR_WHITE, 10, 10);
    draw_text("Torch: " + std::string(torch_on ? "ON" : "OFF"), COLOR_WHITE, 10, 30);
}

int main() {
    try {
        write_line("Starting game initialization...");

        open_window("Lighting Demo", SCREEN_WIDTH, SCREEN_HEIGHT);
        write_line("Window opened successfully.");

        hide_mouse();
        load_sound_effect("gunshot", "gun_shot_1.wav");
        load_sound_effect("hit", "bullet_hit_1.wav");
        write_line("Resources loaded successfully.");

        initialize_lighting();
        write_line("Lighting initialized successfully.");

        Grid initialGrid = create_grid(GRID_WIDTH, GRID_HEIGHT);
        openclWrapper.initializeGrid(initialGrid);
        write_line("Grid created successfully.");

        Player player = {{GRID_WIDTH / 2.0, GRID_HEIGHT / 2.0}, {0, 0}, 0, 100};
        write_line("Player initialized successfully.");

        std::vector<RadialLight> radial_lights;
        try {
            radial_lights = create_radial_lights(10, GRID_WIDTH, GRID_HEIGHT);
            write_line("Radial lights created successfully.");
        } catch (const std::exception& e) {
            write_line("Exception during radial lights creation: " + string(e.what()));
            return 1;
        } catch (...) {
            write_line("Unknown exception during radial lights creation");
            return 1;
        }

        Torch torch = {{player.position.x, player.position.y}, {1, 0}, TORCH_RADIUS, TORCH_RADIUS};
        write_line("Torch created successfully.");

        std::vector<Bullet> bullets;
        std::vector<Particle> particles;

        auto start_time = std::chrono::high_resolution_clock::now();
        auto last_frame_time = start_time;

        const int BENCHMARK_FRAMES = 60;
        std::deque<double> frame_times;

        bool torch_on = true;

        write_line("Entering main game loop...");

        while (!quit_requested() && player.health > 0) {
            auto frame_start = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> delta_duration = frame_start - last_frame_time;
            double delta_time = delta_duration.count();
            last_frame_time = frame_start;

            double total_time = std::chrono::duration<double>(frame_start - start_time).count();

            process_events();

            update_player(player, openclWrapper);
            update_torch(torch, player, total_time);
            update_bullets(bullets, particles, openclWrapper);
            update_particles(particles);
            update_radial_light_movers(radial_lights, openclWrapper.getGridWidth(), openclWrapper.getGridHeight(), delta_time);

            if (mouse_down(LEFT_BUTTON) && player.cooldown == 0) {
                create_bullet(bullets, player);
            }

            if (key_typed(T_KEY)) {
                torch_on = !torch_on;
            }

            write_line("Updating grid lighting...");
            update_grid_lighting(radial_lights, torch, torch_on, openclWrapper);
            write_line("Grid lighting updated successfully.");

            clear_screen(COLOR_BLACK);
            render_grid(openclWrapper);
            render_player(player);
            render_particles(particles);
            draw_crosshair();

            draw_text("Health: " + std::to_string(player.health), COLOR_WHITE, 10, 10);
            draw_text("Torch: " + string(torch_on ? "ON" : "OFF"), COLOR_WHITE, 10, 30);

            // Benchmarking calculations
            auto frame_end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> frame_duration = frame_end - frame_start;

            frame_times.push_back(frame_duration.count());
            if (frame_times.size() > BENCHMARK_FRAMES) {
                frame_times.pop_front();
            }

            double average_frame_time = std::accumulate(frame_times.begin(), frame_times.end(), 0.0) / frame_times.size();
            double fps = 1000.0 / average_frame_time;

            draw_text("Avg Frame Time: " + std::to_string(average_frame_time) + " ms | FPS: " + std::to_string(fps), COLOR_WHITE, 10, SCREEN_HEIGHT - 30);

            refresh_screen(60);

            // FPS limiting
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = current_time - frame_start;
            if (elapsed.count() < 16.67) {
                delay(static_cast<int>(16.67 - elapsed.count()));
            }
        }

        write_line("Game loop ended.");

    } catch (const std::exception& e) {
        write_line("An exception occurred: " + string(e.what()));
    } catch (...) {
        write_line("An unknown exception occurred.");
    }

    return 0;
}
