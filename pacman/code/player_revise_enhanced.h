#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// **修复版刷分专用常量** - 更保守的参数设置
#define SUPER_STAR_VALUE 1200.0f     // 回退到稳定值
#define NORMAL_STAR_VALUE 100.0f     // 回退到稳定值
#define GHOST_HUNT_VALUE 600.0f      // 回退到稳定值

// **修复版刷分专用常量**
#define AGGRESSIVE_MODE_THRESHOLD 8000  // 提高激进模式门槛
#define SAFETY_DISTANCE_REDUCTION 1     // 减少激进程度
#define BFS_SEARCH_DEPTH 150            // 回退到稳定深度
#define STAR_CLUSTER_BONUS 200.0f       // 降低集群奖励

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;
static int aggressive_mode = 0;

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// **修复版初始化函数**
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
    aggressive_mode = 0;
}

// **修复版基础函数** - 增强边界检查
int is_valid(struct Player *player, int x, int y) {
    if (player == NULL) return 0;
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    if (player == NULL || x < 0 || x >= MAXN || y < 0 || y >= MAXN) return 0;
    if (x >= player->row_cnt || y >= player->col_cnt) return 0;
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// **修复版对手位置更新**
void update_opponent_quick(struct Player *player) {
    if (player == NULL) return;
    opponent_x = player->opponent_posx;
    opponent_y = player->opponent_posy;
    
    // **修复**: 更保守的激进模式
    aggressive_mode = (player->your_score >= AGGRESSIVE_MODE_THRESHOLD);
}

// **修复版距离计算** - 防止溢出
int distance_to_ghost(struct Player *player, int x, int y) {
    if (player == NULL) return INF;
    
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_dist) {
            min_dist = dist;
        }
    }
    
    // **修复**: 移除可能导致溢出的操作
    return min_dist;
}

// **修复版BFS算法** - 增强稳定性
int simple_bfs(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    if (player == NULL || next_x == NULL || next_y == NULL) return 0;
    
    // 边界检查
    if (sx < 0 || sx >= player->row_cnt || sy < 0 || sy >= player->col_cnt) return 0;
    if (target_x < 0 || target_x >= player->row_cnt || target_y < 0 || target_y >= player->col_cnt) return 0;
    if (sx == target_x && sy == target_y) return 0;
    
    int vis[MAXN][MAXN] = {0};
    int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    int qx[500], qy[500], head = 0, tail = 0;  // **修复**: 回退到稳定队列大小
    
    qx[tail] = sx;
    qy[tail] = sy;
    tail++;
    vis[sx][sy] = 1;
    parent_x[sx][sy] = -1;
    parent_y[sx][sy] = -1;
    
    while (head < tail && head < BFS_SEARCH_DEPTH) {
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
                if (temp_x == -1 || temp_y == -1) break;
                px = temp_x;
                py = temp_y;
            }
            
            if (backtrack_count >= 100) return 0;
            
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

// **修复版星星集群检测函数**
int count_nearby_stars(struct Player *player, int x, int y, int radius) {
    if (player == NULL) return 0;
    
    int count = 0;
    int start_i = (x - radius > 0) ? x - radius : 0;
    int end_i = (x + radius < player->row_cnt - 1) ? x + radius : player->row_cnt - 1;
    int start_j = (y - radius > 0) ? y - radius : 0;
    int end_j = (y + radius < player->col_cnt - 1) ? y + radius : player->col_cnt - 1;
    
    for (int i = start_i; i <= end_i; i++) {
        for (int j = start_j; j <= end_j; j++) {
            if (is_star(player, i, j)) {
                count++;
            }
        }
    }
    return count;
}

// **修复版星星评估函数** - 防止数值溢出
float evaluate_star_enhanced(struct Player *player, int sx, int sy, int star_x, int star_y) {
    if (player == NULL) return -INF;
    
    char star_type = player->mat[star_x][star_y];
    int my_dist = abs(star_x - sx) + abs(star_y - sy);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    
    // **修复**: 基础价值回退到稳定值
    float value = (star_type == 'O') ? SUPER_STAR_VALUE : NORMAL_STAR_VALUE;
    
    // **修复**: 更保守的分数导向调整
    if (player->your_score < 2000) {
        value *= 2.0f;  // 降低倍数
    } else if (player->your_score < 4000) {
        value *= 1.5f;
    } else if (player->your_score < 8000) {
        value *= 1.2f;
    } else if (aggressive_mode) {
        value *= 1.1f;  // 降低激进模式倍数
    }
    
    // **修复**: 距离惩罚
    value -= my_dist * 5.0f;
    
    // **修复**: 安全评估
    int safety_threshold = aggressive_mode ? 2 : 3;
    if (ghost_dist > safety_threshold) {
        value += 200.0f;
    } else if (ghost_dist <= 1 && player->your_status <= 0) {
        value -= 800.0f;
    }
    
    // **修复**: 超级星奖励
    if (star_type == 'O' && player->your_status <= 0) {
        value += 400.0f;
    }
    
    // **修复**: 地图特征奖励
    if (player->row_cnt <= 12 && player->col_cnt <= 12) {
        value += 150.0f;
    }
    
    // **修复**: 星星集群奖励
    int nearby_stars = count_nearby_stars(player, star_x, star_y, 2);
    if (nearby_stars > 1) {
        value += STAR_CLUSTER_BONUS * (nearby_stars - 1);
    }
    
    // **修复**: 竞争意识
    if (opponent_x != -1 && opponent_y != -1) {
        int opp_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        if (my_dist < opp_dist && star_type == 'O') {
            value += 400.0f;
        } else if (my_dist - opp_dist > 6) {
            value *= 0.8f;
        }
    }
    
    // **修复**: 强化状态奖励
    if (player->your_status > 0) {
        value += 200.0f;  // 降低奖励
    }
    
    return value;
}

// **修复版星星搜索**
int find_best_star(struct Player *player, int sx, int sy, int *best_x, int *best_y) {
    if (player == NULL || best_x == NULL || best_y == NULL) return 0;
    
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

// **修复版鬼魂追击**
int hunt_ghost(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (player == NULL || next_x == NULL || next_y == NULL) return 0;
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
    
    // **修复**: 更保守的追击条件
    int hunt_threshold = aggressive_mode ? 10 : 8;
    if (player->your_status >= 3 || min_dist <= hunt_threshold) {
        return simple_bfs(player, sx, sy, 
                         player->ghost_posx[target_ghost], 
                         player->ghost_posy[target_ghost], 
                         next_x, next_y);
    }
    
    return 0;
}

// **修复版安全移动**
int safe_move(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (player == NULL || next_x == NULL || next_y == NULL) return 0;
    
    int best_dir = -1;
    int max_safety = -1;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        int safety = distance_to_ghost(player, nx, ny);
        
        // **修复**: 星星加分
        if (is_star(player, nx, ny)) {
            char star_type = player->mat[nx][ny];
            safety += (star_type == 'O') ? 15 : 8;
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

// **修复版主决策函数** - 增强稳定性
struct Point walk(struct Player *player) {
    if (player == NULL) {
        struct Point ret = {0, 0};
        return ret;
    }
    
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // 状态更新
    update_opponent_quick(player);
    
    // 星星状态更新
    if (is_star(player, x, y) && x >= 0 && x < MAXN && y >= 0 && y < MAXN) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // **策略1**: 鬼魂追击
    if (player->your_status > 0) {
        if (hunt_ghost(player, x, y, &next_x, &next_y)) {
            last_x = x;
            last_y = y;
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // **策略2**: 星星收集
    int target_x, target_y;
    if (find_best_star(player, x, y, &target_x, &target_y)) {
        if (simple_bfs(player, x, y, target_x, target_y, &next_x, &next_y)) {
            // **修复**: 更保守的安全策略
            int required_dist = aggressive_mode ? 2 : 3;
            if (distance_to_ghost(player, next_x, next_y) >= required_dist || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // **策略3**: 安全移动
    if (safe_move(player, x, y, &next_x, &next_y)) {
        last_x = x;
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 最后手段
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

// **修复版刷分专用优化** - 增强稳定性
// 主要修复：
// 1. 增强边界检查和空指针检查
// 2. 防止数值溢出
// 3. 回退到稳定的参数设置
// 4. 更保守的策略设置
// 5. 增强错误处理
// 结果：预期通过所有测试用例
// 时间：2025-07-16 10:30:00
// 专门为稳定性优化