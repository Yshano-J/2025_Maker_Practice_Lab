#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// 精准优化的常量 - 基于成功版本微调
#define SUPER_STAR_VALUE 1200.0f     // 800→1200 提升50%超级星价值
#define NORMAL_STAR_VALUE 100.0f     // 保持不变，维持12:1黄金比例
#define GHOST_HUNT_VALUE 600.0f      // 保持成功参数

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;  // 轻量级对手追踪

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// 保持成功的初始化
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
}

// 保持成功的基础函数
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    if (x < 0 || x >= MAXN || y < 0 || y >= MAXN) return 0;
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// 轻量级对手位置更新
void update_opponent_quick(struct Player *player) {
    opponent_x = player->opponent_posx;
    opponent_y = player->opponent_posy;
}

// 保持成功的距离计算
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

// 保持成功的BFS算法，只加安全修复
int simple_bfs(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    // 边界检查
    if (sx < 0 || sx >= player->row_cnt || sy < 0 || sy >= player->col_cnt) return 0;
    if (target_x < 0 || target_x >= player->row_cnt || target_y < 0 || target_y >= player->col_cnt) return 0;
    if (sx == target_x && sy == target_y) return 0;
    
    int vis[MAXN][MAXN] = {0};
    int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    int qx[500], qy[500], head = 0, tail = 0;
    
    qx[tail] = sx;
    qy[tail] = sy;
    tail++;
    vis[sx][sy] = 1;
    parent_x[sx][sy] = -1;
    parent_y[sx][sy] = -1;
    
    while (head < tail && head < 150) {  // 保持成功的搜索深度150
        int x = qx[head], y = qy[head];
        head++;
        
        if (x == target_x && y == target_y) {
            // 安全回溯，找到从起点到目标的第一步
            int px = x, py = y;
            int backtrack_count = 0;
            
            // 回溯到起点，找到第一步
            while ((parent_x[px][py] != sx || parent_y[px][py] != sy) && backtrack_count < 100) {
                backtrack_count++;
                int temp_x = parent_x[px][py];
                int temp_y = parent_y[px][py];
                if (temp_x == -1 || temp_y == -1) break; // 防止无效父节点
                px = temp_x;
                py = temp_y;
            }
            
            if (backtrack_count >= 100) return 0; // 防止死循环
            
            // 如果回溯成功，返回第一步
            if (parent_x[px][py] == sx && parent_y[px][py] == sy) {
                *next_x = px;
                *next_y = py;
                return 1;
            }
            
            return 0;
        }
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny] && tail < 499 && nx < MAXN && ny < MAXN) {
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

// **核心优化**: 增强的星星评估函数
float evaluate_star_enhanced(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int my_dist = abs(star_x - sx) + abs(star_y - sy);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    
    // 基础价值 - 保持成功的价值体系
    float value = (star_type == 'O') ? SUPER_STAR_VALUE : NORMAL_STAR_VALUE;
    
    // **关键优化1**: 分数导向动态调整
    if (player->your_score < 2000) {
        value *= 2.5f;  // 极低分时大幅提升价值
    } else if (player->your_score < 4000) {
        value *= 1.8f;  // 低分时提升价值
    } else if (player->your_score < 8000) {
        value *= 1.3f;  // 中等分数时适度提升
    }
    
    // 保持成功的距离惩罚
    value -= my_dist * 5.0f;
    
    // 保持成功的安全评估
    if (ghost_dist > 3) {
        value += 200.0f;
    } else if (ghost_dist <= 1 && player->your_status <= 0) {
        value -= 800.0f;
    }
    
    // 保持成功的超级星奖励
    if (star_type == 'O' && player->your_status <= 0) {
        value += 400.0f;
    }
    
    // 保持成功的小地图奖励
    if (player->row_cnt <= 12 && player->col_cnt <= 12) {
        value += 150.0f;
    }
    
    // **关键优化2**: 轻量级竞争意识
    if (opponent_x != -1 && opponent_y != -1) {
        int opp_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        if (my_dist < opp_dist && star_type == 'O') {
            value += 400.0f;  // 抢夺超级星奖励
        } else if (my_dist - opp_dist > 6) {
            value *= 0.8f;   // 过远目标适度降低
        }
    }
    
    return value;
}

// 保持成功的星星搜索
int find_best_star(struct Player *player, int sx, int sy, int *best_x, int *best_y) {
    float best_value = -INF;
    int found = 0;
    
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                float value = evaluate_star_enhanced(player, sx, sy, i, j);
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

// 保持成功的鬼魂追击
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
    
    // 保持成功的追击条件
    if (player->your_status >= 3 || min_dist <= 8) {
        return simple_bfs(player, sx, sy, 
                         player->ghost_posx[target_ghost], 
                         player->ghost_posy[target_ghost], 
                         next_x, next_y);
    }
    
    return 0;
}

// 保持成功的安全移动
int safe_move(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_dir = -1;
    int max_safety = -1;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        int safety = distance_to_ghost(player, nx, ny);
        
        // 保持成功的星星加分
        if (is_star(player, nx, ny)) {
            safety += (player->mat[nx][ny] == 'O') ? 15 : 8;
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

// **关键优化3**: 增强的主决策函数
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // 轻量级状态更新
    update_opponent_quick(player);
    
    // 保持成功的星星状态更新
    if (is_star(player, x, y) && x >= 0 && x < MAXN && y >= 0 && y < MAXN) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // 策略1: 保持成功的鬼魂追击
    if (player->your_status > 0) {
        if (hunt_ghost(player, x, y, &next_x, &next_y)) {
            last_x = x;
            last_y = y;
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // 策略2: 增强的星星收集
    int target_x, target_y;
    if (find_best_star(player, x, y, &target_x, &target_y)) {
        if (simple_bfs(player, x, y, target_x, target_y, &next_x, &next_y)) {
            // **优化**: 分数导向的安全策略
            int required_dist = (player->your_score < 3000) ? 1 : 2;
            if (distance_to_ghost(player, next_x, next_y) >= required_dist || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // 策略3: 保持成功的安全移动
    if (safe_move(player, x, y, &next_x, &next_y)) {
        last_x = x;
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 保持成功的最后手段
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            last_x = x;
            last_y = y;
            struct Point ret = {nx, ny};
            return ret;
        }
    }
    
    // 保持成功的保护机制
    last_x = x;
    last_y = y;
    struct Point ret = {x, y};
    return ret;
}

// 基于成功版本的精准增强 - 保持所有成功要素，只优化关键瓶颈 
// 结果：成功
// 时间：2025-07-15 22:20:00
// 目前最最强