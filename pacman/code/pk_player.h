#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// 简化的常量定义
#define POWER_TIME_LOW 3      // 强化时间不足阈值
#define MAX_BFS_ITER 50       // BFS最大迭代次数，大幅降低以提升性能

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// 简化的初始化
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
}

// 基础函数
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// 计算到鬼魂的最短距离
int distance_to_ghost(struct Player *player, int x, int y) {
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_dist) min_dist = dist;
    }
    return min_dist;
}

// 简化的安全评估
int is_safe(struct Player *player, int x, int y) {
    int ghost_dist = distance_to_ghost(player, x, y);
    
    if (player->your_status > 0) {
        // 强化状态：可以接近鬼魂
        return 1;
    } else {
        // 普通状态：需要保持安全距离
        return ghost_dist >= 2;
    }
}

// 高效的星星评估
float evaluate_star_simple(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int distance = abs(sx - star_x) + abs(sy - star_y);
    int is_power_star = (star_type == 'O');
    
    float score = is_power_star ? 100.0f : 10.0f;
    
    // 距离越近越好
    score = score * 20.0f / (distance + 1);
    
    // 超级星在非强化状态下极高优先级
    if (is_power_star && player->your_status <= 0) {
        score += 500.0f;
    }
    
    // 安全性检查
    if (!is_safe(player, star_x, star_y)) {
        score -= 200.0f;
    }
    
    return score;
}

// 简化的BFS寻找最佳星星
int bfs_to_best_star(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_star_x = -1, best_star_y = -1;
    float best_score = -1000.0f;
    
    // 快速遍历找最佳星星
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                float score = evaluate_star_simple(player, sx, sy, i, j);
                if (score > best_score) {
                    best_score = score;
                    best_star_x = i;
                    best_star_y = j;
                }
            }
        }
    }
    
    if (best_star_x == -1) return 0;  // 没有星星
    
    // 如果目标星星就在相邻位置，直接移动
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (nx == best_star_x && ny == best_star_y && is_valid(player, nx, ny)) {
            *next_x = nx;
            *next_y = ny;
            return 1;
        }
    }
    
    // 简化的方向性移动
    int best_dir = -1;
    int min_dist = INF;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (is_valid(player, nx, ny)) {
            int dist = abs(nx - best_star_x) + abs(ny - best_star_y);
            if (dist < min_dist) {
                min_dist = dist;
                best_dir = d;
            }
        }
    }
    
    if (best_dir != -1) {
        *next_x = sx + dx[best_dir];
        *next_y = sy + dy[best_dir];
        return 1;
    }
    
    return 0;
}

// 简化的鬼魂追击
int hunt_nearest_ghost(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (player->your_status <= 0) return 0;  // 没有强化状态
    
    // 找最近的鬼魂
    int target_ghost = -1;
    int min_dist = INF;
    
    for (int i = 0; i < 2; i++) {
        int dist = abs(sx - player->ghost_posx[i]) + abs(sy - player->ghost_posy[i]);
        if (dist < min_dist) {
            min_dist = dist;
            target_ghost = i;
        }
    }
    
    if (target_ghost == -1) return 0;
    
    int ghost_x = player->ghost_posx[target_ghost];
    int ghost_y = player->ghost_posy[target_ghost];
    
    // 简单追击：选择最接近鬼魂的方向
    int best_dir = -1;
    int min_new_dist = INF;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (is_valid(player, nx, ny)) {
            int new_dist = abs(nx - ghost_x) + abs(ny - ghost_y);
            if (new_dist < min_new_dist) {
                min_new_dist = new_dist;
                best_dir = d;
            }
        }
    }
    
    if (best_dir != -1) {
        *next_x = sx + dx[best_dir];
        *next_y = sy + dy[best_dir];
        return 1;
    }
    
    return 0;
}

// 安全移动选择
int choose_safe_move(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_dir = -1;
    int max_safety = -1;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        int safety = distance_to_ghost(player, nx, ny);
        
        // 避免回头
        if (nx == last_x && ny == last_y) {
            safety -= 10;
        }
        
        // 如果位置上有星星，增加吸引力
        if (is_star(player, nx, ny)) {
            char c = player->mat[nx][ny];
            safety += (c == 'O') ? 50 : 20;
        }
        
        if (safety > max_safety) {
            max_safety = safety;
            best_dir = d;
        }
    }
    
    if (best_dir != -1) {
        *next_x = x + dx[best_dir];
        *next_y = y + dy[best_dir];
        return 1;
    }
    
    return 0;
}

// 主决策函数
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // 标记当前星星为已吃
    char current_cell = player->mat[x][y];
    if ((current_cell == 'o' || current_cell == 'O') && !star_eaten[x][y]) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // 策略1: 强化状态下追击鬼魂
    if (player->your_status > 0) {
        int remaining_time = player->your_status;
        int nearest_ghost_dist = distance_to_ghost(player, x, y);
        
        // 如果时间充足且鬼魂不太远，积极追击
        if (remaining_time >= 5 || nearest_ghost_dist <= 6) {
            if (hunt_nearest_ghost(player, x, y, &next_x, &next_y)) {
                last_x = x; 
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
        
        // 时间不足时收集附近星星
        if (remaining_time <= POWER_TIME_LOW) {
            if (bfs_to_best_star(player, x, y, &next_x, &next_y)) {
                last_x = x; 
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // 策略2: 收集最佳星星
    if (bfs_to_best_star(player, x, y, &next_x, &next_y)) {
        last_x = x; 
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略3: 安全移动
    if (choose_safe_move(player, x, y, &next_x, &next_y)) {
        last_x = x; 
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 最后手段：随机选择有效方向
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            last_x = x; 
            last_y = y;
            struct Point ret = {nx, ny};
            return ret;
        }
    }
    
    // 无法移动，返回原位置
    last_x = x; 
    last_y = y;
    struct Point ret = {x, y};
    return ret;
}