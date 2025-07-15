#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// 智能平衡版常量
#define SUPER_STAR_VALUE 1200.0f     // 超级星价值
#define NORMAL_STAR_VALUE 120.0f     // 普通星价值
#define GHOST_HUNT_VALUE 800.0f      // 鬼魂追击价值
#define COMPETITION_BONUS 500.0f     // 竞争奖励

// 简化的分数阈值
#define LOW_SCORE_THRESHOLD 3000     // 低分激进阈值

// 时间管理 - 更保守
static clock_t start_time;
static const int MAX_TIME_LIMIT_MS = 70;  // 70ms保守时间限制

// 游戏状态追踪
static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;
static int total_stars = 0;
static int game_phase = 0; // 0:early, 1:mid, 2:late

// 简化的高级特性
static int recent_positions[5][2];  // 仅保留5步历史
static int position_count = 0;

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// 简化的时间检查
int is_timeout() {
    clock_t current_time = clock();
    double elapsed_ms = ((double)(current_time - start_time)) / CLOCKS_PER_SEC * 1000;
    return elapsed_ms > MAX_TIME_LIMIT_MS;
}

// 初始化
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    memset(recent_positions, 0, sizeof(recent_positions));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
    position_count = 0;
    
    // 计算总星星数
    total_stars = 0;
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (player->mat[i][j] == 'o' || player->mat[i][j] == 'O') {
                total_stars++;
            }
        }
    }
}

// 基础函数
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// 简化的位置历史管理
void add_recent_position(int x, int y) {
    recent_positions[position_count % 5][0] = x;
    recent_positions[position_count % 5][1] = y;
    position_count++;
}

// 简化的重复检查 (修复版)
int is_recent_position(int x, int y) {
    if (position_count <= 0) return 0; // 防止未初始化访问
    
    int count = min(position_count, 5);
    int start_idx = (position_count >= 5) ? (position_count % 5) : 0;
    
    // 检查最近的位置
    for (int i = 0; i < count; i++) {
        int idx = (start_idx + i) % 5;
        if (recent_positions[idx][0] == x && recent_positions[idx][1] == y) {
            return 1;
        }
    }
    return 0;
}

// 快速对手位置更新
void update_opponent_position(struct Player *player) {
    for (int i = 0; i < player->row_cnt && !is_timeout(); i++) {
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

// 简化的游戏阶段更新
void update_game_phase(struct Player *player) {
    // 简单的阶段判断，避免遍历整个地图
    if (step_count < 50) {
        game_phase = 0; // 早期
    } else if (step_count < 150) {
        game_phase = 1; // 中期
    } else {
        game_phase = 2; // 后期
    }
}

// 计算到鬼魂的距离 (安全版)
int distance_to_ghost(struct Player *player, int x, int y) {
    if (!player) return INF;
    
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        // 添加边界检查
        if (player->ghost_posx[i] >= 0 && player->ghost_posx[i] < player->row_cnt &&
            player->ghost_posy[i] >= 0 && player->ghost_posy[i] < player->col_cnt) {
            int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
    }
    return min_dist;
}

// 智能但简化的安全评估
int is_safe_move(struct Player *player, int x, int y) {
    if (!is_valid(player, x, y)) return 0;
    
    // 强化状态：更激进
    if (player->your_status > 0) {
        // 避免直接碰撞即可
        for (int i = 0; i < 2; i++) {
            if (x == player->ghost_posx[i] && y == player->ghost_posy[i]) return 0;
        }
        return 1;
    }
    
    // 普通状态：动态安全距离
    int required_distance = (player->your_score < LOW_SCORE_THRESHOLD) ? 2 : 3;
    return distance_to_ghost(player, x, y) >= required_distance;
}

// 改进的BFS - 平衡效率和智能 (修复版)
int smart_bfs(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    if (sx == target_x && sy == target_y) return 0;
    if (is_timeout()) return 0;
    
    // 边界检查
    if (sx < 0 || sx >= player->row_cnt || sy < 0 || sy >= player->col_cnt) return 0;
    if (target_x < 0 || target_x >= player->row_cnt || target_y < 0 || target_y >= player->col_cnt) return 0;
    
    int vis[MAXN][MAXN] = {0};
    int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    int qx[500], qy[500], head = 0, tail = 0;  // 增加队列大小并添加边界检查
    
    qx[tail] = sx;
    qy[tail] = sy;
    tail++;
    vis[sx][sy] = 1;
    parent_x[sx][sy] = -1;
    parent_y[sx][sy] = -1;
    
    // 动态搜索深度
    int max_depth = (player->your_score < LOW_SCORE_THRESHOLD) ? 100 : 80;
    int iterations = 0; // 添加迭代计数器防止死循环
    
    while (head < tail && iterations < max_depth && !is_timeout()) {
        iterations++;
        int x = qx[head], y = qy[head];
        head++;
        
        if (x == target_x && y == target_y) {
            // 安全的回溯找第一步
            int px = x, py = y;
            int backtrack_count = 0; // 防止回溯死循环
            
            while (backtrack_count < 200 && parent_x[px][py] != -1 && parent_y[px][py] != -1) {
                int temp_x = parent_x[px][py];
                int temp_y = parent_y[px][py];
                
                // 如果父节点是起始点，当前点就是下一步
                if (temp_x == sx && temp_y == sy) {
                    *next_x = px;
                    *next_y = py;
                    return 1;
                }
                
                px = temp_x;
                py = temp_y;
                backtrack_count++;
            }
            
            // 如果回溯失败，返回0
            return 0;
        }
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny] && tail < 499) { // 添加队列边界检查
                // 简化的安全检查
                if (is_safe_move(player, nx, ny) || player->your_status > 0) {
                    vis[nx][ny] = 1;
                    parent_x[nx][ny] = x;
                    parent_y[nx][ny] = y;
                    qx[tail] = nx;
                    qy[tail] = ny;
                    tail++;
                }
            }
        }
    }
    
    return 0;
}

// 智能平衡的星星评估
float evaluate_star_balanced(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int my_dist = abs(star_x - sx) + abs(star_y - sy);
    
    // 基础价值
    float value = (star_type == 'O') ? SUPER_STAR_VALUE : NORMAL_STAR_VALUE;
    
    // 分数导向调整（简化）
    if (player->your_score < LOW_SCORE_THRESHOLD) {
        value *= 1.8f;  // 低分时提升价值
    }
    
    // 距离成本
    value -= my_dist * 8.0f;
    
    // 简化的对手竞争评估
    if (opponent_x != -1) {
        int opponent_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        if (my_dist < opponent_dist) {
            value += COMPETITION_BONUS * 0.5f;  // 简化计算
        } else if (my_dist - opponent_dist > 5) {
            value *= 0.7f;  // 太远的目标降低优先级
        }
    }
    
    // 安全性评估
    if (!is_safe_move(player, star_x, star_y)) {
        if (player->your_status > 0) {
            value += 200.0f;  // 强化状态鼓励冒险
        } else {
            value -= 800.0f;  // 危险大幅惩罚
        }
    }
    
    // 简化的聚合奖励（仅检查相邻）
    int nearby_stars = 0;
    for (int d = 0; d < 4; d++) {
        int nx = star_x + dx[d], ny = star_y + dy[d];
        if (is_star(player, nx, ny)) {
            nearby_stars++;
        }
    }
    value += nearby_stars * 80.0f;
    
    // 避免重复位置
    if (is_recent_position(star_x, star_y)) {
        value -= 150.0f;
    }
    
    return value;
}

// 寻找最佳星星 - 平衡版
int find_best_star_balanced(struct Player *player, int sx, int sy, int *best_x, int *best_y) {
    float best_value = -INF;
    int found = 0;
    
    // 限制搜索范围以控制时间
    int search_radius = 8;
    
    for (int i = max(0, sx - search_radius); i <= min(player->row_cnt - 1, sx + search_radius) && !is_timeout(); i++) {
        for (int j = max(0, sy - search_radius); j <= min(player->col_cnt - 1, sy + search_radius); j++) {
            if (is_star(player, i, j)) {
                float value = evaluate_star_balanced(player, sx, sy, i, j);
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

// 智能鬼魂追击 - 简化版
int balanced_ghost_hunt(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
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
    
    // 简化的追击判断
    if (player->your_status >= 3 || min_dist <= 8) {
        return smart_bfs(player, sx, sy, 
                        player->ghost_posx[target_ghost], 
                        player->ghost_posy[target_ghost], 
                        next_x, next_y);
    }
    
    return 0;
}

// 简化的阻挡策略
int balanced_block_opponent(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (opponent_x == -1 || game_phase == 0 || is_timeout()) return 0;
    
    // 只考虑非常近的超级星
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j) && player->mat[i][j] == 'O') {
                int opp_dist = abs(i - opponent_x) + abs(j - opponent_y);
                int my_dist = abs(i - sx) + abs(j - sy);
                
                // 只在非常有把握时才阻挡
                if (opp_dist <= 2 && my_dist <= 4 && my_dist < opp_dist) {
                    return smart_bfs(player, sx, sy, i, j, next_x, next_y);
                }
            }
        }
    }
    
    return 0;
}

// 安全移动 - 平衡版
int safe_move_balanced(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_dir = -1;
    float max_safety = -INF;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        float safety = distance_to_ghost(player, nx, ny);
        
        // 有星星的位置加分
        if (is_star(player, nx, ny)) {
            safety += (player->mat[nx][ny] == 'O') ? 10.0f : 5.0f;
        }
        
        // 避免重复位置
        if (is_recent_position(nx, ny)) {
            safety -= 3.0f;
        }
        
        // 避免死胡同
        int exits = 0;
        for (int dd = 0; dd < 4; dd++) {
            if (is_valid(player, nx + dx[dd], ny + dy[dd])) {
                exits++;
            }
        }
        safety += exits * 0.5f;
        
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
    
    if (best_dir != -1) {
        *next_x = sx + dx[best_dir];
        *next_y = sy + dy[best_dir];
        return 1;
    }
    
    return 0;
}

// 主决策函数 - 智能平衡版 (健壮版)
struct Point walk(struct Player *player) {
    start_time = clock();
    step_count++;
    
    // 安全检查
    if (!player) {
        struct Point ret = {0, 0};
        return ret;
    }
    
    int x = player->your_posx, y = player->your_posy;
    
    // 位置边界检查
    if (x < 0 || x >= player->row_cnt || y < 0 || y >= player->col_cnt) {
        struct Point ret = {x, y};
        return ret;
    }
    
    // 轻量级状态更新
    add_recent_position(x, y);
    update_opponent_position(player);
    update_game_phase(player);
    
    // 更新星星状态
    if (is_star(player, x, y)) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // 策略1: 强化状态追击鬼魂
    if (player->your_status > 0) {
        if (balanced_ghost_hunt(player, x, y, &next_x, &next_y)) {
            last_x = x;
            last_y = y;
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // 策略2: 适度的阻挡策略
    if (game_phase >= 1 && player->your_status <= 0 && !is_timeout()) {
        if (balanced_block_opponent(player, x, y, &next_x, &next_y)) {
            if (is_safe_move(player, next_x, next_y)) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // 策略3: 智能星星收集
    int star_target_x, star_target_y;
    if (find_best_star_balanced(player, x, y, &star_target_x, &star_target_y) && !is_timeout()) {
        if (smart_bfs(player, x, y, star_target_x, star_target_y, &next_x, &next_y)) {
            if (is_safe_move(player, next_x, next_y) || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // 策略4: 安全移动
    if (safe_move_balanced(player, x, y, &next_x, &next_y)) {
        last_x = x;
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略5: 紧急移动
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            int ghost_dist = distance_to_ghost(player, nx, ny);
            int required = (player->your_score < LOW_SCORE_THRESHOLD) ? 1 : 2;
            if (ghost_dist >= required || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {nx, ny};
                return ret;
            }
        }
    }
    
    // 最终保护
    last_x = x;
    last_y = y;
    struct Point ret = {x, y};
    return ret;
}
