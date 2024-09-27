#include "./include/types.h"
#include "splashkit.h"
#include <cmath>



void create_bullet(std::vector<Bullet>& bullets, Player& player) {
    Bullet new_bullet;
    play_sound_effect("gunshot", 1, 0.5);
    new_bullet.position = player.position;
    new_bullet.velocity = {
            cos(player.heading) * BULLET_SPEED,
            sin(player.heading) * BULLET_SPEED
    };
    new_bullet.lifetime = BULLET_LIFETIME;
    player.cooldown = BULLET_COOLDOWN; // TODO: Move this to a separate function
    bullets.push_back(new_bullet);
}

bool ray_cast_collision(const Grid& grid, const Vector2D& start, const Vector2D& end, Vector2D& hit_point) {
    double dx = std::abs(end.x - start.x);
    double dy = std::abs(end.y - start.y);
    int x = static_cast<int>(start.x);
    int y = static_cast<int>(start.y);
    int n = 1 + static_cast<int>(dx + dy);
    int x_inc = (end.x > start.x) ? 1 : -1;
    int y_inc = (end.y > start.y) ? 1 : -1;
    double error = dx - dy;
    dx *= 2;
    dy *= 2;

    for (; n > 0; --n) {
        if (x >= 0 && x < grid.width && y >= 0 && y < grid.height) {
            const Cell& cell = grid.cells[y * grid.width + x];
            if (cell.height > HeightLevel::FLOOR) {
                hit_point = {static_cast<double>(x), static_cast<double>(y)};
                return true;
            }
        }

        if (error > 0) {
            x += x_inc;
            error -= dy;
        } else {
            y += y_inc;
            error += dx;
        }
    }

    return false;
}

void update_bullets(std::vector<Bullet>& bullets, std::vector<Particle>& particles, Grid& grid) {
    for (auto it = bullets.begin(); it != bullets.end();) {
        Vector2D start = it->position;
        Vector2D end = {
                it->position.x + it->velocity.x,
                it->position.y + it->velocity.y
        };

        Vector2D hit_point;
        bool collision = ray_cast_collision(grid, start, end, hit_point);

        if (collision) {
            // Calculate hit normal
            play_sound_effect("hit");
            Vector2D normal;
            int hit_x = static_cast<int>(hit_point.x);
            int hit_y = static_cast<int>(hit_point.y);

            // Determine which side of the block was hit
            double dx = hit_point.x - start.x;
            double dy = hit_point.y - start.y;

            if (std::abs(dx) > std::abs(dy)) {
                // Hit on vertical side
                normal.x = (dx > 0) ? -1.0 : 1.0;
                normal.y = 0.0;
            } else {
                // Hit on horizontal side
                normal.x = 0.0;
                normal.y = (dy > 0) ? -1.0 : 1.0;
            }

            // Create particles
            create_particles(particles, hit_point, normal, 30);  // Create 10 particles

            // Destroy the object
            Cell& cell = grid.cells[hit_y * grid.width + hit_x];
            cell.height = HeightLevel::FLOOR;
            cell.base_color = height_to_color(HeightLevel::FLOOR);

            // Destroy the bullet
            it = bullets.erase(it);
        } else {
            // Update bullet position
            it->position = end;
            it->lifetime--;

            if (it->lifetime <= 0 ||
                it->position.x < 0 || it->position.x >= grid.width ||
                it->position.y < 0 || it->position.y >= grid.height) {
                it = bullets.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void render_bullets(const std::vector<Bullet>& bullets) {
    for (const auto& bullet : bullets) {
        double screen_x = bullet.position.x * CELL_SIZE;
        double screen_y = bullet.position.y * CELL_SIZE;
        drawing_options width = option_line_width(BULLET_RADIUS);
//        // Render bullet as a red line in the direction of movement
//        double line_length = BULLET_RADIUS * 1.5 * CELL_SIZE; // Adjust this value to change the line length
//        double end_x = screen_x + bullet.velocity.x * line_length / BULLET_SPEED;
//        double end_y = screen_y + bullet.velocity.y * line_length / BULLET_SPEED;
//
//        draw_line(COLOR_RED, screen_x, screen_y, end_x, end_y);

        // Render tracer effect
        double alpha = static_cast<double>(bullet.lifetime) / BULLET_LIFETIME;
        color tracer_color = rgba_color(255, 0, 0, static_cast<int>(alpha * 255));
        draw_line(tracer_color,
                  screen_x - bullet.velocity.x * CELL_SIZE * BULLET_TRACER_LENGTH,
                  screen_y - bullet.velocity.y * CELL_SIZE * BULLET_TRACER_LENGTH,
                  screen_x, screen_y, width);
    }
}
