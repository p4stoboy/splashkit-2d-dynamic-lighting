// lighting_kernels.cl

#define LIGHT_LEVELS 5
#define PLAYER_HEIGHT 10
#define TORCH_HEIGHT 30
#define M_PI 3.14159265358979323846f

typedef struct {
    double x, y;
} Vector2D;

typedef struct {
    Vector2D position;
    double intensity;
    double radius;
    Vector2D velocity;
    int height;
} RadialLight;

typedef struct {
    Vector2D position;
    Vector2D direction;
    double base_radius;
    double current_radius;
} Torch;

bool has_clear_path(__global const int* grid_heights, int x1, int y1, int z1, int x2, int y2, int z2, int grid_width, int grid_height) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int dz = z2 - z1;
    int x = x1;
    int y = y1;
    float z = (float)z1 + 0.1f;
    int n = 1 + dx + dy;
    int x_inc = (x2 > x1) ? 1 : -1;
    int y_inc = (y2 > y1) ? 1 : -1;
    float z_inc = (float)dz / n;
    int error = dx - dy;
    dx *= 2;
    dy *= 2;

    for (int i = 0; i < n; ++i) {
        if (x >= 0 && x < grid_width && y >= 0 && y < grid_height) {
            int cell_height = grid_heights[y * grid_width + x];
            if (z < cell_height) {
                return false;
            }
        }

        if (error > 0) {
            x += x_inc;
            error -= dy;
        } else {
            y += y_inc;
            error += dx;
        }
        z += z_inc;
    }

    return true;
}

__kernel void calculate_lighting(
    __global const int* grid_heights,
    __global const RadialLight* lights,
    __global const int* base_colors,
    __global float2* positions,
    __global float3* colors,
    __constant Torch* torch,
    int num_lights,
    int grid_width,
    int grid_height,
    float ambient_light,
    int torch_on,
    int cell_size
) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= grid_width || y >= grid_height) return;

    int index = y * grid_width + x;

    // Debug pattern: alternating red and white cells
    float r = (x + y) % 2 == 0 ? 1.0f : 1.0f;
    float g = (x + y) % 2 == 0 ? 0.0f : 1.0f;
    float b = (x + y) % 2 == 0 ? 0.0f : 1.0f;

    // Calculate position in pixel coordinates
    float px = x * cell_size;
    float py = y * cell_size;

    // Output position
    positions[index] = (float2)(px, py);

    // Output color
    colors[index] = (float3)(r, g, b);
}
