#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// 极限高分常量
#define ULTRA_TARGET_SCORE 10000
#define EXTREME_LOW_THRESHOLD 1500    // 1500分以下极度激进
#define ULTRA_LOW_THRESHOLD 3000      // 3000分以下超级激进
#define HIGH_PERFORMANCE_THRESHOLD 6000  // 6000分以上保持攻击性

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int consecutive_low_score = 0;

// 极限时间优化 - 100ms满载运行
static clock_t start_time;
static const int MAX_TIME_LIMIT_MS = 90;  

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// 高性能路径追踪
static int elite_path_x[50], elite_path_y[50];
static int elite_path_count = 0;
static int super_target_x = -1, super_target_y = -1;
static int target_lock_duration = 0;

// 鬼魂预测
static int ghost_predicted_x[2][5], ghost_predicted_y[2][5];

int is_timeout() {
    clock_t current_time = clock();
    double elapsed_ms = ((double)(current_time - start_time)) / CLOCKS_PER_SEC * 1000;
    return elapsed_ms > MAX_TIME_LIMIT_MS;
}

void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    elite_path_count = 0;
    super_target_x = -1;
    super_target_y = -1;
    target_lock_duration = 0;
    consecutive_low_score = 0;
}

int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// 高性能路径记录
void record_elite_path(int x, int y) {
    if (elite_path_count >= 50) {
        for (int i = 0; i < 49; i++) {
            elite_path_x[i] = elite_path_x[i + 1];
            elite_path_y[i] = elite_path_y[i + 1];
        }
        elite_path_count = 49;
    }
    elite_path_x[elite_path_count] = x;
    elite_path_y[elite_path_count] = y;
    elite_path_count++;
}

// 检查最近路径重复
int recent_path_penalty(int x, int y) {
    int penalty = 0;
    int check_recent = min(8, elite_path_count);
    for (int i = elite_path_count - check_recent; i < elite_path_count; i++) {
        if (elite_path_x[i] == x && elite_path_y[i] == y) {
            penalty += (10 - (elite_path_count - i)) * 30;
        }
    }
    return penalty;
}

// 多步骤鬼魂行为预测
void predict_ghost_moves(struct Player *player) {
    for (int ghost_id = 0; ghost_id < 2; ghost_id++) {
        int gx = player->ghost_posx[ghost_id];
        int gy = player->ghost_posy[ghost_id];
        
        ghost_predicted_x[ghost_id][0] = gx;
        ghost_predicted_y[ghost_id][0] = gy;
        
        // 预测4步
        for (int step = 1; step <= 4; step++) {
            // 简化：假设鬼魂向玩家移动
            int target_x = player->your_posx;
            int target_y = player->your_posy;
            
            int best_dist = INF;
            int next_gx = gx, next_gy = gy;
            
            for (int d = 0; d < 4; d++) {
                int new_gx = gx + dx[d];
                int new_gy = gy + dy[d];
                
                if (is_valid(player, new_gx, new_gy)) {
                    int dist = abs(new_gx - target_x) + abs(new_gy - target_y);
                    if (dist < best_dist) {
                        best_dist = dist;
                        next_gx = new_gx;
                        next_gy = new_gy;
                    }
                }
            }
            
            ghost_predicted_x[ghost_id][step] = next_gx;
            ghost_predicted_y[ghost_id][step] = next_gy;
            gx = next_gx;
            gy = next_gy;
        }
    }
}

// 极限安全检查 - 根据分数动态调整
int is_extremely_safe(struct Player *player, int x, int y, int future_step) {
    if (!is_valid(player, x, y)) return 0;
    
    // 强化状态：完全激进
    if (player->your_status > 0) {
        if (player->your_status > 5) return 1;  // 时间充足完全激进
        // 即使时间不足也要激进，只避免直接碰撞
        for (int i = 0; i < 2; i++) {
            if (x == player->ghost_posx[i] && y == player->ghost_posy[i]) return 0;
        }
        return 1;
    }
    
    // 根据分数调整安全距离
    int required_distance;
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        required_distance = 1;  // 分数极低时只要求1格距离
        consecutive_low_score++;
    } else if (player->your_score < ULTRA_LOW_THRESHOLD) {
        required_distance = 2;  // 分数较低时要求2格距离
        consecutive_low_score = max(0, consecutive_low_score - 1);
    } else {
        required_distance = 3;  // 分数正常时要求3格距离
        consecutive_low_score = 0;
    }
    
    // 连续低分时进一步降低安全要求
    if (consecutive_low_score > 10) {
        required_distance = max(1, required_distance - 1);
    }
    
    // 检查预测的鬼魂位置
    for (int i = 0; i < 2; i++) {
        int step = min(future_step, 4);
        int ghost_x = ghost_predicted_x[i][step];
        int ghost_y = ghost_predicted_y[i][step];
        int dist = abs(x - ghost_x) + abs(y - ghost_y);
        if (dist < required_distance) return 0;
    }
    
    return 1;
}

// 终极评分函数
float ultimate_score_move(struct Player *player, int from_x, int from_y, int to_x, int to_y) {
    if (!is_valid(player, to_x, to_y)) return -100000.0f;
    
    float score = 0.0f;
    
    // 1. 超级星星价值 - 根据分数动态调整
    if (is_star(player, to_x, to_y)) {
        char star_type = player->mat[to_x][to_y];
        if (star_type == 'O') {
            score += 3000.0f;  // 大星星超高价值
        } else {
            score += 800.0f;   // 小星星高价值
        }
        
        // 分数不足时大幅提升星星价值
        if (player->your_score < EXTREME_LOW_THRESHOLD) {
            score *= 3.0f;
        } else if (player->your_score < ULTRA_LOW_THRESHOLD) {
            score *= 2.0f;
        }
    }
    
    // 2. 安全距离优化 - 更智能的风险评估
    if (!is_extremely_safe(player, to_x, to_y, 0)) {
    if (player->your_status > 0) {
            score += 500.0f;  // 强化状态下鼓励冒险
        } else {
            // 分数导向的风险调整
            if (player->your_score < EXTREME_LOW_THRESHOLD) {
                score -= 200.0f;  // 极低分时适度惩罚危险
            } else {
                score -= 2000.0f; // 正常分数时严重惩罚危险
            }
        }
    }
    
    // 3. 附近星星聚合度
    int nearby_stars = 0;
    for (int radius = 1; radius <= 5; radius++) {
        for (int i = max(0, to_x - radius); i <= min(player->row_cnt - 1, to_x + radius); i++) {
            for (int j = max(0, to_y - radius); j <= min(player->col_cnt - 1, to_y + radius); j++) {
                if (is_star(player, i, j)) {
                    int dist = abs(i - to_x) + abs(j - to_y);
                    if (dist <= radius) {
                        nearby_stars++;
                        score += (400.0f / (dist + 1)) * (6 - radius) / 5.0f;
                    }
                }
            }
        }
    }
    
    // 4. 对手干扰 - 大幅强化
    float interference_value = 0.0f;
    if (player->opponent_score > 6000) {
        interference_value = 1000.0f;  // 高分对手强力干扰
    } else if (player->opponent_score > 3000) {
        interference_value = 600.0f;
    } else {
        interference_value = 300.0f;
    }
    
    int opp_dist = abs(to_x - player->opponent_posx) + abs(to_y - player->opponent_posy);
    if (opp_dist <= 3) {
        score += interference_value / (opp_dist + 1);
    }
    
    // 5. 路径历史惩罚 - 强化版
    int path_penalty = recent_path_penalty(to_x, to_y);
    score -= path_penalty;
    
    // 6. 开阔区域和移动性
    int mobility = 0;
    for (int d = 0; d < 4; d++) {
        int check_x = to_x + dx[d];
        int check_y = to_y + dy[d];
        if (is_valid(player, check_x, check_y)) {
            mobility++;
        }
    }
    score += mobility * 100.0f;
    
    // 7. 分数压力系统
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        score += 1000.0f;  // 极低分数时激进加分
        if (nearby_stars > 0) {
            score += 2000.0f;  // 有星星时超级加分
        }
    }
    
    // 8. 目标持续性奖励
    if (to_x == super_target_x && to_y == super_target_y) {
        score += 300.0f * target_lock_duration;
    }
    
    return score;
}

// 超级A*寻路
int super_astar_pathfind(struct Player *player, int start_x, int start_y, int target_x, int target_y, int *next_x, int *next_y) {
    if (is_timeout()) return 0;
    
    static int g_cost[MAXN][MAXN], f_cost[MAXN][MAXN];
    static int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    static int open_x[500], open_y[500];  // 开放列表
    static int closed[MAXN][MAXN];
    
    // 初始化
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            g_cost[i][j] = INF;
            f_cost[i][j] = INF;
            parent_x[i][j] = -1;
            parent_y[i][j] = -1;
            closed[i][j] = 0;
        }
    }
    
    g_cost[start_x][start_y] = 0;
    f_cost[start_x][start_y] = abs(start_x - target_x) + abs(start_y - target_y);
    
    int open_size = 1;
    open_x[0] = start_x;
    open_y[0] = start_y;
    
    // 动态迭代次数
    int max_iterations = 150;
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        max_iterations = 200;  // 低分时增加计算量
    }
    
    int iterations = 0;
    
    while (open_size > 0 && iterations < max_iterations && !is_timeout()) {
        iterations++;
        
        // 找最优节点
        int best_idx = 0;
        for (int i = 1; i < open_size; i++) {
            if (f_cost[open_x[i]][open_y[i]] < f_cost[open_x[best_idx]][open_y[best_idx]]) {
                best_idx = i;
            }
        }
        
        int current_x = open_x[best_idx];
        int current_y = open_y[best_idx];
        
        // 移除当前节点
        for (int i = best_idx; i < open_size - 1; i++) {
            open_x[i] = open_x[i + 1];
            open_y[i] = open_y[i + 1];
        }
        open_size--;
        
        closed[current_x][current_y] = 1;
        
        // 找到目标
        if (current_x == target_x && current_y == target_y) {
            int path_x = target_x, path_y = target_y;
            while (parent_x[path_x][path_y] != -1) {
                int px = parent_x[path_x][path_y];
                int py = parent_y[path_x][path_y];
                
                if (px == start_x && py == start_y) {
                    *next_x = path_x;
                    *next_y = path_y;
                    return 1;
                }
                
                path_x = px;
                path_y = py;
            }
        }
        
        // 扩展邻居
        for (int d = 0; d < 4; d++) {
            int nx = current_x + dx[d];
            int ny = current_y + dy[d];
            
            if (!is_valid(player, nx, ny) || closed[nx][ny]) continue;
            
            // 安全检查 - 更宽松
            if (!is_extremely_safe(player, nx, ny, 1) && player->your_status == 0) {
                if (player->your_score >= ULTRA_LOW_THRESHOLD) continue;
            }
            
            int tentative_g = g_cost[current_x][current_y] + 1;
            
            if (tentative_g < g_cost[nx][ny]) {
                parent_x[nx][ny] = current_x;
                parent_y[nx][ny] = current_y;
                g_cost[nx][ny] = tentative_g;
                f_cost[nx][ny] = tentative_g + abs(nx - target_x) + abs(ny - target_y);
                
                // 添加到开放列表
                int found = 0;
                for (int i = 0; i < open_size; i++) {
                    if (open_x[i] == nx && open_y[i] == ny) {
                        found = 1;
                        break;
                    }
                }
                
                if (!found && open_size < 500) {
                    open_x[open_size] = nx;
                    open_y[open_size] = ny;
                    open_size++;
                }
            }
        }
    }
    
    return 0;
}

struct Point walk(struct Player *player) {
    start_time = clock();
    step_count++;
    
    int x = player->your_posx;
    int y = player->your_posy;
    
    // 更新路径历史
    record_elite_path(x, y);
    
    // 预测鬼魂移动
    predict_ghost_moves(player);
    
    // 标记吃掉的星星
    if (last_x != -1 && last_y != -1 && is_star(player, last_x, last_y)) {
        star_eaten[last_x][last_y] = 1;
    }
    
    // 超级目标搜索 - 扩大范围和价值评估
    int best_target_x = -1, best_target_y = -1;
    float best_target_value = -1.0f;
    
    int search_radius = (player->your_score < EXTREME_LOW_THRESHOLD) ? 10 : 8;
    
    for (int i = max(0, x - search_radius); i <= min(player->row_cnt - 1, x + search_radius) && !is_timeout(); i++) {
        for (int j = max(0, y - search_radius); j <= min(player->col_cnt - 1, y + search_radius); j++) {
            if (!is_star(player, i, j)) continue;
            
            int dist = abs(i - x) + abs(j - y);
            if (dist == 0) continue;
            
            float value = 0.0f;
            
            // 星星基础价值
            char star_type = player->mat[i][j];
            if (star_type == 'O') {
                value = 5000.0f / (dist + 1);
            } else {
                value = 1500.0f / (dist + 1);
            }
            
            // 分数不足时提升价值
            if (player->your_score < EXTREME_LOW_THRESHOLD) {
                value *= 4.0f;
            } else if (player->your_score < ULTRA_LOW_THRESHOLD) {
                value *= 2.5f;
            }
            
            // 安全性考虑
            if (!is_extremely_safe(player, i, j, min(3, dist/3))) {
                if (player->your_status == 0) {
                    value *= 0.3f;  // 危险位置降低价值
                } else {
                    value *= 1.5f;  // 强化状态提升价值
                }
            }
            
            // 聚合奖励
            int nearby_count = 0;
            for (int di = i - 3; di <= i + 3; di++) {
                for (int dj = j - 3; dj <= j + 3; dj++) {
                    if (is_star(player, di, dj) && (di != i || dj != j)) {
                        nearby_count++;
                    }
                }
            }
            value += nearby_count * 200.0f;
            
            if (value > best_target_value) {
                best_target_value = value;
                best_target_x = i;
                best_target_y = j;
            }
        }
    }
    
    // 目标锁定系统
    if (best_target_x == super_target_x && best_target_y == super_target_y) {
        target_lock_duration++;
    } else {
        target_lock_duration = 0;
        super_target_x = best_target_x;
        super_target_y = best_target_y;
    }
    
    // 使用A*寻路到目标
    if (best_target_x != -1) {
        int next_x, next_y;
        if (super_astar_pathfind(player, x, y, best_target_x, best_target_y, &next_x, &next_y)) {
                struct Point ret = {next_x, next_y};
                return ret;
            }
    }
    
    // A*失败时使用终极贪心策略
    int final_x = x, final_y = y;
    float final_score = -1000000.0f;
    
    for (int d = 0; d < 4 && !is_timeout(); d++) {
        int nx = x + dx[d];
        int ny = y + dy[d];
        
        if (!is_valid(player, nx, ny)) continue;
        
        float score = ultimate_score_move(player, x, y, nx, ny);
        
        if (score > final_score) {
            final_score = score;
            final_x = nx;
            final_y = ny;
        }
    }
    
    // 绝对最后的安全移动
    if (final_x == x && final_y == y) {
    for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
                // 检查最低安全要求
                int min_dist = INF;
                for (int i = 0; i < 2; i++) {
                    int dist = abs(nx - player->ghost_posx[i]) + abs(ny - player->ghost_posy[i]);
                    min_dist = min(min_dist, dist);
                }
                
                int required = (player->your_score < EXTREME_LOW_THRESHOLD) ? 1 : 2;
                if (min_dist >= required || player->your_status > 0) {
                    final_x = nx;
                    final_y = ny;
                    break;
                }
            }
        }
    }
    
    // 最终保护
    if (final_x == x && final_y == y) {
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (is_valid(player, nx, ny)) {
                final_x = nx;
                final_y = ny;
                break;
            }
        }
    }
    
    last_x = x;
    last_y = y;
    
    struct Point ret = {final_x, final_y};
    return ret;
}
