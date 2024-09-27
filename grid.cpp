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
    std::uniform_int_distribution<> height_dist(static_cast<int>(HeightLevel::WALL), static_cast<int>(HeightLevel::CEILING));

    const int square_size = 10;
    const int num_squares = 380;
    std::set<std::pair<int, int>> square_positions;

    for (int i = 0; i < num_squares; ++i) {
        int sx, sy;
        do {
            sx = x_dist(gen);
            sy = y_dist(gen);
        } while (square_positions.count({sx, sy}) > 0);

        square_positions.insert({sx, sy});
        HeightLevel square_height = HeightLevel::BOX; //static_cast<HeightLevel>(height_dist(gen));

        for (int y = sy; y < std::min(sy + square_size, height); ++y) {
            for (int x = sx; x < std::min(sx + square_size, width); ++x) {
                grid.cells[y * width + x].height = square_height;
                grid.cells[y * width + x].base_color = height_to_color(square_height);
            }
        }
    }

    return grid;
}

Cell get_cell(const Grid& grid, int x, int y) {
    if (x >= 0 && x < grid.width && y >= 0 && y < grid.height) {
        return grid.cells[y * grid.width + x];
    }
    return {HeightLevel::FLOOR, 0, height_to_color(HeightLevel::FLOOR)};
}

void render_grid(const Grid& grid) {
    for (int y = 0; y < grid.height; ++y) {
        for (int x = 0; x < grid.width; ++x) {
            const Cell& cell = grid.cells[y * grid.width + x];
            color final_color = apply_lighting(cell.base_color, cell.light_level);
            fill_rectangle(final_color, x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        }
    }
}
