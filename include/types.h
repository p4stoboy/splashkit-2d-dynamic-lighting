// types.h
#ifndef TYPES_H
#define TYPES_H

#include "splashkit.h"
#include <vector>
#include <cmath>

const int MAX_RADIAL_LIGHTS = 10;

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 1200;
const int GRID_WIDTH = 250;
const int GRID_HEIGHT = 250;
const int CELL_SIZE = SCREEN_WIDTH / GRID_WIDTH;
const int MAX_HEIGHT = 80;
//const int GRID_WIDTH = 200;
//const int GRID_HEIGHT = 200;
//const int CELL_SIZE = 6;  // Only used for rendering
//const int SCREEN_WIDTH = GRID_WIDTH * CELL_SIZE;
//const int SCREEN_HEIGHT = GRID_HEIGHT * CELL_SIZE;

const double AMBIENT_LIGHT = 0.1;
const double LIGHT_LEVELS = 5;

const double BREATHING_SPEED = 2.0;
const double BREATHING_MAGNITUDE = 3.0;

// All distances now in grid units
const double TORCH_RADIUS = 18.0;
const double TORCH_ANGLE = 60.0;
const double TORCH_ANGLE_RAD = TORCH_ANGLE * M_PI / 180.0;
const int TORCH_BARS = 4; // Number of distinct light bars in the torch

const double PLAYER_TURN_SPEED = 0.07; // Radians per frame
const double PLAYER_ACCELERATION = 0.05;
const double PLAYER_MAX_SPEED = 0.3;
const double PLAYER_FRICTION = 0.1;
const int BULLET_COOLDOWN = 8;

// Bullet constants
// Bullet constants
const double BULLET_SPEED = 10;
const double BULLET_RADIUS = 3.0;
const double BULLET_LIGHT_WIDTH = 0.5;  // Half-width of the bullet light area
const double BULLET_TRACER_LENGTH = 0.5;
const int BULLET_LIGHT_LEVEL = 1;
const double BULLET_VELOCITY_DECAY = 0.9; // 1% velocity loss per frame
const int BULLET_LIFETIME = 60; // frame

const double PI = 3.14159265358979323846;


// Height levels enum
enum class HeightLevel {
    BLOCK1 = 5,
    BLOCK2 = 10,
    BLOCK3 = 15,
    FLOOR = 1,
    BOX = 15,
    PLAYER = 10,
    TORCH = 30,
    BULLET = 25,
    WALL = 49,
    RADIAL = 50,
    CEILING = 40
};

struct Vector2D {
    double x, y;
};

struct Cell {
    HeightLevel height;
    int light_level;
    color base_color;
    color final_color;
};

struct Grid {
    std::vector<Cell> cells;
    int width;
    int height;
};

struct Player {
    Vector2D position;
    Vector2D velocity;
    double heading;
    int health;
    int cooldown;
};

struct Torch {
    Vector2D position;
    Vector2D direction;
    double base_radius;
    double current_radius;
};

struct RadialLight {
    Vector2D position;
    double intensity;
    double radius;
    Vector2D velocity; // New property
    int height;  // New property
};

struct Bullet {
    Vector2D position;
    Vector2D velocity;
    int lifetime;
};

struct Particle {
    Vector2D position;
    Vector2D velocity;
    int lifetime;
    color particle_color;
    double velocity_decay;
};

inline color height_to_color(HeightLevel height) {
    switch (height) {
        case HeightLevel::BLOCK1: return rgba_color(150, 150, 150, 255);
        case HeightLevel::BLOCK2: return rgba_color(180, 180, 180, 255);
        case HeightLevel::BLOCK3: return rgba_color(210, 210, 210, 255);
        case HeightLevel::FLOOR: return rgba_color(50, 50, 50, 255);
        case HeightLevel::WALL: return rgba_color(150, 150, 150, 255);
        case HeightLevel::CEILING: return rgba_color(200, 200, 200, 255);
        default: return rgba_color(200, 200, 200, 255);
    }
}

// Function declarations
Grid create_grid(int width, int height);
Cell get_cell(const Grid& grid, int x, int y);
void update_player(Player& player, const Grid& grid);
double calculate_breathing_radius(double base_radius, double total_time);
void update_torch(Torch& torch, const Player& player, double total_time);
void update_grid_lighting(Grid& grid, const std::vector<RadialLight>& lights, const Torch& torch, const Player& player, bool torch_on);
void render_grid(const Grid& grid);
void render_player(const Player& player);
color apply_lighting(color base_color, int light_level);
void update_bullets(std::vector<Bullet>& bullets, std::vector<struct Particle>& particles, Grid& grid);
void create_bullet(std::vector<Bullet>& bullets, Player& player);
void render_bullets(const std::vector<Bullet>& bullets);
void calculate_radial_lighting(Grid& grid, const std::vector<RadialLight>& lights, const Player& player);
void calculate_torch_lighting(Grid& grid, const Torch& torch, const Player& player);
void calculate_bullet_lighting(Grid& grid, const std::vector<Bullet>& bullets);
bool ray_cast_collision(const Grid& grid, const Vector2D& start, const Vector2D& end, Vector2D& hit_point);
void update_radial_light_movers(std::vector<RadialLight>& lights, const Grid& grid, double deltaTime);
void update_radial_light_mover(RadialLight& light, const Grid& grid, double deltaTime);
void create_particles(std::vector<Particle>& particles, const Vector2D& hit_point, const Vector2D& normal, int count);
void update_particles(std::vector<Particle>& particles);
void render_particles(const std::vector<Particle>& particles);
bool has_clear_path(const Grid& grid, int x1, int y1, int z1, int x2, int y2, int z2, const Player& player);
void draw_crosshair();
void initialize_lighting();
void update_grid_lighting(Grid& grid, const std::vector<RadialLight>& lights, const Torch& torch, bool torch_on);

#endif // TYPES_H
