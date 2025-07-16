#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// 微调后的高效常量
#define SUPER_STAR_VALUE 1000.0f     // 提升超级星价值
#define NORMAL_STAR_VALUE 120.0f     // 微调普通星价值
#define GHOST_HUNT_VALUE 700.0f      // 鬼魂追击价值

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;  // 添加对手追踪

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// 初始化
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
}

// 基础函数
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// 轻量级对手位置更新
void update_opponent(struct Player *player) {
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            char c = player->mat[i][j];
            if (c == 'A' || c == 'B') {
                if (!(i == player->your_posx && j == player->your_posy)) {
                    opponent_x = i;
                    opponent_y = j;
                    return;
                }
            }
        }
    }
}

// 计算到鬼魂的距离
int distance_to_ghost(struct Player *player, int x, int y) {
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_dist) {
            min_dist = dist;
        }
    }
    return min_dist;
}

// 改进的BFS寻路 - 保持简单但更稳健
int smart_bfs(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    if (sx == target_x && sy == target_y) return 0;
    
    int vis[MAXN][MAXN] = {0};
    int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    int qx[400], qy[400], head = 0, tail = 0;
    
    qx[tail] = sx;
    qy[tail] = sy;
    tail++;
    vis[sx][sy] = 1;
    parent_x[sx][sy] = -1;
    parent_y[sx][sy] = -1;
    
    while (head < tail && head < 120) {  // 适中的搜索深度
        int x = qx[head], y = qy[head];
        head++;
        
        if (x == target_x && y == target_y) {
            // 安全回溯找第一步
            int px = x, py = y;
            int backtrack = 0;
            while (backtrack < 100 && parent_x[px][py] != -1) {
                int temp_x = parent_x[px][py];
                int temp_y = parent_y[px][py];
                if (temp_x == sx && temp_y == sy) {
                    *next_x = px;
                    *next_y = py;
                    return 1;
                }
                px = temp_x;
                py = temp_y;
                backtrack++;
            }
            return 0;
        }
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny] && tail < 399) {
                vis[nx][ny] = 1;
                parent_x[nx][ny] = x;
                parent_y[nx][ny] = y;
                qx[tail] = nx;
                qy[tail] = ny;
                tail++;
            }
        }
    }
    
    return 0;
}

// 改进的星星评估 - 增加竞争意识
float evaluate_star_improved(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int my_dist = abs(star_x - sx) + abs(star_y - sy);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    
    // 基础价值
    float value = (star_type == 'O') ? SUPER_STAR_VALUE : NORMAL_STAR_VALUE;
    
    // 距离惩罚
    value -= my_dist * 5.0f;
    
    // 安全评估
    if (ghost_dist > 3) {
        value += 200.0f;
    } else if (ghost_dist <= 1 && player->your_status <= 0) {
        value -= 800.0f;  // 危险惩罚
    }
    
    // 超级星额外奖励
    if (star_type == 'O' && player->your_status <= 0) {
        value += 500.0f;  // 提升超级星重要性
    }
    
    // 小地图激进策略
    if (player->row_cnt <= 12 && player->col_cnt <= 12) {
        value += 200.0f;
    }
    
    // 轻量级竞争检测
    if (opponent_x != -1) {
        int opp_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        if (my_dist < opp_dist && star_type == 'O') {
            value += 300.0f;  // 抢夺超级星奖励
        }
    }
    
    // 分数导向调整
    if (player->your_score < 3000) {
        value *= 1.5f;  // 低分时更激进
    }
    
    return value;
}

// 寻找最佳星星 - 轻微优化
int find_best_star(struct Player *player, int sx, int sy, int *best_x, int *best_y) {
    float best_value = -INF;
    int found = 0;
    
    // 优先搜索附近区域
    int search_radius = 10;
    for (int i = max(0, sx - search_radius); i <= min(player->row_cnt - 1, sx + search_radius); i++) {
        for (int j = max(0, sy - search_radius); j <= min(player->col_cnt - 1, sy + search_radius); j++) {
            if (is_star(player, i, j)) {
                float value = evaluate_star_improved(player, sx, sy, i, j);
                if (value > best_value) {
                    best_value = value;
                    *best_x = i;
                    *best_y = j;
                    found = 1;
                }
            }
        }
    }
    
    // 如果附近没找到，再搜索全图
    if (!found) {
        for (int i = 0; i < player->row_cnt; i++) {
            for (int j = 0; j < player->col_cnt; j++) {
                if (is_star(player, i, j)) {
                    float value = evaluate_star_improved(player, sx, sy, i, j);
                    if (value > best_value) {
                        best_value = value;
                        *best_x = i;
                        *best_y = j;
                        found = 1;
                    }
                }
            }
        }
    }
    
    return found;
}

// 鬼魂追击 - 微调
int hunt_ghost(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (player->your_status <= 0) return 0;
    
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
    
    // 更智能的追击判断
    if (player->your_status >= 2 || min_dist <= 10) {
        return smart_bfs(player, sx, sy, 
                        player->ghost_posx[target_ghost], 
                        player->ghost_posy[target_ghost], 
                        next_x, next_y);
    }
    
    return 0;
}

// 安全移动 - 微调
int safe_move(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_dir = -1;
    float max_safety = -INF;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        float safety = distance_to_ghost(player, nx, ny);
        
        // 有星星的位置加分
        if (is_star(player, nx, ny)) {
            safety += (player->mat[nx][ny] == 'O') ? 20.0f : 10.0f;
        }
        
        // 避免重复位置
        if (nx == last_x && ny == last_y) {
            safety -= 5.0f;
        }
        
        if (safety > max_safety) {
            max_safety = safety;
            best_dir = d;
        }
    }
    
    if (best_dir != -1) {
        *next_x = sx + dx[best_dir];
        *next_y = sy + dy[best_dir];
        return 1;
    }
    
    return 0;
}

// 主决策函数 - 保持简洁但更智能
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // 轻量级状态更新
    update_opponent(player);
    
    // 更新星星状态
    if (is_star(player, x, y)) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // 策略1: 强化状态追击鬼魂
    if (player->your_status > 0) {
        if (hunt_ghost(player, x, y, &next_x, &next_y)) {
            last_x = x;
            last_y = y;
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // 策略2: 智能收集星星
    int target_x, target_y;
    if (find_best_star(player, x, y, &target_x, &target_y)) {
        if (smart_bfs(player, x, y, target_x, target_y, &next_x, &next_y)) {
            // 动态安全检查
            int required_dist = (player->your_score < 3000) ? 1 : 2;
            if (distance_to_ghost(player, next_x, next_y) >= required_dist || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // 策略3: 安全移动
    if (safe_move(player, x, y, &next_x, &next_y)) {
        last_x = x;
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 最后手段：任意有效移动
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            last_x = x;
            last_y = y;
            struct Point ret = {nx, ny};
            return ret;
        }
    }
    
    // 无法移动
    last_x = x;
    last_y = y;
    struct Point ret = {x, y};
    return ret;
}

// 基于简化版的精准优化 - 保持高效简洁的核心优势 
// 结果：失败