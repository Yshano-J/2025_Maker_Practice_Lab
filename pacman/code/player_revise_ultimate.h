#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// 优化后的常量 - 平衡性能和稳定性
#define SUPER_STAR_VALUE 1000.0f     // 超级星价值
#define NORMAL_STAR_VALUE 100.0f     // 普通星价值
#define GHOST_HUNT_VALUE 600.0f      // 鬼魂追击价值

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// 初始化函数
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
}

// 基础验证函数
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// 修复的对手位置更新 - 更安全可靠
void update_opponent_quick(struct Player *player) {
    opponent_x = -1;
    opponent_y = -1;
    
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            char c = player->mat[i][j];
            // 检查是否为对手位置（排除自己和鬼魂）
            if (c != 'O' && c != 'o' && c != '#' && c != '.') {
                if (!(i == player->your_posx && j == player->your_posy)) {
                    // 排除鬼魂位置
                    int is_ghost = 0;
                    for (int k = 0; k < 2; k++) {
                        if (i == player->ghost_posx[k] && j == player->ghost_posy[k]) {
                            is_ghost = 1;
                            break;
                        }
                    }
                    if (!is_ghost) {
                        opponent_x = i;
                        opponent_y = j;
                        return;
                    }
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

// 修复的BFS算法 - 增强边界检查和安全性
int simple_bfs(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    // 边界检查
    if (sx < 0 || sx >= player->row_cnt || sy < 0 || sy >= player->col_cnt) return 0;
    if (target_x < 0 || target_x >= player->row_cnt || target_y < 0 || target_y >= player->col_cnt) return 0;
    if (sx == target_x && sy == target_y) return 0;
    
    int vis[MAXN][MAXN] = {0};
    int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    int qx[500], qy[500], head = 0, tail = 0;
    
    // 初始化
    qx[tail] = sx;
    qy[tail] = sy;
    tail++;
    vis[sx][sy] = 1;
    parent_x[sx][sy] = -1;
    parent_y[sx][sy] = -1;
    
    int max_iterations = 120; // 减少迭代次数提高性能
    int iterations = 0;
    
    while (head < tail && iterations < max_iterations) {
        int x = qx[head], y = qy[head];
        head++;
        iterations++;
        
        if (x == target_x && y == target_y) {
            // 安全回溯
            int px = x, py = y;
            int backtrack_count = 0;
            while ((parent_x[px][py] != sx || parent_y[px][py] != sy) && backtrack_count < 100) {
                backtrack_count++;
                int temp_x = parent_x[px][py];
                int temp_y = parent_y[px][py];
                if (temp_x == -1 || temp_y == -1) break; // 防止无效父节点
                px = temp_x;
                py = temp_y;
            }
            if (backtrack_count >= 100) return 0; // 防止死循环
            
            *next_x = px;
            *next_y = py;
            return 1;
        }
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny] && tail < 499) {
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

// 简化的星星评估函数 - 避免复杂计算导致超时
float evaluate_star_simple(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int my_dist = abs(star_x - sx) + abs(star_y - sy);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    
    // 基础价值
    float value = (star_type == 'O') ? SUPER_STAR_VALUE : NORMAL_STAR_VALUE;
    
    // 距离惩罚
    value -= my_dist * 5.0f;
    
    // 安全奖励
    if (ghost_dist > 3) {
        value += 200.0f;
    } else if (ghost_dist <= 1 && player->your_status <= 0) {
        value -= 600.0f;
    }
    
    // 超级星额外奖励
    if (star_type == 'O' && player->your_status <= 0) {
        value += 400.0f;
    }
    
    // 小地图奖励
    if (player->row_cnt <= 12 && player->col_cnt <= 12) {
        value += 100.0f;
    }
    
    // 竞争意识
    if (opponent_x != -1) {
        int opp_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        if (my_dist < opp_dist && star_type == 'O') {
            value += 300.0f;
        }
    }
    
    return value;
}

// 星星搜索
int find_best_star(struct Player *player, int sx, int sy, int *best_x, int *best_y) {
    float best_value = -INF;
    int found = 0;
    
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                float value = evaluate_star_simple(player, sx, sy, i, j);
                if (value > best_value) {
                    best_value = value;
                    *best_x = i;
                    *best_y = j;
                    found = 1;
                }
            }
        }
    }
    
    return found;
}

// 鬼魂追击
int hunt_ghost(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (player->your_status <= 0) return 0;
    
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
    
    // 追击条件
    if (player->your_status >= 3 || min_dist <= 8) {
        return simple_bfs(player, sx, sy, 
            player->ghost_posx[target_ghost], 
            player->ghost_posy[target_ghost], 
            next_x, next_y);
    }
    
    return 0;
}

// 安全移动
int safe_move(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_dir = -1;
    float max_score = -1;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        float score = distance_to_ghost(player, nx, ny);
        
        // 星星加分
        if (is_star(player, nx, ny)) {
            score += (player->mat[nx][ny] == 'O') ? 15 : 8;
        }
        
        // 移动性评估
        int valid_exits = 0;
        for (int dd = 0; dd < 4; dd++) {
            if (is_valid(player, nx + dx[dd], ny + dy[dd])) {
                valid_exits++;
            }
        }
        if (valid_exits == 1) {
            score -= 5.0f;  // 惩罚死路
        } else if (valid_exits >= 3) {
            score += 1.0f;  // 奖励高移动性
        }
        
        if (score > max_score) {
            max_score = score;
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

// 主决策函数
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // 更新对手位置
    update_opponent_quick(player);
    
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
    
    // 策略2: 收集星星
    int target_x, target_y;
    if (find_best_star(player, x, y, &target_x, &target_y)) {
        if (simple_bfs(player, x, y, target_x, target_y, &next_x, &next_y)) {
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
    
    // 保护机制
    last_x = x;
    last_y = y;
    struct Point ret = {x, y};
    return ret;
} 