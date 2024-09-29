#include "./include/types.h"
#include <cmath>
#include <random>
#include <set>

Grid create_grid(int width, int height) {
    Grid grid;
    grid.width = width;
    grid.height = height;
    grid.cells.resize(width * height, {HeightLevel::FLOOR, 0, height_to_color(HeightLevel::FLOOR)});

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> x_dist(0, width - 1);
    std::uniform_int_distribution<> y_dist(0, height - 1);
    std::uniform_int_distribution<> height_dist(static_cast<int>(HeightLevel::BLOCK1), static_cast<int>(HeightLevel::BLOCK3));

    const int square_size = 5;
    const int num_squares = 100;
    std::set<std::pair<int, int>> square_positions;

    for (int i = 0; i < num_squares; ++i) {
        int sx, sy;
        do {
            sx = x_dist(gen);
            sy = y_dist(gen);
        } while (square_positions.count({sx, sy}) > 0);

        square_positions.insert({sx, sy});
        HeightLevel square_height = HeightLevel::BOX;

        for (int y = sy; y < std::min(sy + square_size, height); ++y) {
            for (int x = sx; x < std::min(sx + square_size, width); ++x) {
                grid.cells[y * width + x].height = square_height;
                grid.cells[y * width + x].base_color = height_to_color(square_height);
            }
        }
    }

    return grid;
}


void render_grid(const OpenCLWrapper& openclWrapper) {
    int gridWidth = openclWrapper.getGridWidth();
    int gridHeight = openclWrapper.getGridHeight();

    std::vector<int> gridHeights;
    std::vector<int> lightLevels;
    openclWrapper.readGridHeights(gridHeights);
    openclWrapper.readLightLevels(lightLevels);

    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            int index = y * gridWidth + x;
            HeightLevel height = static_cast<HeightLevel>(gridHeights[index]);
            color base_color = height_to_color(height);
            color final_color = apply_lighting(base_color, lightLevels[index]);
            fill_rectangle(final_color, x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        }
    }
}
