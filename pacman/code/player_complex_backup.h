#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// 地图分类常量
#define SMALL_MAP_THRESHOLD 100    // 小地图大小阈值（10x10）
#define LARGE_MAP_THRESHOLD 900    // 大地图大小阈值（30x30）

// 强化状态时间常量
#define POWER_TIME_ABUNDANT 8      // 强化时间充足阈值
#define POWER_TIME_MODERATE 3      // 强化时间适中阈值

// 安全距离常量
#define DANGER_DISTANCE_IMMEDIATE 1  // 立即危险距离
#define DANGER_DISTANCE_CLOSE 2      // 近距离危险
#define SAFE_DISTANCE_PREFERRED 3    // 偏好安全距离

// 新增：更严格的安全距离常量
#define EMERGENCY_DISTANCE 1          // 紧急避险距离
#define CRITICAL_DISTANCE 2           // 临界危险距离
#define SAFE_DISTANCE_MIN 3           // 最小安全距离
#define SAFE_DISTANCE_PREFERRED_NEW 4 // 新的偏好安全距离

// 分数常量
#define GHOST_HUNT_BASE_SCORE 300.0f    // 鬼魂追击基础分数（提升）
#define NORMAL_STAR_VALUE 15            // 普通星星价值（提升）
#define POWER_STAR_VALUE 50             // 强化星星价值（大幅提升）

// 权重常量
#define DISTANCE_WEIGHT 50              // 距离权重
#define SAFETY_WEIGHT 100              // 安全权重
#define STAR_WEIGHT 200                // 星星权重
#define GHOST_AVOID_WEIGHT 150         // 鬼魂避免权重
#define OPPONENT_INTERFERENCE_WEIGHT 80 // 对手干扰权重

// 搜索性能常量
#define MAX_ASTAR_ITERATIONS_SMALL 300  // 小地图A*最大迭代次数
#define MAX_ASTAR_ITERATIONS_LARGE 100  // 大地图A*最大迭代次数
#define MAX_BFS_SEARCHES_SMALL 200      // 小地图BFS最大搜索数
#define MAX_BFS_SEARCHES_LARGE 1000     // 大地图BFS最大搜索数

// 目标稳定性常量
#define TARGET_STABILITY_MIN_COUNT 3    // 目标稳定性最小计数
#define TARGET_MAX_DISTANCE 8           // 目标最大合理距离

// 强制探索常量
#define FORCE_EXPLORATION_INTERVAL 50   // 强制探索间隔步数
#define FORCE_EXPLORATION_DISTANCE 10   // 强制探索最小距离

// 超级星诱导常量
#define GHOST_DISTANCE_FAR 20          // 鬼魂距离过远阈值
#define GHOST_DISTANCE_NEAR 5          // 鬼魂距离过近阈值
#define POWER_STAR_LURE_BONUS 80       // 超级星诱导奖励
#define NORMAL_STAR_LURE_BONUS 40      // 普通星诱导奖励
#define POWER_STAR_URGENT_BONUS 200    // 紧急情况超级星奖励

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;  // 记录上一步位置
static int step_count = 0;  // 步数计数器

// 时间控制变量
static clock_t start_time;  // 开始时间

// 【新增：路径历史记录，用于检测和避免震荡】
#define PATH_HISTORY_SIZE 5  // 记录最近5步位置
static int path_history_x[PATH_HISTORY_SIZE];
static int path_history_y[PATH_HISTORY_SIZE];
static int path_history_head = 0;  // 循环缓冲区头指针
static int path_history_count = 0;  // 当前记录的历史步数

// 破坏者出生地记录（用于强化状态下的智能追击）
static int ghost_spawn_x[2] = {-1, -1};  // 破坏者初始X坐标
static int ghost_spawn_y[2] = {-1, -1};  // 破坏者初始Y坐标
static int spawn_recorded = 0;           // 是否已记录出生地

// AI稳定性控制变量
static int current_target_x = -1, current_target_y = -1;  // 当前目标星星
static int target_stable_count = 0;  // 目标稳定计数器
static int last_score = 0;  // 上一回合分数
static int ghost_last_x[2] = {-1, -1}, ghost_last_y[2] = {-1, -1};  // 鬼魂上一步位置
static int danger_level = 0;  // 当前危险等级 0-安全 1-中等 2-危险
static int consecutive_safe_moves = 0;  // 连续安全移动次数
static int power_star_eaten_count = 0;  // 已吃超级星数量

// 星星缓存结构，避免重复遍历地图
struct StarInfo {
    int x, y;
    char type;  // 'o' 或 'O'
    int valid;  // 【新增：标记是否有效，用于增量更新】
};

static struct StarInfo star_cache[MAXN * MAXN];
static struct StarInfo power_star_cache[MAXN * MAXN];
static int star_cache_size = 0;
static int power_star_cache_size = 0;
static int cache_last_update_step = 0;  // 【新增：缓存最后更新步数】

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// A* 节点结构
struct AStarNode {
    int x, y;
    int g_cost;
    int h_cost;
    int f_cost;
    int parent_x, parent_y;
    int in_open;
    int in_closed;
};
static struct AStarNode nodes[MAXN][MAXN];
static int open_list[MAXN * MAXN * 2];
static int open_size;

//【优化：完善的初始化函数】
void init(struct Player *player) {
    // 初始化时间控制
    start_time = clock();
    
    memset(star_eaten, 0, sizeof(star_eaten));
    memset(ghost_last_x, -1, sizeof(ghost_last_x));
    memset(ghost_last_y, -1, sizeof(ghost_last_y));
    
    // 重置AI状态变量
    current_target_x = -1;
    current_target_y = -1;
    target_stable_count = 0;
    danger_level = 0;
    consecutive_safe_moves = 0;
    power_star_eaten_count = 0;
    last_score = 0;
    
    // 重置破坏者出生地记录
    ghost_spawn_x[0] = ghost_spawn_x[1] = -1;
    ghost_spawn_y[0] = ghost_spawn_y[1] = -1;
    spawn_recorded = 0;
    
    last_x = -1;
    last_y = -1;
    step_count = 0;
    
    // 【新增：初始化路径历史记录】
    memset(path_history_x, -1, sizeof(path_history_x));
    memset(path_history_y, -1, sizeof(path_history_y));
    path_history_head = 0;
    path_history_count = 0;
    
    // 【优化：完善缓存初始化】
    star_cache_size = 0;
    power_star_cache_size = 0;
    memset(power_star_cache, 0, sizeof(power_star_cache));
    memset(star_cache, 0, sizeof(star_cache));
    cache_last_update_step = 0;
    
    // 【新增：A*算法相关初始化】
    memset(nodes, 0, sizeof(nodes));
    memset(open_list, 0, sizeof(open_list));
    open_size = 0;
    
    // 【新增：游戏状态重置】
    // 确保所有静态变量都被正确重置
    if (player) {
        // 基于玩家信息进行特定初始化
        int map_size = player->row_cnt * player->col_cnt;
        
        // 根据地图大小调整某些参数
        if (map_size <= SMALL_MAP_THRESHOLD) {
            // 小地图特殊初始化
        } else if (map_size >= LARGE_MAP_THRESHOLD) {
            // 大地图特殊初始化
        }
    }
}

int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// 【新增：路径历史记录管理函数】
void add_to_path_history(int x, int y) {
    path_history_x[path_history_head] = x;
    path_history_y[path_history_head] = y;
    path_history_head = (path_history_head + 1) % PATH_HISTORY_SIZE;
    
    if (path_history_count < PATH_HISTORY_SIZE) {
        path_history_count++;
    }
}

// 计算到破坏者的距离
int distance_to_ghost(struct Player *player, int x, int y) {
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_dist) min_dist = dist;
    }
    return min_dist;
}

//【新增：增强的路径震荡检测函数】
int detect_path_oscillation(int current_x, int current_y) {
    if (path_history_count < 3) {
        return 0;  // 历史记录太少，无法检测震荡
    }
    
    // 检测2点循环（A→B→A→B）
    if (path_history_count >= 4) {
        int idx1 = (path_history_head - 2 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        int idx2 = (path_history_head - 4 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        
        if (path_history_x[idx1] == current_x && path_history_y[idx1] == current_y &&
            path_history_x[idx2] == current_x && path_history_y[idx2] == current_y) {
            return 2;  // 检测到2点循环
        }
    }
    
    // 检测3点循环（A→B→C→A→B→C）
    if (path_history_count >= 6) {
        int idx1 = (path_history_head - 3 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        
        if (path_history_x[idx1] == current_x && path_history_y[idx1] == current_y) {
            return 3;  // 检测到3点循环
        }
    }
    
    // 【新增：检测4点循环（A→B→C→D→A→B→C→D）】
    if (path_history_count >= 8) {
        int idx1 = (path_history_head - 4 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        int idx2 = (path_history_head - 8 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        
        if (path_history_x[idx1] == current_x && path_history_y[idx1] == current_y &&
            path_history_x[idx2] == current_x && path_history_y[idx2] == current_y) {
            return 4;  // 检测到4点循环
        }
    }
    
    // 【新增：检测来回震荡模式（A→B→A→B→A）】
    if (path_history_count >= 5) {
        int back_forth_count = 0;
        for (int i = 1; i < path_history_count - 1; i++) {
            int idx_prev = (path_history_head - i - 1 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
            int idx_curr = (path_history_head - i + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
            int idx_next = (path_history_head - i + 1 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
            
            // 检查是否来回移动
            if (path_history_x[idx_prev] == path_history_x[idx_next] &&
                path_history_y[idx_prev] == path_history_y[idx_next] &&
                !(path_history_x[idx_curr] == path_history_x[idx_prev] &&
                  path_history_y[idx_curr] == path_history_y[idx_prev])) {
                back_forth_count++;
            }
        }
        
        if (back_forth_count >= 2) {
            return 5;  // 检测到来回震荡模式
        }
    }
    
    // 【新增：检测区域震荡（在小区域内反复移动）】
    if (path_history_count >= PATH_HISTORY_SIZE) {
        // 计算历史位置的边界框
        int min_x = current_x, max_x = current_x;
        int min_y = current_y, max_y = current_y;
        
        for (int i = 0; i < path_history_count; i++) {
            if (path_history_x[i] < min_x) min_x = path_history_x[i];
            if (path_history_x[i] > max_x) max_x = path_history_x[i];
            if (path_history_y[i] < min_y) min_y = path_history_y[i];
            if (path_history_y[i] > max_y) max_y = path_history_y[i];
        }
        
        // 如果在一个小区域内反复移动
        int area_width = max_x - min_x + 1;
        int area_height = max_y - min_y + 1;
        
        if (area_width <= 3 && area_height <= 3) {
            return 6;  // 检测到区域震荡
        }
    }
    
    // 检测短距离重复访问
    int visit_count = 0;
    for (int i = 0; i < path_history_count; i++) {
        if (path_history_x[i] == current_x && path_history_y[i] == current_y) {
            visit_count++;
        }
    }
    
    if (visit_count >= 2) {
        return 1;  // 检测到重复访问
    }
    
    return 0;  // 未检测到震荡
}

//【新增：增强的打破震荡模式函数】
int break_oscillation_pattern(struct Player *player, int current_x, int current_y, int *next_x, int *next_y) {
    // 策略1：优先选择垂直方向移动（如果当前主要是水平移动）
    int horizontal_moves = 0, vertical_moves = 0;
    
    // 分析最近的移动趋势
    for (int i = 1; i < path_history_count; i++) {
        int idx_curr = (path_history_head - i + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        int idx_prev = (path_history_head - i - 1 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        
        if (path_history_x[idx_curr] != path_history_x[idx_prev]) {
            horizontal_moves++;
        }
        if (path_history_y[idx_curr] != path_history_y[idx_prev]) {
            vertical_moves++;
        }
    }
    
    // 确定优先方向：如果水平移动多，优先垂直；反之亦然
    int priority_dirs[4];
    if (horizontal_moves > vertical_moves) {
        // 优先垂直移动
        priority_dirs[0] = 0;  // 上
        priority_dirs[1] = 2;  // 下
        priority_dirs[2] = 1;  // 右
        priority_dirs[3] = 3;  // 左
    } else {
        // 优先水平移动
        priority_dirs[0] = 1;  // 右
        priority_dirs[1] = 3;  // 左
        priority_dirs[2] = 0;  // 上
        priority_dirs[3] = 2;  // 下
    }
    
    // 【新增：策略2A：智能方向选择，考虑安全性和目标导向】
    typedef struct {
        int dir;
        int x, y;
        float score;
    } CandidateMove;
    
    CandidateMove candidates[4];
    int candidate_count = 0;
    
    for (int i = 0; i < 4; i++) {
        int d = priority_dirs[i];
        int nx = current_x + dx[d];
        int ny = current_y + dy[d];
        
        if (is_valid(player, nx, ny)) {
            float score = 100.0f;
            
            // 检查这个位置是否在历史记录中
            int in_history = 0;
            int history_recency = 0;
            
            for (int j = 0; j < path_history_count; j++) {
                if (path_history_x[j] == nx && path_history_y[j] == ny) {
                    in_history = 1;
                    history_recency = path_history_count - j;  // 越近的历史记录recency越大
                    break;
                }
            }
            
            // 如果不在历史记录中，大幅加分
            if (!in_history) {
                score += 500.0f;
            } else {
                // 在历史记录中，根据时间距离给分
                score += history_recency * 10.0f;
            }
            
            // 【新增：安全性评估】
            int ghost_dist = distance_to_ghost(player, nx, ny);
            score += ghost_dist * 15.0f;  // 距离鬼魂越远越好
            
            // 【新增：目标导向性评估】
            // 寻找最近的星星，朝向星星的方向给予奖励
            int min_star_dist = INF;
            for (int row = 0; row < player->row_cnt; row++) {
                for (int col = 0; col < player->col_cnt; col++) {
                    if (is_star(player, row, col)) {
                        int star_dist = abs(nx - row) + abs(ny - col);
                        if (star_dist < min_star_dist) {
                            min_star_dist = star_dist;
                        }
                    }
                }
            }
            
            if (min_star_dist != INF) {
                score += 100.0f / (min_star_dist + 1);  // 距离星星越近越好
            }
            
            // 【新增：避免死胡同】
            int exit_count = 0;
            for (int dd = 0; dd < 4; dd++) {
                int check_x = nx + dx[dd];
                int check_y = ny + dy[dd];
                if (is_valid(player, check_x, check_y)) {
                    exit_count++;
                }
            }
            score += exit_count * 20.0f;  // 出路越多越好
            
            // 【新增：边界奖励】
            if (nx == 0 || nx == player->row_cnt - 1 || ny == 0 || ny == player->col_cnt - 1) {
                score += 50.0f;  // 边界位置有助于打破震荡
            }
            
            candidates[candidate_count].dir = d;
            candidates[candidate_count].x = nx;
            candidates[candidate_count].y = ny;
            candidates[candidate_count].score = score;
            candidate_count++;
        }
    }
    
    // 排序候选位置
    for (int i = 0; i < candidate_count; i++) {
        for (int j = i + 1; j < candidate_count; j++) {
            if (candidates[j].score > candidates[i].score) {
                CandidateMove temp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = temp;
            }
        }
    }
    
    // 选择最佳候选
    if (candidate_count > 0) {
        *next_x = candidates[0].x;
        *next_y = candidates[0].y;
        return 1;
    }
    
    // 策略3：如果所有位置都在历史中，选择最远的历史位置
    int best_dir = -1;
    int max_history_distance = -1;
    
    for (int i = 0; i < 4; i++) {
        int d = priority_dirs[i];
        int nx = current_x + dx[d];
        int ny = current_y + dy[d];
        
        if (is_valid(player, nx, ny)) {
            // 找这个位置在历史中的最远距离
            int history_distance = path_history_count;
            for (int j = 0; j < path_history_count; j++) {
                if (path_history_x[j] == nx && path_history_y[j] == ny) {
                    history_distance = j;
                    break;
                }
            }
            
            if (history_distance > max_history_distance) {
                max_history_distance = history_distance;
                best_dir = d;
            }
        }
    }
    
    if (best_dir != -1) {
        *next_x = current_x + dx[best_dir];
        *next_y = current_y + dy[best_dir];
        return 1;
    }
    
    return 0;  // 无法打破震荡
}

// 【新增：增强的安全移动选择，整合震荡检测】
int choose_safe_direction_with_oscillation_check(struct Player *player, int x, int y, int *next_x, int *next_y) {
    // 首先检查是否有震荡
    int oscillation_type = detect_path_oscillation(x, y);
    
    if (oscillation_type > 0) {
        // 检测到震荡，尝试打破
        if (break_oscillation_pattern(player, x, y, next_x, next_y)) {
            return 1;
        }
    }
    
    // 如果没有震荡或无法打破，使用原有的安全移动选择
    return choose_safe_direction(player, x, y, next_x, next_y);
}

// 获取对手位置和分数（确保对手分数为正数）
int get_opponent_info(struct Player *player, int *opponent_x, int *opponent_y, int *opponent_score) {
    // 验证输入参数
    if (!player || !opponent_x || !opponent_y || !opponent_score) {
        return 0;  // 参数无效
    }
    
    // 检查对手是否存在且存活
    if (player->opponent_status == -1) {
        return 0;  // 对手已死亡
    }
    
    // 验证对手位置是否有效
    if (player->opponent_posx < 0 || player->opponent_posx >= player->row_cnt ||
        player->opponent_posy < 0 || player->opponent_posy >= player->col_cnt) {
        return 0;  // 对手位置无效
    }
    
    // 获取对手基本信息
    *opponent_x = player->opponent_posx;
    *opponent_y = player->opponent_posy;
    *opponent_score = player->opponent_score;
    
    // 验证对手分数是否为正数（只有正分的对手才值得追击）
    if (*opponent_score <= 0) {
        return 0;  // 对手分数非正数，不值得追击
    }
    return 1;  // 成功获取对手信息
}

// 评估超级星的反杀价值（考虑获得强化后的收益）
float evaluate_power_star_counter_attack_value(struct Player *player, int sx, int sy, int power_x, int power_y) {
    if (player->your_status > 0) return 0.0;  // 已经强化，无需额外价值
    
    float total_value = 0.0;
    
    // 计算到超级星的距离
    int dist_to_power = abs(sx - power_x) + abs(sy - power_y);
    
    // 评估每个鬼魂的追击价值
    for (int i = 0; i < 2; i++) {
        int ghost_x = player->ghost_posx[i];
        int ghost_y = player->ghost_posy[i];
        
        // 鬼魂到超级星的距离
        int ghost_to_power_dist = abs(ghost_x - power_x) + abs(ghost_y - power_y);
        
        // 我们到鬼魂的距离（获得强化后）
        int power_to_ghost_dist = abs(power_x - ghost_x) + abs(power_y - ghost_y);
        
        // 关键判断：如果我们能在鬼魂到达前获得超级星，并且强化时间足够追击
        int time_advantage = ghost_to_power_dist - dist_to_power;  // 我们的时间优势
        
        if (time_advantage >= 1) {  // 至少有1回合的时间优势
            // 预估强化时间（通常超级星给10回合强化）
            int estimated_power_time = 10;
            
            // 如果强化时间足够追到鬼魂
            if (estimated_power_time > power_to_ghost_dist + 2) {
                float ghost_value = GHOST_HUNT_BASE_SCORE;
                
                // 距离越近，价值越高
                float distance_factor = 1.0 / (power_to_ghost_dist + 1);
                ghost_value *= distance_factor;
                
                // 时间优势越大，价值越高
                if (time_advantage >= 3) {
                    ghost_value *= 1.5;  // 大幅时间优势
                } else if (time_advantage >= 2) {
                    ghost_value *= 1.2;  // 适度时间优势
                }
                
                // 鬼魂在死胡同中价值更高（更容易追到）
                int ghost_escape_routes = 0;
                for (int d = 0; d < 4; d++) {
                    int check_x = ghost_x + dx[d];
                    int check_y = ghost_y + dy[d];
                    if (is_valid(player, check_x, check_y)) {
                        ghost_escape_routes++;
                    }
                }
                
                if (ghost_escape_routes <= 2) {
                    ghost_value *= 1.3;  // 鬼魂在死胡同，更容易追到
                }
                
                total_value += ghost_value;
            }
        }
    }
    
    // 双鬼追击奖励：如果能同时追击两个鬼魂
    if (total_value > GHOST_HUNT_BASE_SCORE * 1.5) {
        total_value += 100.0;  // 双杀奖励
    }
    
    return total_value;
}

// 【新增：资源封锁策略 - 抢夺对手目标的超级星】
float evaluate_resource_blocking(struct Player *player, int our_x, int our_y, int power_star_x, int power_star_y) {
    int opponent_x, opponent_y, opponent_score;
    
    // 获取对手信息
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return 0.0;  // 无对手信息，无法执行封锁
    }
    
    int our_dist = abs(our_x - power_star_x) + abs(our_y - power_star_y);
    int opponent_dist = abs(opponent_x - power_star_x) + abs(opponent_y - power_star_y);
    
    // 基础封锁价值
    float blocking_value = 0.0;
    
    // 如果对手比我们更接近超级星，考虑封锁
    if (opponent_dist < our_dist) {
        int distance_disadvantage = our_dist - opponent_dist;
        
        // 封锁价值与距离劣势成反比
        if (distance_disadvantage <= 3) {
            blocking_value = 200.0 - distance_disadvantage * 50.0;  // 距离劣势越小，封锁价值越高
            
            // 对手分数越高，封锁价值越大
            if (opponent_score > player->your_score) {
                blocking_value += 100.0;  // 对手领先时，封锁更重要
            } else if (opponent_score > player->your_score * 0.8) {
                blocking_value += 50.0;   // 对手接近时，适度封锁
            }
            
            // 超级星位置的重要性
            if (power_star_x <= 2 || power_star_x >= player->row_cnt - 3 || 
                power_star_y <= 2 || power_star_y >= player->col_cnt - 3) {
                blocking_value += 30.0;  // 边缘超级星更重要
            }
            
            // 游戏后期封锁更重要
            if (step_count > 100) {
                blocking_value += 40.0;  // 后期资源稀缺，封锁更重要
            }
        }
    }
    
    return blocking_value;
}

// 【新增：路径阻挡策略 - 在狭窄通道阻挡对手】
float evaluate_path_blocking(struct Player *player, int our_x, int our_y, int move_x, int move_y) {
    int opponent_x, opponent_y, opponent_score;
    
    // 获取对手信息
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return 0.0;  // 无对手信息，无法执行阻挡
    }
    
    // 预测对手的移动方向
    int pred_opponent_x, pred_opponent_y;
    predict_opponent_movement(player, opponent_x, opponent_y, &pred_opponent_x, &pred_opponent_y);
    
    // 检查移动位置是否在对手可能的路径上
    float blocking_value = 0.0;
    
    // 计算对手到我们移动位置的距离
    int opponent_to_move_dist = abs(opponent_x - move_x) + abs(move_y - opponent_y);
    
    // 只有在对手相对较近时才考虑阻挡
    if (opponent_to_move_dist <= 5) {
        // 检查这个位置是否是狭窄通道
        int narrow_passage_score = 0;
        int adjacent_walls = 0;
        
        for (int d = 0; d < 4; d++) {
            int check_x = move_x + dx[d];
            int check_y = move_y + dy[d];
            
            if (check_x < 0 || check_x >= player->row_cnt || 
                check_y < 0 || check_y >= player->col_cnt ||
                player->mat[check_x][check_y] == '#') {
                adjacent_walls++;
            }
        }
        
        // 狭窄通道（3面墙 或 2面墙但形成走廊）
        if (adjacent_walls >= 2) {
            narrow_passage_score = (adjacent_walls - 1) * 30;  // 墙越多，通道越窄
        }
        
        // 检查是否在对手的移动路径上
        int on_opponent_path = 0;
        
        // 简单检查：如果我们的移动位置在对手当前位置和预测位置之间
        if ((move_x == opponent_x && abs(move_y - opponent_y) <= 2) ||
            (move_y == opponent_y && abs(move_x - opponent_x) <= 2)) {
            on_opponent_path = 1;
        }
        
        // 或者在对手前往最近星星的路径上
        build_star_cache(player);
        int nearest_star_x = -1, nearest_star_y = -1;
        int min_star_dist = INF;
        
        for (int k = 0; k < star_cache_size; k++) {
            int dist = abs(opponent_x - star_cache[k].x) + abs(opponent_y - star_cache[k].y);
            if (dist < min_star_dist) {
                min_star_dist = dist;
                nearest_star_x = star_cache[k].x;
                nearest_star_y = star_cache[k].y;
            }
        }
        
        // 检查超级星
        for (int k = 0; k < power_star_cache_size; k++) {
            int dist = abs(opponent_x - power_star_cache[k].x) + abs(opponent_y - power_star_cache[k].y);
            if (dist < min_star_dist) {
                min_star_dist = dist;
                nearest_star_x = power_star_cache[k].x;
                nearest_star_y = power_star_cache[k].y;
            }
        }
        
        // 如果找到了对手的目标星星，检查是否在路径上
        if (nearest_star_x != -1) {
            // 简化的路径检查：我们是否在对手到星星的大致路径上
            int opponent_to_star_x = nearest_star_x - opponent_x;
            int opponent_to_star_y = nearest_star_y - opponent_y;
            int opponent_to_move_x = move_x - opponent_x;
            int opponent_to_move_y = move_y - opponent_y;
            
            // 如果移动方向与对手到星星的方向相似
            if ((opponent_to_star_x * opponent_to_move_x >= 0 && opponent_to_star_y * opponent_to_move_y >= 0) &&
                (abs(opponent_to_move_x) + abs(opponent_to_move_y) <= abs(opponent_to_star_x) + abs(opponent_to_star_y))) {
                on_opponent_path = 1;
            }
        }
        
        if (on_opponent_path && narrow_passage_score > 0) {
            blocking_value = narrow_passage_score + 50.0;  // 基础阻挡价值
            
            // 对手分数越高，阻挡价值越大
            if (opponent_score > player->your_score) {
                blocking_value += 60.0;  // 对手领先时，阻挡更重要
            }
            
            // 如果我们处于安全状态，阻挡价值更高
            int our_safety = assess_danger_level(player, move_x, move_y);
            if (our_safety == 0) {
                blocking_value += 40.0;  // 我们安全时，可以更积极阻挡
            }
        }
    }
    
    return blocking_value;
}

// 【新增：鬼魂引导策略 - 将鬼魂引向对手】
float evaluate_ghost_guidance(struct Player *player, int our_x, int our_y, int move_x, int move_y) {
    int opponent_x, opponent_y, opponent_score;
    
    // 获取对手信息
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return 0.0;  // 无对手信息，无法执行引导
    }
    
    // 只有在我们相对安全时才考虑引导鬼魂
    int our_safety = assess_danger_level(player, move_x, move_y);
    if (our_safety >= 2) {
        return 0.0;  // 我们不安全，不执行引导
    }
    
    float guidance_value = 0.0;
    
    // 检查移动是否会让我们更接近对手（从而引导鬼魂）
    int current_dist_to_opponent = abs(our_x - opponent_x) + abs(our_y - opponent_y);
    int new_dist_to_opponent = abs(move_x - opponent_x) + abs(move_y - opponent_y);
    
    // 只有当对手不太远时才考虑引导
    if (current_dist_to_opponent <= 8 && new_dist_to_opponent < current_dist_to_opponent) {
        // 检查鬼魂位置和我们的相对位置
        for (int i = 0; i < 2; i++) {
            int ghost_x = player->ghost_posx[i];
            int ghost_y = player->ghost_posy[i];
            
            int ghost_to_us_dist = abs(ghost_x - move_x) + abs(ghost_y - move_y);
            int ghost_to_opponent_dist = abs(ghost_x - opponent_x) + abs(ghost_y - opponent_y);
            
            // 如果鬼魂离我们比离对手更近，但差距不大，可以引导
            if (ghost_to_us_dist <= ghost_to_opponent_dist + 2 && ghost_to_us_dist <= 5) {
                // 检查我们移动后是否会让鬼魂更接近对手
                int new_ghost_to_us = abs(ghost_x - move_x) + abs(ghost_y - move_y);
                int original_ghost_to_us = abs(ghost_x - our_x) + abs(ghost_y - our_y);
                
                // 如果我们移动向对手方向，鬼魂追击我们时会更接近对手
                if (new_dist_to_opponent < current_dist_to_opponent) {
                    guidance_value += 40.0;  // 基础引导价值
                    
                    // 对手分数越高，引导价值越大
                    if (opponent_score > player->your_score) {
                        guidance_value += 50.0;  // 对手领先时，引导更重要
                    }
                    
                    // 如果对手处于相对危险的位置，引导价值更高
                    int opponent_danger = assess_danger_level(player, opponent_x, opponent_y);
                    if (opponent_danger >= 1) {
                        guidance_value += 30.0;  // 对手已经有危险，引导更有效
                    }
                    
                    // 鬼魂越接近，引导效果越好
                    if (ghost_to_us_dist <= 3) {
                        guidance_value += 20.0;  // 鬼魂很近，引导更有效
                    }
                }
            }
        }
    }
    
    return guidance_value;
}

// 【新增：综合对手干扰策略评估】
float evaluate_opponent_interference(struct Player *player, int our_x, int our_y, int move_x, int move_y) {
    float total_interference_value = 0.0;
    
    // 1. 资源封锁评估
    build_star_cache(player);
    for (int k = 0; k < power_star_cache_size; k++) {
        float blocking_value = evaluate_resource_blocking(player, our_x, our_y, 
                                                         power_star_cache[k].x, power_star_cache[k].y);
        
        // 如果这个移动让我们更接近需要封锁的超级星
        int current_dist = abs(our_x - power_star_cache[k].x) + abs(our_y - power_star_cache[k].y);
        int new_dist = abs(move_x - power_star_cache[k].x) + abs(move_y - power_star_cache[k].y);
        
        if (new_dist < current_dist && blocking_value > 0) {
            total_interference_value += blocking_value * OPPONENT_INTERFERENCE_WEIGHT / 100.0;  // 使用统一权重常量
        }
    }
    
    // 2. 路径阻挡评估
    float path_blocking_value = evaluate_path_blocking(player, our_x, our_y, move_x, move_y);
    total_interference_value += path_blocking_value * OPPONENT_INTERFERENCE_WEIGHT / 150.0;  // 使用统一权重常量
    
    // 3. 鬼魂引导评估
    float ghost_guidance_value = evaluate_ghost_guidance(player, our_x, our_y, move_x, move_y);
    total_interference_value += ghost_guidance_value * OPPONENT_INTERFERENCE_WEIGHT / 120.0;  // 使用统一权重常量
    
    return total_interference_value;
}

// 优化的破坏者移动预测函数【增强：智能路径预测、墙壁检测、多步预测】
void predict_ghost_movement(struct Player *player, int ghost_idx, int *pred_x, int *pred_y) {
    // 边界检查
    if (ghost_idx < 0 || ghost_idx >= 2) {
        *pred_x = player->your_posx;
        *pred_y = player->your_posy;
        return;
    }
    
    int gx = player->ghost_posx[ghost_idx];
    int gy = player->ghost_posy[ghost_idx];
    int px = player->your_posx;
    int py = player->your_posy;
    
    // 简化：基于强化状态的基本预测
    int best_dir = -1;
    int best_score = -1000;
    
            for (int d = 0; d < 4; d++) {
                int next_x = gx + dx[d];
                int next_y = gy + dy[d];
                
        if (!is_valid(player, next_x, next_y)) continue;
        
        int score = 0;
        
        // 基于强化状态的方向偏好
        if (player->your_status == 0) {
            // 非强化状态：鬼魂追击玩家
            int dist_to_player = abs(next_x - px) + abs(next_y - py);
            score += 1000 / (dist_to_player + 1);
        } else {
            // 强化状态：鬼魂远离玩家
            int dist_to_player = abs(next_x - px) + abs(next_y - py);
            score += dist_to_player * 20;
        }
        
        // 历史方向一致性
                    if (ghost_last_x[ghost_idx] != -1 && ghost_last_y[ghost_idx] != -1) {
                        int hist_dx = gx - ghost_last_x[ghost_idx];
                        int hist_dy = gy - ghost_last_y[ghost_idx];
                        
                        if (dx[d] == hist_dx && dy[d] == hist_dy) {
                score += 50;  // 方向一致性奖励
            } else if (dx[d] == -hist_dx && dy[d] == -hist_dy) {
                score -= 30;  // 避免掉头
            }
        }
        
        // 出路数量评估
            int exit_routes = 0;
            for (int dd = 0; dd < 4; dd++) {
                int check_x = next_x + dx[dd];
                int check_y = next_y + dy[dd];
                if (is_valid(player, check_x, check_y)) {
                    exit_routes++;
                }
            }
        score += exit_routes * 10;
        
        if (score > best_score) {
            best_score = score;
                best_dir = d;
            }
        }
        
        if (best_dir != -1) {
            *pred_x = gx + dx[best_dir];
            *pred_y = gy + dy[best_dir];
    } else {
    *pred_x = gx;
    *pred_y = gy;
    }
}

// 改进的安全移动方向选择（适用于所有地图大小）
int choose_safe_direction(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_dir = -1;
    float max_safety = -1000.0f;
    
    // 计算当前危险等级
    int current_danger = assess_danger_level(player, x, y);
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    // 紧急避险模式：如果鬼魂距离≤1，强制选择最安全的方向
    if (min_ghost_dist <= EMERGENCY_DISTANCE) {
        float emergency_safety = -1000.0f;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (!is_valid(player, nx, ny)) continue;
            
            int new_ghost_dist = distance_to_ghost(player, nx, ny);
            float safety = new_ghost_dist * 10.0f; // 紧急模式下距离权重极高
            
            // 检查这个方向是否远离所有鬼魂
            int all_ghosts_safe = 1;
            for (int i = 0; i < 2; i++) {
                int pred_x, pred_y;
                predict_ghost_movement(player, i, &pred_x, &pred_y);
                int pred_dist = abs(nx - pred_x) + abs(ny - pred_y);
                if (pred_dist <= 1) {
                    all_ghosts_safe = 0;
                    safety -= 50.0f;
                }
            }
            
            // 死胡同惩罚
            int exit_routes = 0;
            for (int dd = 0; dd < 4; dd++) {
                int check_x = nx + dx[dd], check_y = ny + dy[dd];
                if (is_valid(player, check_x, check_y)) exit_routes++;
            }
            if (exit_routes <= 1) safety -= 30.0f;
            
            if (safety > emergency_safety) {
                emergency_safety = safety;
                best_dir = d;
            }
        }
        
        if (best_dir != -1) {
            *next_x = x + dx[best_dir];
            *next_y = y + dy[best_dir];
            return 1;
        }
    }
    
    // 正常安全移动模式
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        float safety_score = 0.0f;
        
        // 1. 基础距离安全价值（权重：50% - 提升权重）
        int new_ghost_dist = distance_to_ghost(player, nx, ny);
        safety_score += new_ghost_dist * 0.5f;
        
        // 2. 鬼魂移动预测（权重：40% - 大幅提升权重）
        for (int i = 0; i < 2; i++) {
            int pred_x, pred_y;
            predict_ghost_movement(player, i, &pred_x, &pred_y);
            int pred_dist = abs(nx - pred_x) + abs(ny - pred_y);
            safety_score += pred_dist * 0.2f; // 每个鬼魂20%权重
            
            // 如果预测位置非常近，大幅惩罚
            if (pred_dist <= 1) {
                safety_score -= 20.0f;
            } else if (pred_dist <= 2) {
                safety_score -= 10.0f;
            }
        }
        
        // 3. 避免回头（权重：5%）
        if (nx == last_x && ny == last_y) {
            safety_score -= 0.05f;
        }
        
        // 4. 死胡同评估（权重：15%）
        int exit_routes = 0;
        for (int dd = 0; dd < 4; dd++) {
            int check_x = nx + dx[dd], check_y = ny + dy[dd];
            if (is_valid(player, check_x, check_y)) exit_routes++;
        }
        
        if (exit_routes <= 1) {
            safety_score -= 0.15f;
            
            // 死胡同中的超级星反杀评估（仅在强化状态）
            if (player->your_status > 0) {
                for (int depth = 1; depth < 4; depth++) {
                    int search_x = nx + dx[d] * (depth - 1);
                    int search_y = ny + dy[d] * (depth - 1);
                    if (!is_valid(player, search_x, search_y)) break;
                    
                    if (player->mat[search_x][search_y] == 'O') {
                        float counter_value = evaluate_power_star_counter_attack_value(player, x, y, search_x, search_y);
                        if (counter_value >= 300.0f) {
                            safety_score += 0.2f;
                        } else if (counter_value >= 200.0f) {
                            safety_score += 0.1f;
                        }
                        break;
                    }
                }
            }
        }
        
        // 5. 星星收集安全性（权重：5%）
        char cell = player->mat[nx][ny];
        if (cell == 'o' || cell == 'O') {
            int star_danger = assess_danger_level(player, nx, ny);
            if (star_danger < 2) {
                safety_score += 0.05f;
            } else {
                safety_score -= 0.1f; // 危险星星惩罚
            }
        }
        
        // 6. 外圈优先策略（权重：10%）
        int center_x = player->row_cnt / 2;
        int center_y = player->col_cnt / 2;
        int dist_from_center = abs(nx - center_x) + abs(ny - center_y);
        int max_dist_from_center = (player->row_cnt + player->col_cnt) / 2;
        
        float outer_ring_bonus = (float)dist_from_center / max_dist_from_center;
        safety_score += outer_ring_bonus * 0.1f;
        
        // 7. 新增：当前危险等级影响
        if (current_danger >= 2) {
            // 高危险时，只有非常安全的方向才考虑
            if (new_ghost_dist < SAFE_DISTANCE_MIN) {
                safety_score -= 50.0f;
            }
        }
        
        if (safety_score > max_safety) {
            max_safety = safety_score;
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

// 评估追击特定鬼魂的价值【优化：路径收益、时间管理、出生地策略】
float evaluate_ghost_hunt(struct Player *player, int sx, int sy, int ghost_idx) {
    if (player->your_status <= 0) return -1000.0f;
    
    int ghost_x = player->ghost_posx[ghost_idx];
    int ghost_y = player->ghost_posy[ghost_idx];
    int distance = abs(sx - ghost_x) + abs(sy - ghost_y);
    
    float score = 1.0f;  // 基础价值1.0
    
    // 1. 距离因子（权重：40%）
    score += 1.0f / (distance + 1) * 0.4f;
    
    // 2. 强化时间管理（权重：30%）
    if (player->your_status > 8) {
        score += 0.3f;  // 时间充足
    } else if (player->your_status > 3) {
        score += 0.2f;  // 时间适中
    } else {
        score += 0.1f;  // 时间紧张
    }
    
    // 3. 地形优势（权重：20%）
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int check_x = ghost_x + dx[d], check_y = ghost_y + dy[d];
        if (is_valid(player, check_x, check_y)) escape_routes++;
    }
    
    if (escape_routes <= 2) {
        score += 0.2f;  // 鬼魂在死胡同
    } else if (escape_routes == 3) {
        score += 0.1f;  // 鬼魂选择有限
    }
    
    // 4. 预测移动（权重：10%）
    int pred_x, pred_y;
    predict_ghost_movement(player, ghost_idx, &pred_x, &pred_y);
    int pred_distance = abs(sx - pred_x) + abs(sy - pred_y);
    
    if (pred_distance < distance) {
        score += 0.1f;  // 鬼魂在靠近
    } else if (pred_distance > distance) {
        score -= 0.05f;  // 鬼魂在远离
    }
    
    return score;
}
}

// 评估追击对手的价值
float evaluate_opponent_hunt(struct Player *player, int sx, int sy, int opponent_x, int opponent_y, int opponent_score) {
    if (player->your_status <= 0) return -1000.0;  // 没有强化状态不能追击
    
    // 关键验证：确保对手分数为正数
    if (opponent_score <= 0) {
        return -1000.0;  // 对手分数为0或负数，不值得追击
    }
    
    int distance = abs(sx - opponent_x) + abs(sy - opponent_y);
    
    // 基础价值：对手分数的一半，但有最低保障
    float score = (float)opponent_score * 0.5;
    
    // 确保最低追击价值（正分对手总是有一定价值）
    if (score < 50.0) {
        score = 50.0;  // 最低50分价值
    }
    
    // 距离因子：距离越近价值越高
    float distance_factor = 1.0 / (distance + 1);
    score *= distance_factor * 2.0;
    
    // 强化时间管理：确保有足够时间追到对手
    if (player->your_status <= distance) {
        score -= score * 0.8;  // 时间不够，大幅降分
    } else if (player->your_status <= distance + 2) {
        score -= score * 0.4;  // 时间紧张，适度降分
    }
    
    // 对手可能也在移动，增加追击难度
    if (distance > 3) {
        score -= 50.0;  // 距离太远，对手容易逃脱
    }
    
    // 地形优势：角落和死胡同中的对手更容易追到
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int check_x = opponent_x + dx[d];
        int check_y = opponent_y + dy[d];
        if (is_valid(player, check_x, check_y)) {
            escape_routes++;
        }
    }
    
    if (escape_routes <= 2) {
        score += 60.0;  // 对手在死胡同或角落
    } else if (escape_routes == 3) {
        score += 30.0;  // 对手选择有限
    }
    
    // 风险评估：如果附近有鬼魂，追击对手可能有风险
    for (int i = 0; i < 2; i++) {
        int ghost_to_opponent = abs(opponent_x - player->ghost_posx[i]) + abs(opponent_y - player->ghost_posy[i]);
        if (ghost_to_opponent <= 2) {
            score -= 40.0;  // 对手附近有鬼魂，追击风险大
        }
    }
    
    // 【优化：结合对手信息的动态追击价值判断】
    // 根据对手分数与我方分数的相对关系动态调整
    int score_gap = opponent_score - player->your_score;
    
1    // 基础追击价值：对手分数越高，追击价值越大
    if (opponent_score >= 5000) {
        score += 150.0;  // 高分对手，追击价值极高
        if (score_gap > 500) {
            score += 100.0;  // 对手领先很多时，追击更重要
        }
    } else if (opponent_score >= 2500) {
        score += 100.0;  // 中高分对手
        if (score_gap > 300) {
            score += 80.0;   // 对手领先时，追击重要
        }
    } else if (opponent_score >= 1000) {
        score += 60.0;   // 中等分对手
        if (score_gap > 200) {
            score += 50.0;   // 对手领先时，适度提升
        }
    } else if (opponent_score >= 500) {
        score += 30.0;   // 低分对手，但仍有价值
    }
    
    // 【新增：对手强化状态考虑】
    // 如果对手处于强化状态，追击价值降低（因为对手可能反杀）
    if (player->opponent_status > 0) {
        score -= 50.0;  // 对手强化状态，追击风险增加
        if (player->opponent_status > 5) {
            score -= 50.0;  // 对手强化时间长，风险更大
        }
    }
    
    return score;
}

// 新增：预测对手下一步行为（基于策略文档的对手预测模块）
void predict_opponent_movement(struct Player *player, int opponent_x, int opponent_y, int *pred_x, int *pred_y) {
    // 默认返回当前位置
    *pred_x = opponent_x;
    *pred_y = opponent_y;
    
    // 边界检查
    if (opponent_x < 0 || opponent_x >= player->row_cnt || 
        opponent_y < 0 || opponent_y >= player->col_cnt) {
        return;
    }
    
    // 简单的对手行为预测：假设对手会向最近的星星移动
    build_star_cache(player);
    
    int best_dir = -1;
    float best_value = -1000.0f;
    
    for (int d = 0; d < 4; d++) {
        int next_x = opponent_x + dx[d];
        int next_y = opponent_y + dy[d];
        
        if (is_valid(player, next_x, next_y)) {
            float move_value = 0;
            
            // 评估移动到星星的价值
            char cell = player->mat[next_x][next_y];
            if (cell == 'o') {
                move_value += 100.0f;  // 普通星星
            } else if (cell == 'O') {
                move_value += 200.0f;  // 超级星星
            }
            
            // 寻找最近星星的方向奖励
            int min_star_dist = INF;
            for (int k = 0; k < star_cache_size; k++) {
                int dist = abs(next_x - star_cache[k].x) + abs(next_y - star_cache[k].y);
                if (dist < min_star_dist) {
                    min_star_dist = dist;
                }
            }
            for (int k = 0; k < power_star_cache_size; k++) {
                int dist = abs(next_x - power_star_cache[k].x) + abs(next_y - power_star_cache[k].y);
                if (dist < min_star_dist) {
                    min_star_dist = dist;
                }
            }
            
            // 距离越近价值越高
            if (min_star_dist < INF) {
                move_value += (20.0f / (min_star_dist + 1));
            }
            
            // 避免移动到危险位置（接近我们的位置）
            int dist_to_us = abs(next_x - player->your_posx) + abs(next_y - player->your_posy);
            if (player->your_status > 0 && dist_to_us <= 3) {
                move_value -= 150.0f;  // 我们强化状态时，对手会避开
            }
            
            if (move_value > best_value) {
                best_value = move_value;
                best_dir = d;
            }
        }
    }
    
    // 返回预测位置
    if (best_dir != -1) {
        *pred_x = opponent_x + dx[best_dir];
        *pred_y = opponent_y + dy[best_dir];
    }
}

// 选择最佳对手追击目标
float choose_best_opponent_target(struct Player *player, int sx, int sy, int *target_x, int *target_y) {
    if (player->your_status <= 0) return -1000.0;  // 没有强化状态
    
    int opponent_x, opponent_y, opponent_score;
    
    // 获取对手信息
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return -1000.0;  // 无法获取对手信息
    }
    
    // 双重验证：确保对手分数为正数
    if (opponent_score <= 0) {
        return -1000.0;  // 对手分数为0或负数，不追击
    }
    
    // 评估追击这个对手的价值
    float opponent_value = evaluate_opponent_hunt(player, sx, sy, opponent_x, opponent_y, opponent_score);
    
    // 三重验证：确保评估价值也为正
    if (opponent_value > 0) {
        *target_x = opponent_x;
        *target_y = opponent_y;
        return opponent_value;
    }
    
    return -1000.0;  // 追击对手不可行
}

// 【优化：增强的鬼魂追击寻路，加入路径星星收益评估】
// bfs_ghost_hunt目标判定：当前位置为鬼魂或预测位置
struct GhostHuntGoalData {
    int ghost_x, ghost_y;
    int ghost_idx;
    struct Player *player;
};
static int bfs_ghost_hunt_goal(struct Player *player, int x, int y, void *userdata) {
    struct GhostHuntGoalData *data = (struct GhostHuntGoalData*)userdata;
    if (x == data->ghost_x && y == data->ghost_y) return 1;
    int pred_x, pred_y;
    predict_ghost_movement(player, data->ghost_idx, &pred_x, &pred_y);
    if (x == pred_x && y == pred_y) return 1;
    return 0;
}
// bfs_ghost_hunt路径价值：距离+星星奖励+强化状态
static float bfs_ghost_hunt_value(struct Player *player, int from_x, int from_y, int to_x, int to_y, void *userdata) {
    struct GhostHuntGoalData *data = (struct GhostHuntGoalData*)userdata;
    int dist_to_ghost = abs(to_x - data->ghost_x) + abs(to_y - data->ghost_y);
    float priority = 1000.0f / (dist_to_ghost + 1);
    char cell = player->mat[to_x][to_y];
    if (cell == 'o') priority += 50.0f;
    else if (cell == 'O') priority += 120.0f;
    if (player->your_status > 0) {
        if (player->your_status <= 3) {
            if (cell == 'o') priority += 20.0f;
            if (cell == 'O') priority += 40.0f;
        } else if (player->your_status <= 6) {
            if (cell == 'o') priority += 35.0f;
            if (cell == 'O') priority += 80.0f;
        } else {
            if (cell == 'o') priority += 50.0f;
            if (cell == 'O') priority += 120.0f;
        }
    }
    return priority;
}
int bfs_ghost_hunt(struct Player *player, int sx, int sy, int target_ghost, int *next_x, int *next_y) {
    if (target_ghost < 0 || target_ghost >= 2) return 0;
    struct GhostHuntGoalData data;
    data.ghost_x = player->ghost_posx[target_ghost];
    data.ghost_y = player->ghost_posy[target_ghost];
    data.ghost_idx = target_ghost;
    data.player = player;
    int max_search = 250;
    int tx, ty;
    if (general_bfs(player, sx, sy, &tx, &ty, max_search, bfs_ghost_hunt_goal, bfs_ghost_hunt_value, &data)) {
        // 回溯第一步
        int px = tx, py = ty;
        int prex[MAXN][MAXN], prey[MAXN][MAXN];
        int vis[MAXN][MAXN] = {0};
        int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
        qx[tail] = sx; qy[tail] = sy; tail++;
        vis[sx][sy] = 1;
        prex[sx][sy] = -1; prey[sx][sy] = -1;
        while (head < tail) {
            int x = qx[head], y = qy[head]; head++;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                    vis[nx][ny] = 1;
                    prex[nx][ny] = x; prey[nx][ny] = y;
                    qx[tail] = nx; qy[tail] = ny; tail++;
                    if (nx == tx && ny == ty) goto found_ghost;
                }
            }
        }
    found_ghost:
        while (!(prex[px][py] == sx && prey[px][py] == sy)) {
            int tpx = prex[px][py], tpy = prey[px][py];
            px = tpx; py = tpy;
        }
        *next_x = px; *next_y = py;
        return 1;
    }
    return 0;
}

// bfs_opponent_hunt目标判定：当前位置为对手或其预测位置
struct OpponentHuntGoalData {
    int target_x, target_y;
};
static int bfs_opponent_hunt_goal(struct Player *player, int x, int y, void *userdata) {
    struct OpponentHuntGoalData *data = (struct OpponentHuntGoalData*)userdata;
    if (x == data->target_x && y == data->target_y) return 1;
    for (int d = 0; d < 4; d++) {
        int pred_x = data->target_x + dx[d];
        int pred_y = data->target_y + dy[d];
        if (is_valid(player, pred_x, pred_y) && x == pred_x && y == pred_y) return 1;
    }
    return 0;
}
int bfs_opponent_hunt(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    struct OpponentHuntGoalData data;
    data.target_x = target_x;
    data.target_y = target_y;
    int max_search = 150;
    int tx, ty;
    if (general_bfs(player, sx, sy, &tx, &ty, max_search, bfs_opponent_hunt_goal, NULL, &data)) {
        // 回溯第一步
        int px = tx, py = ty;
        int prex[MAXN][MAXN], prey[MAXN][MAXN];
        int vis[MAXN][MAXN] = {0};
        int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
        qx[tail] = sx; qy[tail] = sy; tail++;
        vis[sx][sy] = 1;
        prex[sx][sy] = -1; prey[sx][sy] = -1;
        while (head < tail) {
            int x = qx[head], y = qy[head]; head++;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                    vis[nx][ny] = 1;
                    prex[nx][ny] = x; prey[nx][ny] = y;
                    qx[tail] = nx; qy[tail] = ny; tail++;
                    if (nx == tx && ny == ty) goto found_oppo;
                }
            }
        }
    found_oppo:
        while (!(prex[px][py] == sx && prey[px][py] == sy)) {
            int tpx = prex[px][py], tpy = prey[px][py];
            px = tpx; py = tpy;
        }
        *next_x = px; *next_y = py;
        return 1;
    }
    return 0;
}

// 专门的对手追击寻路（预测对手移动）
int bfs_opponent_hunt(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    int vis[MAXN][MAXN] = {0};
    int prex[MAXN][MAXN], prey[MAXN][MAXN];
    int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
    
    qx[tail] = sx; 
    qy[tail] = sy; 
    tail++;
    
    vis[sx][sy] = 1;
    prex[sx][sy] = -1; 
    prey[sx][sy] = -1;
    
    // 对手追击模式：考虑对手可能的移动
    int max_search = 150;  // 对手追击搜索深度
    int searched = 0;
    
    while (head < tail && searched < max_search) {
        int x = qx[head], y = qy[head]; 
        head++;
        searched++;
        
        // 到达对手当前位置
        if (x == target_x && y == target_y) {
            // 回溯第一步
            int px = x, py = y;
            while (!(prex[px][py] == sx && prey[px][py] == sy)) {
                int tpx = prex[px][py], tpy = prey[px][py];
                px = tpx;
                py = tpy;
            }
            *next_x = px; 
            *next_y = py;
            return 1;
        }
        
        // 考虑对手可能移动到的位置（预测拦截）
        for (int pred_d = 0; pred_d < 4; pred_d++) {
            int pred_x = target_x + dx[pred_d];
            int pred_y = target_y + dy[pred_d];
            
            if (is_valid(player, pred_x, pred_y) && x == pred_x && y == pred_y) {
                // 到达对手可能的移动位置
                int px = x, py = y;
                while (!(prex[px][py] == sx && prey[px][py] == sy)) {
                    int tpx = prex[px][py], tpy = prey[px][py];
                    px = tpx;
                    py = tpy;
                }
                *next_x = px; 
                *next_y = py;
                return 1;
            }
        }
        
        // 优先搜索更接近对手的方向
        int directions[4] = {0, 1, 2, 3};
        
        // 按距离对手的远近排序方向
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                int nx1 = x + dx[directions[i]], ny1 = y + dy[directions[i]];
                int nx2 = x + dx[directions[j]], ny2 = y + dy[directions[j]];
                
                int dist1 = abs(nx1 - target_x) + abs(ny1 - target_y);
                int dist2 = abs(nx2 - target_x) + abs(ny2 - target_y);
                
                if (dist2 < dist1) {
                    int temp = directions[i];
                    directions[i] = directions[j];
                    directions[j] = temp;
                }
            }
        }
        
        for (int i = 0; i < 4; i++) {
            int d = directions[i];
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                vis[nx][ny] = 1;
                prex[nx][ny] = x; 
                prey[nx][ny] = y;
                qx[tail] = nx; 
                qy[tail] = ny; 
                tail++;
            }
        }
    }
    
    return 0;  // 追击失败
}

// 评估对手威胁：防止对手获得超级星优势
float assess_opponent_threat(struct Player *player, int our_x, int our_y) {
    int opponent_x, opponent_y, opponent_score;
    
    // 获取对手信息
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return 0.0;  // 无对手信息，无威胁
    }
    
    float total_threat = 0.0;
    
    // 检查所有超级星，评估对手威胁
    for (int k = 0; k < power_star_cache_size; k++) {
        int power_x = power_star_cache[k].x;
        int power_y = power_star_cache[k].y;
        
        int our_dist_to_power = abs(our_x - power_x) + abs(our_y - power_y);
        int opponent_dist_to_power = abs(opponent_x - power_x) + abs(opponent_y - power_y);
        
        // 对手距离超级星更近，这是威胁！
        if (opponent_dist_to_power < our_dist_to_power) {
            float threat_level = 0.0;
            
            // 距离优势越大，威胁越大
            int distance_advantage = our_dist_to_power - opponent_dist_to_power;
            threat_level += distance_advantage * 50.0;  // 每步优势50分威胁
            
            // 对手分数越高，威胁越大（不能便宜了对面）
            if (opponent_score > player->your_score) {
                threat_level += 100.0;  // 对手领先时威胁+100
            } else if (opponent_score > player->your_score * 0.8) {
                threat_level += 50.0;   // 对手接近时威胁+50
            }
            
            // 我们处于强化状态时，对手获得超级星更危险
            if (player->your_status > 0) {
                threat_level += 150.0;  // 强化状态下对手威胁+150
                
                // 强化即将结束时更危险
                if (player->your_status <= POWER_TIME_MODERATE) {
                    threat_level += 100.0;  // 即将结束时+100
                }
            }
            
            total_threat += threat_level;
        }
    }
    
    return total_threat;
}

//【优化：地图大小自适应的安全评估函数】
int assess_danger_level(struct Player *player, int x, int y) {
    int min_ghost_dist = distance_to_ghost(player, x, y);
    int map_size = player->row_cnt * player->col_cnt;
    
    // 计算预测距离
    int predicted_min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int pred_x, pred_y;
        predict_ghost_movement(player, i, &pred_x, &pred_y);
        int pred_dist = abs(x - pred_x) + abs(y - pred_y);
        if (pred_dist < predicted_min_dist) predicted_min_dist = pred_dist;
    }
    
    // 使用更严格的安全距离
    int safe_distance, danger_distance_immediate, danger_distance_close;
    
    if (map_size <= SMALL_MAP_THRESHOLD) {
        // 小地图：距离要求更宽松，但仍要严格
        safe_distance = 2;  // 从1提升到2
        danger_distance_immediate = 1;
        danger_distance_close = 2;  // 从1提升到2
    } else if (map_size >= LARGE_MAP_THRESHOLD) {
        // 大地图：距离要求更严格
        safe_distance = 4;  // 从3提升到4
        danger_distance_immediate = 2;
        danger_distance_close = 3;
    } else {
        // 中等地图：标准距离要求
        safe_distance = 3;  // 从2提升到3
        danger_distance_immediate = 1;
        danger_distance_close = 2;
    }
    
    // 评估逃跑路径数量
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int escape_x = x + dx[d];
        int escape_y = y + dy[d];
        if (is_valid(player, escape_x, escape_y)) {
            int escape_ghost_dist = distance_to_ghost(player, escape_x, escape_y);
            if (escape_ghost_dist >= min_ghost_dist) {
                escape_routes++;
            }
        }
    }
    
    // 综合当前和预测距离评估危险等级
    int overall_dist = (min_ghost_dist + predicted_min_dist) / 2;
    
    // 地形修正：逃跑路径越少，危险等级越高
    int terrain_penalty = 0;
    if (escape_routes <= 1) {
        terrain_penalty = 1;
    } else if (escape_routes == 2) {
        terrain_penalty = 0;
    }
    
    // 强化状态评估
    if (player->your_status > 0) {
        if (player->your_status > POWER_TIME_ABUNDANT) {
            return 0;
        } else if (player->your_status > POWER_TIME_MODERATE) {
            if (overall_dist <= danger_distance_immediate) {
                return 1;
            } else {
                return 0;
            }
        } else {
            if (overall_dist <= danger_distance_immediate) {
                return 2;
            } else if (overall_dist <= danger_distance_close) {
                return 1;
            } else {
                return 0;
            }
        }
    } else {
        // 非强化状态危险评估 - 更加严格
        int base_danger = 0;
        if (overall_dist <= danger_distance_immediate) {
            base_danger = 2;  // 高危险
        } else if (overall_dist <= danger_distance_close) {
            base_danger = 1;  // 中等危险
        } else if (overall_dist <= safe_distance) {
            base_danger = (escape_routes <= 1) ? 1 : 0;
        } else {
            base_danger = 0;  // 安全
        }
        
        // 应用地形修正
        int final_danger = base_danger + terrain_penalty;
        
        // 小地图特殊处理 - 稍微宽松但仍要谨慎
        if (map_size <= SMALL_MAP_THRESHOLD) {
            if (overall_dist <= 1 && escape_routes <= 1) {
                final_danger = 2;
            } else if (overall_dist <= 1) {
                final_danger = 1;
            } else if (overall_dist <= 2) {
                final_danger = (escape_routes <= 1) ? 1 : 0;
            } else {
                final_danger = 0;
            }
        }
        
        return (final_danger > 2) ? 2 : final_danger;
    }
}

// 检查目标是否应该保持稳定
int should_keep_target(struct Player *player, int x, int y) {
    // 没有当前目标
    if (current_target_x == -1 || current_target_y == -1) {
        return 0;
    }
    
    // 目标星星已经被吃掉
    if (!is_star(player, current_target_x, current_target_y)) {
        return 0;
    }
    
    // 超级星优先策略：如果当前目标不是超级星，检查是否有更优的超级星
    char current_target_type = player->mat[current_target_x][current_target_y];
    if (current_target_type != 'O' && player->your_status <= 0) {
        // 检查是否有安全的超级星值得切换
        for (int k = 0; k < power_star_cache_size; k++) {
            int super_danger = assess_danger_level(player, power_star_cache[k].x, power_star_cache[k].y);
            int super_distance = abs(x - power_star_cache[k].x) + abs(y - power_star_cache[k].y);
            int current_distance = abs(x - current_target_x) + abs(y - current_target_y);
            
            // 如果超级星安全且不比当前目标远太多，应该切换
            if (super_danger < 2 && super_distance <= current_distance + 3) {
                return 0;  // 放弃当前目标，切换到超级星
            }
        }
    }
    
    // 目标稳定时间太短，继续保持
    if (target_stable_count < TARGET_STABILITY_MIN_COUNT) {
        return 1;
    }
    
    // 距离目标太远，可以考虑换目标
    int dist_to_target = abs(x - current_target_x) + abs(y - current_target_y);
    if (dist_to_target > TARGET_MAX_DISTANCE) {
        return 0;
    }
    
    // 目标位置变得危险
    int target_danger = assess_danger_level(player, current_target_x, current_target_y);
    if (target_danger >= 2 && player->your_status <= 0) {
        return 0;
    }
    
    return 1;  // 保持当前目标
}

// 【优化：增量更新星星缓存，避免重复遍历地图】
void build_star_cache(struct Player *player) {
    int map_size = player->row_cnt * player->col_cnt;
    
    // 【新增：对于小地图或首次构建，使用全量更新】
    if (map_size <= SMALL_MAP_THRESHOLD || cache_last_update_step == 0) {
        star_cache_size = 0;
        power_star_cache_size = 0;
        
        for (int i = 0; i < player->row_cnt; i++) {
            for (int j = 0; j < player->col_cnt; j++) {
                if (is_star(player, i, j)) {
                    if (player->mat[i][j] == 'O') {
                        // 强化星星
                        power_star_cache[power_star_cache_size].x = i;
                        power_star_cache[power_star_cache_size].y = j;
                        power_star_cache[power_star_cache_size].type = 'O';
                        power_star_cache[power_star_cache_size].valid = 1;
                        power_star_cache_size++;
                    } else {
                        // 普通星星
                        star_cache[star_cache_size].x = i;
                        star_cache[star_cache_size].y = j;
                        star_cache[star_cache_size].type = 'o';
                        star_cache[star_cache_size].valid = 1;
                        star_cache_size++;
                    }
                }
            }
        }
        cache_last_update_step = step_count;
        return;
    }
    
    // 【新增：对于大地图，使用增量更新】
    // 检查缓存中的星星是否仍然有效
    int new_star_size = 0;
    for (int i = 0; i < star_cache_size; i++) {
        if (is_star(player, star_cache[i].x, star_cache[i].y)) {
            if (new_star_size != i) {
                star_cache[new_star_size] = star_cache[i];
            }
            new_star_size++;
        }
    }
    star_cache_size = new_star_size;
    
    int new_power_star_size = 0;
    for (int i = 0; i < power_star_cache_size; i++) {
        if (is_star(player, power_star_cache[i].x, power_star_cache[i].y)) {
            if (new_power_star_size != i) {
                power_star_cache[new_power_star_size] = power_star_cache[i];
            }
            new_power_star_size++;
        }
    }
    power_star_cache_size = new_power_star_size;
    
    // 【新增：定期（每10步）进行一次范围检查，寻找新星星】
    if (step_count - cache_last_update_step >= 10) {
        int check_radius = 5;  // 检查玩家周围5步范围内的新星星
        int px = player->your_posx;
        int py = player->your_posy;
        
        for (int i = px - check_radius; i <= px + check_radius; i++) {
            for (int j = py - check_radius; j <= py + check_radius; j++) {
                if (i >= 0 && i < player->row_cnt && j >= 0 && j < player->col_cnt) {
                    if (is_star(player, i, j)) {
                        // 检查是否已经在缓存中
                        int found = 0;
                        
                        if (player->mat[i][j] == 'O') {
                            for (int k = 0; k < power_star_cache_size; k++) {
                                if (power_star_cache[k].x == i && power_star_cache[k].y == j) {
                                    found = 1;
                                    break;
                                }
                            }
                            if (!found && power_star_cache_size < MAXN * MAXN) {
                                power_star_cache[power_star_cache_size].x = i;
                                power_star_cache[power_star_cache_size].y = j;
                                power_star_cache[power_star_cache_size].type = 'O';
                                power_star_cache[power_star_cache_size].valid = 1;
                                power_star_cache_size++;
                            }
                        } else {
                            for (int k = 0; k < star_cache_size; k++) {
                                if (star_cache[k].x == i && star_cache[k].y == j) {
                                    found = 1;
                                    break;
                                }
                            }
                            if (!found && star_cache_size < MAXN * MAXN) {
                                star_cache[star_cache_size].x = i;
                                star_cache[star_cache_size].y = j;
                                star_cache[star_cache_size].type = 'o';
                                star_cache[star_cache_size].valid = 1;
                                star_cache_size++;
                            }
                        }
                    }
                }
            }
        }
        cache_last_update_step = step_count;
    }
}

// 更新AI状态信息
void update_ai_state(struct Player *player) {
    // 记录破坏者出生地（第一次遇到时记录，或检测到破坏者回到初始位置）
    if (!spawn_recorded || step_count <= 5) {
        for (int i = 0; i < 2; i++) {
            if (player->ghost_posx[i] >= 0 && player->ghost_posx[i] < player->row_cnt &&
                player->ghost_posy[i] >= 0 && player->ghost_posy[i] < player->col_cnt) {
                
                if (ghost_spawn_x[i] == -1 || ghost_spawn_y[i] == -1) {
                    ghost_spawn_x[i] = player->ghost_posx[i];
                    ghost_spawn_y[i] = player->ghost_posy[i];
                }
            }
        }
        if (step_count > 5) spawn_recorded = 1;  // 前5步记录完成
    }
    
    // 检测破坏者是否回到出生地（被吃掉后重生）
    for (int i = 0; i < 2; i++) {
        if (ghost_spawn_x[i] != -1 && ghost_spawn_y[i] != -1) {
            int current_x = player->ghost_posx[i];
            int current_y = player->ghost_posy[i];
            
            // 如果破坏者回到出生地，可能是刚被重生
            if (current_x == ghost_spawn_x[i] && current_y == ghost_spawn_y[i]) {
                // 这里可以添加重生检测逻辑（比如标记该破坏者刚重生，可能有短暂无敌期）
            }
        }
    }
    
    // 更新鬼魂历史位置，添加边界检查
    for (int i = 0; i < 2; i++) {
        if (player->ghost_posx[i] >= 0 && player->ghost_posx[i] < player->row_cnt &&
            player->ghost_posy[i] >= 0 && player->ghost_posy[i] < player->col_cnt) {
            ghost_last_x[i] = player->ghost_posx[i];
            ghost_last_y[i] = player->ghost_posy[i];
        }
    }
    
    // 更新危险等级
    danger_level = assess_danger_level(player, player->your_posx, player->your_posy);
    
    // 更新连续安全移动次数
    if (danger_level == 0) {
        consecutive_safe_moves++;
    } else {
        consecutive_safe_moves = 0;
    }
    
    // 检查是否吃到超级星
    if (player->mat[player->your_posx][player->your_posy] == 'O') {
        power_star_eaten_count++;
    }
    
    // 更新分数
    last_score = player->your_score;
    
    // 构建星星缓存
    build_star_cache(player);
}

// 地图大小自适应的启发函数
int heuristic(struct Player *player, int x1, int y1, int x2, int y2) {
    int manhattan = abs(x1 - x2) + abs(y1 - y2);
    int map_size = player->row_cnt * player->col_cnt;
    
    // 小地图(<= 15x15)：精确计算
    if (map_size <= 225) {
        if (manhattan <= 5) return manhattan;
        
        int dx = (x2 > x1) ? 1 : ((x2 < x1) ? -1 : 0);
        int dy = (y2 > y1) ? 1 : ((y2 < y1) ? -1 : 0);
        int wall_count = 0;
        
        for (int i = 1; i <= 5 && i <= manhattan; i++) {
            int check_x = x1 + dx * i;
            int check_y = y1 + dy * i;
            
            if (check_x >= 0 && check_x < player->row_cnt && 
                check_y >= 0 && check_y < player->col_cnt && 
                player->mat[check_x][check_y] == '#') {
                wall_count++;
            }
        }
        return manhattan + wall_count;
    }
    
    // 大地图(> 15x15)：简化计算，避免超时
    if (manhattan <= 3) return manhattan;
    
    // 大地图只检查前3步
    int dx = (x2 > x1) ? 1 : ((x2 < x1) ? -1 : 0);
    int dy = (y2 > y1) ? 1 : ((y2 < y1) ? -1 : 0);
    int wall_count = 0;
    
    for (int i = 1; i <= 3; i++) {
        int check_x = x1 + dx * i;
        int check_y = y1 + dy * i;
        
        if (check_x >= 0 && check_x < player->row_cnt && 
            check_y >= 0 && check_y < player->col_cnt && 
            player->mat[check_x][check_y] == '#') {
            wall_count++;
        }
    }
    
    int base_cost = manhattan + wall_count / 2;
    
    // 大地图外圈优先：如果目标在外圈，稍微降低启发式成本
    if (map_size >= LARGE_MAP_THRESHOLD) {
        int center_x = player->row_cnt / 2;
        int center_y = player->col_cnt / 2;
        int target_dist_from_center = abs(x2 - center_x) + abs(y2 - center_y);
        int max_dist_from_center = (player->row_cnt + player->col_cnt) / 2;
        
        // 如果目标在外圈，降低启发式成本（鼓励外圈路径）
        if (target_dist_from_center > max_dist_from_center * 0.7) {
            base_cost = (int)(base_cost * 0.9);  // 外圈目标降低10%成本
        }
    }
    
    return base_cost;
}

// 10x10地图专用：检查位置是否安全，标准：强化状态时即可以吃掉鬼魂--安全
int is_position_safe(struct Player *player, int x, int y) {
    // 基本有效性检查
    if (!is_valid(player, x, y)) return 0;
    
    // 对于小地图，使用放宽的安全检查
    if (player->row_cnt * player->col_cnt <= SMALL_MAP_THRESHOLD) {
        // 【优化：强化状态时智能重叠判断】
        if (player->your_status > 0) {
            // 强化状态时允许与鬼魂重叠（即可以吃掉鬼魂）
            // 但需要确保强化时间足够
    for (int i = 0; i < 2; i++) {
                // 强化状态时允许与鬼魂重叠（即可以吃掉鬼魂）
                if (x == player->ghost_posx[i] && y == player->ghost_posy[i]) {
                    // 检查强化时间是否足够
                    if (player->your_status >= 1) {
                        return 1;  // 强化状态下可以直接重叠吃掉鬼魂
                    } else {
                        return 0;  // 强化时间不足，重叠危险
                    }
                }
            }
            return 1;  // 强化状态下其他位置都安全
        }
        
        // 非强化状态：只有距离1以内才绝对危险
        for (int i = 0; i < 2; i++) {
            int ghost_x = player->ghost_posx[i];
            int ghost_y = player->ghost_posy[i];
            int dist = abs(x - ghost_x) + abs(y - ghost_y);
            
            // 只有紧邻位置才真正危险
            if (dist <= 1) {
                return 0;
            }
        }
        
        return 1;  // 距离2及以上都认为安全
    }
    
    return 1;  // 大地图默认安全
}

// 地图大小自适应的评估函数
// 评估超级星周围的普通星（用于诱导鬼魂）
float evaluate_power_star_surrounding_stars(struct Player *player, int sx, int sy, int power_star_x, int power_star_y) {
    float best_score = -1000.0f;
    int best_star_x = -1, best_star_y = -1;
    
    // 检查超级星周围3x3区域内的普通星
    for (int dx = -3; dx <= 3; dx++) {
        for (int dy = -3; dy <= 3; dy++) {
            int check_x = power_star_x + dx;
            int check_y = power_star_y + dy;
            
            if (is_valid(player, check_x, check_y) && 
                player->mat[check_x][check_y] == 'o' && 
                !star_eaten[check_x][check_y]) {
                
                float score = NORMAL_STAR_VALUE;
                int distance = heuristic(player, sx, sy, check_x, check_y);
                
                // 距离因子
                score -= distance * 8;
                
                // 诱导奖励：距离超级星越近的普通星越有价值
                int dist_to_power = abs(check_x - power_star_x) + abs(check_y - power_star_y);
                score += NORMAL_STAR_LURE_BONUS / (dist_to_power + 1);
                
                // 安全性检查
                int danger = assess_danger_level(player, check_x, check_y);
                if (danger >= 2) score -= 50;
                else if (danger == 1) score -= 15;
                
                if (score > best_score) {
                    best_score = score;
                    best_star_x = check_x;
                    best_star_y = check_y;
                }
            }
        }
    }
    
    return best_score;
}

float evaluate_star(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int star_value = (star_type == 'O') ? POWER_STAR_VALUE : NORMAL_STAR_VALUE;
    int distance = heuristic(player, sx, sy, star_x, star_y);
    int map_size = player->row_cnt * player->col_cnt;
    float score = star_value;
    
    // 距离因子
    score -= distance * 8;
    
    // 大地图优先外圈星星
    if (map_size >= LARGE_MAP_THRESHOLD) {
        int margin = 2;
        if (star_x < margin || star_x >= player->row_cnt - margin || star_y < margin || star_y >= player->col_cnt - margin) {
            score += 60; // 外圈奖励
        }
    }
    // 小地图优先近距离
    else if (map_size <= SMALL_MAP_THRESHOLD) {
        score += 30 / (distance + 1);
    }
    
    // 安全性简单惩罚
    int danger = assess_danger_level(player, star_x, star_y);
    if (danger >= 2) score -= 50;
    else if (danger == 1) score -= 15;
    
    return score;
}

// 优化的BFS（小地图和大地图通用）
int bfs_backup(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int userdata[2] = {sx, sy};
    int map_size = player->row_cnt * player->col_cnt;
    int max_search = (map_size <= SMALL_MAP_THRESHOLD) ? map_size * 2 :
                     ((map_size > LARGE_MAP_THRESHOLD) ? MAX_BFS_SEARCHES_LARGE : 500);
    // 回溯到第一步
    int tx, ty;
    if (general_bfs(player, sx, sy, &tx, &ty, max_search, bfs_backup_goal, NULL, userdata)) {
        // 回溯第一步
        int px = tx, py = ty;
        int prex[MAXN][MAXN], prey[MAXN][MAXN]; // 只用于回溯
        // 重新跑一遍BFS以获得父节点
        int vis[MAXN][MAXN] = {0};
        int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
        qx[tail] = sx; qy[tail] = sy; tail++;
        vis[sx][sy] = 1;
        prex[sx][sy] = -1; prey[sx][sy] = -1;
        while (head < tail) {
            int x = qx[head], y = qy[head]; head++;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                    vis[nx][ny] = 1;
                    prex[nx][ny] = x; prey[nx][ny] = y;
                    qx[tail] = nx; qy[tail] = ny; tail++;
                    if (nx == tx && ny == ty) goto found;
                }
            }
        }
    found:
        while (!(prex[px][py] == sx && prey[px][py] == sy)) {
            int tpx = prex[px][py], tpy = prey[px][py];
            px = tpx; py = tpy;
        }
        *next_x = px; *next_y = py;
        return 1;
    }
    return 0;
}

// bfs_backup目标判定：当前位置是星星
static int bfs_backup_goal(struct Player *player, int x, int y, void *userdata) {
    int sx = ((int*)userdata)[0];
    int sy = ((int*)userdata)[1];
    if (is_star(player, x, y) && !(x == sx && y == sy)) return 1;
    return 0;
}

// 最小堆操作
void heap_push(int node_idx) {
    if (open_size >= MAXN * MAXN) return;  // 防止堆溢出
    
    open_list[open_size++] = node_idx;
    int pos = open_size - 1;
    
    while (pos > 0) {
        int parent = (pos - 1) / 2;
        int curr_x = node_idx / MAXN, curr_y = node_idx % MAXN;
        int parent_x = open_list[parent] / MAXN, parent_y = open_list[parent] % MAXN;
        
        // 边界检查
        if (curr_x >= MAXN || curr_y >= MAXN || parent_x >= MAXN || parent_y >= MAXN) break;
        
        if (nodes[curr_x][curr_y].f_cost >= nodes[parent_x][parent_y].f_cost) break;
        
        int temp = open_list[pos];
        open_list[pos] = open_list[parent];
        open_list[parent] = temp;
        pos = parent;
    }
}

int heap_pop() {
    if (open_size == 0) return -1;
    int result = open_list[0];
    open_list[0] = open_list[--open_size];
    
    int pos = 0;
    while (pos * 2 + 1 < open_size) {
        int left = pos * 2 + 1;
        int right = pos * 2 + 2;
        int smallest = pos;
        
        int pos_x = open_list[pos] / MAXN, pos_y = open_list[pos] % MAXN;
        int left_x = open_list[left] / MAXN, left_y = open_list[left] % MAXN;
        
        // 边界检查
        if (pos_x >= MAXN || pos_y >= MAXN || left_x >= MAXN || left_y >= MAXN) break;
        
        if (nodes[left_x][left_y].f_cost < nodes[pos_x][pos_y].f_cost) {
            smallest = left;
        }
        
        if (right < open_size) {
            int right_x = open_list[right] / MAXN, right_y = open_list[right] % MAXN;
            int smallest_x = open_list[smallest] / MAXN, smallest_y = open_list[smallest] % MAXN;
            
            // 边界检查
            if (right_x >= MAXN || right_y >= MAXN || smallest_x >= MAXN || smallest_y >= MAXN) break;
            
            if (nodes[right_x][right_y].f_cost < nodes[smallest_x][smallest_y].f_cost) {
                smallest = right;
            }
        }
        
        if (smallest == pos) break;
        
        int temp = open_list[pos];
        open_list[pos] = open_list[smallest];
        open_list[smallest] = temp;
        pos = smallest;
    }
    
    return result;
}

// 大地图优化的A*（限制搜索范围）
int astar_limited(struct Player *player, int sx, int sy, int tx, int ty, int *next_x, int *next_y) {
    int map_size = player->row_cnt * player->col_cnt;
    int max_distance = abs(sx - tx) + abs(sy - ty);
    
    // 大地图且距离较远时，直接用BFS
    if (map_size > LARGE_MAP_THRESHOLD && max_distance > (FORCE_EXPLORATION_DISTANCE + 5)) {
        return 0;  // 让调用者使用BFS
    }
    
    // 初始化节点（只初始化必要区域）
    int min_x = (sx < tx ? sx : tx) - 2;
    int max_x = (sx > tx ? sx : tx) + 2;
    int min_y = (sy < ty ? sy : ty) - 2;
    int max_y = (sy > ty ? sy : ty) + 2;
    
    if (min_x < 0) min_x = 0;
    if (max_x >= player->row_cnt) max_x = player->row_cnt - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= player->col_cnt) max_y = player->col_cnt - 1;
    
    for (int i = min_x; i <= max_x; i++) {
        for (int j = min_y; j <= max_y; j++) {
            nodes[i][j].x = i;
            nodes[i][j].y = j;
            nodes[i][j].g_cost = INF;
            nodes[i][j].h_cost = heuristic(player, i, j, tx, ty);
            nodes[i][j].f_cost = INF;
            nodes[i][j].parent_x = -1;
            nodes[i][j].parent_y = -1;
            nodes[i][j].in_open = 0;
            nodes[i][j].in_closed = 0;
        }
    }
    
    // 起点
    nodes[sx][sy].g_cost = 0;
    nodes[sx][sy].f_cost = nodes[sx][sy].h_cost;
    nodes[sx][sy].in_open = 1;
    
    open_size = 0;
    heap_push(sx * MAXN + sy);
    
    int iterations = 0;
    int max_iterations = (map_size > LARGE_MAP_THRESHOLD) ? MAX_ASTAR_ITERATIONS_LARGE : MAX_ASTAR_ITERATIONS_SMALL;
    
    while (open_size > 0 && iterations < max_iterations) {
        int current_idx = heap_pop();
        int x = current_idx / MAXN, y = current_idx % MAXN;
        iterations++;
        
        nodes[x][y].in_open = 0;
        nodes[x][y].in_closed = 1;
        
        if (x == tx && y == ty) {
            // 回溯路径找到第一步
            int px = tx, py = ty;
            while (nodes[px][py].parent_x != sx || nodes[px][py].parent_y != sy) {
                int temp_x = nodes[px][py].parent_x;
                int temp_y = nodes[px][py].parent_y;
                px = temp_x;
                py = temp_y;
            }
            *next_x = px;
            *next_y = py;
            return 1;
        }
        
        // 探索邻居
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (!is_valid(player, nx, ny) || nodes[nx][ny].in_closed) continue;
            
            // 确保节点在初始化范围内
            if (nx < min_x || nx > max_x || ny < min_y || ny > max_y) continue;
            
            int new_g_cost = nodes[x][y].g_cost + 1;
            
            if (!nodes[nx][ny].in_open || new_g_cost < nodes[nx][ny].g_cost) {
                nodes[nx][ny].g_cost = new_g_cost;
                nodes[nx][ny].f_cost = new_g_cost + nodes[nx][ny].h_cost;
                nodes[nx][ny].parent_x = x;
                nodes[nx][ny].parent_y = y;
                
                if (!nodes[nx][ny].in_open) {
                    nodes[nx][ny].in_open = 1;
                    heap_push(nx * MAXN + ny);
                }
            }
        }
    }
    
    return 0;  // 超时或找不到路径
}

//【优化：增强的出生地伏击策略评估】
float evaluate_spawn_point_approach(struct Player *player, int sx, int sy, int ghost_idx) {
    if (player->your_status <= 0) return -1000.0;  // 非强化状态无法追击
    if (ghost_idx < 0 || ghost_idx >= 2) return -1000.0;  // 无效鬼魂索引
    if (ghost_spawn_x[ghost_idx] == -1 || ghost_spawn_y[ghost_idx] == -1) return -1000.0;  // 未记录出生地
    
    int spawn_x = ghost_spawn_x[ghost_idx];
    int spawn_y = ghost_spawn_y[ghost_idx];
    int ghost_x = player->ghost_posx[ghost_idx];
    int ghost_y = player->ghost_posy[ghost_idx];
    
    // 如果鬼魂就在出生地附近，直接追击更好
    int ghost_spawn_dist = abs(ghost_x - spawn_x) + abs(ghost_y - spawn_y);
    if (ghost_spawn_dist <= 2) {
        return -500.0;  // 鬼魂在出生地附近，直接追击
    }
    
    // 计算我们到出生地的距离
    int our_spawn_dist = abs(sx - spawn_x) + abs(sy - spawn_y);
    
    // 计算鬼魂到出生地的距离
    int ghost_to_spawn_dist = abs(ghost_x - spawn_x) + abs(ghost_y - spawn_y);
    
    // 基础策略：如果鬼魂距离出生地较远，我们可以先向出生地靠近等待
    float score = 0.0;
    
    // 【优化：更积极的出生地策略】
    if (player->your_status > 5) {  // 【优化：从6降到5】
        // 鬼魂距离出生地很远，我们有机会在出生地等待
        if (ghost_to_spawn_dist >= 4) {  // 【优化：从5降到4】
            score = GHOST_HUNT_BASE_SCORE * 0.8;  // 【优化：从70%提升到80%】
            
            // 距离因子：我们越接近出生地价值越高
            float distance_factor = 1.0 / (our_spawn_dist + 1);
            score *= distance_factor;
            
            // 【优化：更宽松的时间因子调整】
            if (player->your_status >= 10) {  // 【优化：从12降到10】
                score *= 1.5;  // 【优化：从1.4提升到1.5】时间非常充足
            } else if (player->your_status >= 7) {  // 【优化：从9降到7】
                score *= 1.3;  // 【优化：从1.2提升到1.3】时间充足
            } else {
                score *= 1.1;  // 【优化：从1.0提升到1.1】中等时间也要奖励
            }
            
            // 如果鬼魂正在向出生地移动，增加价值
            int pred_x, pred_y;
            predict_ghost_movement(player, ghost_idx, &pred_x, &pred_y);
            int pred_spawn_dist = abs(pred_x - spawn_x) + abs(pred_y - spawn_y);
            if (pred_spawn_dist < ghost_to_spawn_dist) {
                score *= 1.4;  // 【优化：从1.3提升到1.4】鬼魂正在向出生地移动
            }
            
            // 【新增：出生地附近的伏击奖励】
            if (our_spawn_dist <= 3) {
                score += 120.0;  // 【优化：从100.0提升到120.0】接近出生地的伏击奖励
                
                // 【新增：完美伏击位置奖励】
                if (our_spawn_dist <= 1) {
                    score += 180.0;  // 【优化：从150.0提升到180.0】在出生地附近的完美伏击奖励
                }
            }
            
            // 【新增：地形优势评估】
            // 检查出生地周围的地形，死胡同或通道口更有利
            int spawn_exits = 0;
            for (int d = 0; d < 4; d++) {
                int check_x = spawn_x + dx[d];
                int check_y = spawn_y + dy[d];
                if (is_valid(player, check_x, check_y)) {
                    spawn_exits++;
                }
            }
            
            if (spawn_exits <= 2) {
                score += 100.0;  // 【优化：从80.0提升到100.0】出生地在死胡同或通道，更容易伏击
            } else if (spawn_exits == 3) {
                score += 50.0;  // 【优化：从40.0提升到50.0】出生地选择有限，有一定优势
            }
        }
    } else if (player->your_status > 3) {  // 【优化：时间要求保持不变】
        // 【优化：时间有限时的伏击策略】
        if (our_spawn_dist <= 5 && ghost_to_spawn_dist >= 5) {  // 【优化：从4/6调整到5/5】
            score = GHOST_HUNT_BASE_SCORE * 0.6;  // 【优化：从50%提升到60%】
            score *= (1.0 / (our_spawn_dist + 1));
            
            // 【新增：时间有限时的伏击奖励】
            if (our_spawn_dist <= 2) {
                score += 80.0;  // 【优化：从60.0提升到80.0】时间有限但位置好的伏击奖励
            }
        }
    } else if (player->your_status > 1) {  // 【新增：最后时刻的伏击策略】
        // 【新增：最后时刻的拼死伏击】
        if (our_spawn_dist <= 2 && ghost_to_spawn_dist >= 2) {
            score = GHOST_HUNT_BASE_SCORE * 0.1;  // 最后时刻的伏击仍有价值
            score *= (1.0 / (our_spawn_dist + 1));
            
            // 最后时刻的伏击奖励
            if (our_spawn_dist <= 1) {
                score += 60.0;  // 最后时刻的完美伏击
            }
        }
    }
    
    // 【新增：多鬼魂协同伏击奖励】
    // 如果另一个鬼魂也在向出生地移动，增加伏击价值
    int other_ghost = (ghost_idx == 0) ? 1 : 0;
    int other_ghost_x = player->ghost_posx[other_ghost];
    int other_ghost_y = player->ghost_posy[other_ghost];
    int other_ghost_to_spawn = abs(other_ghost_x - spawn_x) + abs(other_ghost_y - spawn_y);
    
    if (other_ghost_to_spawn <= 6 && ghost_to_spawn_dist >= 4) {
        score += 60.0;  // 【优化：从40.0提升到60.0】可能的双鬼伏击机会
    }
    
    // 【新增：分数压力下的伏击策略 - 动态调整阈值】
    // 根据地图和游戏进展动态调整分数阈值
    int target_score = 10000;  // 目标分数
    int low_score_threshold = target_score / 3;    // 3333分 - 分数严重不足
    int mid_score_threshold = target_score / 2;    // 5000分 - 分数适中
    int high_score_threshold = target_score * 2/3; // 6667分 - 分数接近目标
    
    if (player->your_score < low_score_threshold) {
        score += 80.0;  // 分数严重不足时，伏击策略极其重要
    } else if (player->your_score < mid_score_threshold) {
        score += 60.0;  // 分数不足时，伏击策略很重要
    } else if (player->your_score < high_score_threshold) {
        score += 40.0;  // 分数接近目标时，适度提升
    } else if (player->your_score < target_score) {
        score += 20.0;  // 接近目标时，轻微提升
    }
    
    return score;
}
    
// 选择最佳追击目标（包括出生地策略）
int choose_best_ghost_target(struct Player *player, int sx, int sy) {
    if (player->your_status <= 0) return -1;  // 没有强化状态
    
    float best_score = -1000.0;
    int best_ghost = -1;
    
    for (int i = 0; i < 2; i++) {
        // 评估直接追击的价值
        float direct_score = evaluate_ghost_hunt(player, sx, sy, i);
        
        // 评估出生地策略的价值
        float spawn_score = evaluate_spawn_point_approach(player, sx, sy, i);
        
        // 选择最优策略
        if (direct_score > best_score && direct_score > 0) {
            best_score = direct_score;
            best_ghost = i;
        }
        
        if (spawn_score > best_score && spawn_score > 0) {
            best_score = spawn_score;
            best_ghost = i;
        }
    }
    
    return best_ghost;
}

// 专门的出生地接近寻路（强化状态专用）
int bfs_spawn_approach(struct Player *player, int sx, int sy, int target_ghost, int *next_x, int *next_y) {
    if (target_ghost < 0 || target_ghost >= 2) return 0;
    if (ghost_spawn_x[target_ghost] == -1 || ghost_spawn_y[target_ghost] == -1) return 0;
    struct SpawnApproachGoalData data;
    data.spawn_x = ghost_spawn_x[target_ghost];
    data.spawn_y = ghost_spawn_y[target_ghost];
    int max_search = 150;
    int tx, ty;
    if (general_bfs(player, sx, sy, &tx, &ty, max_search, bfs_spawn_approach_goal, NULL, &data)) {
        // 回溯第一步
        int px = tx, py = ty;
        int prex[MAXN][MAXN], prey[MAXN][MAXN];
        int vis[MAXN][MAXN] = {0};
        int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
        qx[tail] = sx; qy[tail] = sy; tail++;
        vis[sx][sy] = 1;
        prex[sx][sy] = -1; prey[sx][sy] = -1;
        while (head < tail) {
            int x = qx[head], y = qy[head]; head++;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                    vis[nx][ny] = 1;
                    prex[nx][ny] = x; prey[nx][ny] = y;
                    qx[tail] = nx; qy[tail] = ny; tail++;
                    if (nx == tx && ny == ty) goto found_spawn;
                }
            }
        }
    found_spawn:
        while (!(prex[px][py] == sx && prey[px][py] == sy)) {
            int tpx = prex[px][py], tpy = prey[px][py];
            px = tpx; py = tpy;
        }
        *next_x = px; *next_y = py;
        return 1;
    }
    return 0;
}

//【新增：策略0 - 非强化状态智能远离策略】
int handle_non_power_escape_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    if (player->your_status != 0) return 0;  // 只处理非强化状态
    
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    // 动态危险阈值
    int map_size = player->row_cnt * player->col_cnt;
    int danger_threshold = (map_size <= SMALL_MAP_THRESHOLD) ? 3 : 
                          (map_size >= LARGE_MAP_THRESHOLD) ? 5 : 4;
    
    // 如果距离破坏者太近，需要主动远离
    if (min_ghost_dist <= danger_threshold) {
        int current_danger = assess_danger_level(player, x, y);
        int emergency_mode = (current_danger >= 2 || min_ghost_dist <= 2);
        
        int best_escape_dir = -1;
        float max_escape_value = -1000.0f;
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny)) {
                float escape_value = 0.0f;
                
                // 1. 基础距离安全价值（权重：50%）
                int new_ghost_dist = distance_to_ghost(player, nx, ny);
                escape_value += new_ghost_dist * 0.5f;
                
                // 2. 鬼魂移动预测（权重：30%）
                for (int i = 0; i < 2; i++) {
                    int pred_x, pred_y;
                    predict_ghost_movement(player, i, &pred_x, &pred_y);
                    int pred_dist = abs(nx - pred_x) + abs(ny - pred_y);
                    
                    if (emergency_mode) {
                        if (pred_dist <= 2) escape_value -= 0.3f;
                        else if (pred_dist <= 3) escape_value -= 0.2f;
                        else if (pred_dist <= 4) escape_value -= 0.1f;
                    } else {
                        if (pred_dist <= 2) escape_value -= 0.2f;
                        else if (pred_dist <= 3) escape_value -= 0.1f;
                    }
                }
                
                // 3. 出路安全性评估（权重：15%）
                int exit_count = 0, safe_exit_count = 0;
                for (int dd = 0; dd < 4; dd++) {
                    int check_x = nx + dx[dd], check_y = ny + dy[dd];
                    if (is_valid(player, check_x, check_y)) {
                        exit_count++;
                        int exit_ghost_dist = distance_to_ghost(player, check_x, check_y);
                        if (exit_ghost_dist >= new_ghost_dist) safe_exit_count++;
                    }
                }
                escape_value += (exit_count * 0.05f + safe_exit_count * 0.1f);
                
                // 4. 星星收集安全性（权重：5%）
                char cell = player->mat[nx][ny];
                if (cell == 'o' && (!emergency_mode || (safe_exit_count > 0 && new_ghost_dist >= 2))) {
                    escape_value += 0.05f;
                } else if (cell == 'O' && (!emergency_mode || (safe_exit_count > 0 && new_ghost_dist >= 2))) {
                    escape_value += 0.1f;
                }
                
                // 5. 避免震荡（权重：5%）
                if (nx == last_x && ny == last_y) {
                    escape_value -= 0.05f;
                }
                
                if (escape_value > max_escape_value) {
                    max_escape_value = escape_value;
                    best_escape_dir = d;
                }
            }
        }
        
        if (best_escape_dir != -1) {
            *next_x = x + dx[best_escape_dir];
            *next_y = y + dy[best_escape_dir];
            return 1;
        }
    }
    
    return 0;
}

//【新增：策略1 - 强化状态追击策略】
int handle_power_hunt_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    if (player->your_status <= 0) return 0;  // 只处理强化状态
    
    int remaining_power_time = player->your_status;
    
    // 强化状态下立即追击最近的鬼魂，但要考虑时间成本
    int best_ghost = -1;
    int min_ghost_dist = INF;
    
    // 找最近的鬼魂
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_ghost_dist) {
            min_ghost_dist = dist;
            best_ghost = i;
        }
    }
    
    // 评估对手追击的价值
    int opponent_x, opponent_y, opponent_score;
    int should_hunt_opponent = 0;
    float opponent_hunt_value = 0;
    
    if (get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score) && opponent_score > 0) {
        int opponent_dist = abs(x - opponent_x) + abs(y - opponent_y);
        
        // 【优化：借鉴优秀代码的权重设置】
        // 计算对手分值的一半 vs 破坏者得分(300分)的比较
        float opponent_half_score = opponent_score * 0.5f;
        float ghost_score = GHOST_HUNT_BASE_SCORE;  // 使用统一权重常量
        
        // 如果对手分值的一半大于鬼魂得分，优先追击对手
        if (opponent_half_score > ghost_score && remaining_power_time >= 4) {
            if (opponent_dist <= remaining_power_time - 1) {
                should_hunt_opponent = 1;
                opponent_hunt_value = opponent_half_score;
            }
        }
        // 即使对手分值一半小于鬼魂得分，但如果对手分数很高也值得考虑
        else if (opponent_score > 600 && remaining_power_time >= 7) {
            if (opponent_dist <= remaining_power_time - 2) {
                should_hunt_opponent = 1;
                opponent_hunt_value = opponent_half_score;
            }
        }
    }
    
    // 【优化：借鉴优秀代码的优先级判断】
    // 优先级判断：对手价值 vs 破坏者价值
    if (should_hunt_opponent && (opponent_hunt_value > GHOST_HUNT_BASE_SCORE || min_ghost_dist > 8)) {
        // 追击对手
        if (bfs_opponent_hunt(player, x, y, opponent_x, opponent_y, next_x, next_y)) {
            return 1;  // 成功处理
        }
    }
    
    if (best_ghost >= 0) {
        // 【强化：更激进的追击决策，扩大追击范围】
        int should_chase = 0;
        
        if (remaining_power_time >= 7) {
            should_chase = (min_ghost_dist <= 20);  // 【强化：从12提升到20】时间充足时大幅扩大追击范围
        } else if (remaining_power_time >= 4) {
            should_chase = (min_ghost_dist <= 15);  // 【强化：从7提升到15】时间适中时大幅扩大追击范围
        } else if (remaining_power_time >= 2) {
            should_chase = (min_ghost_dist <= 10);  // 【强化：从4提升到10】时间紧张时仍要扩大追击范围
        } else {
            should_chase = (min_ghost_dist <= 6);   // 【强化：从2提升到6】最后时刻也要积极追击
            
            // 最后3回合的特殊处理
            if (remaining_power_time <= 3 && remaining_power_time >= 1) {
                if (min_ghost_dist <= 2) {
                    should_chase = 1;
                }
                // 考虑鬼魂预测位置的追击机会
                else {
                    int pred_x, pred_y;
                    predict_ghost_movement(player, best_ghost, &pred_x, &pred_y);
                    int pred_dist = abs(x - pred_x) + abs(y - pred_y);
                    
                    if (pred_dist <= 2) {
                        should_chase = 1;
                    }
                }
            }
        }
        
        if (should_chase) {
            // 智能追击路径
            if (bfs_ghost_hunt(player, x, y, best_ghost, next_x, next_y)) {
                return 1;  // 成功处理
            }
            
            // BFS失败，考虑直接靠近
            int ghost_x = player->ghost_posx[best_ghost];
            int ghost_y = player->ghost_posy[best_ghost];
            int best_dir = -1;
            float best_move_value = -1000.0;
            
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (is_valid(player, nx, ny)) {
                    float move_value = 0;
                    
                    // 基础追击价值
                    int dist_to_ghost = abs(nx - ghost_x) + abs(ny - ghost_y);
                    int current_dist = abs(x - ghost_x) + abs(y - ghost_y);
                    if (dist_to_ghost < current_dist) {
                        move_value += 100.0;
                    }
                    
                    // 【调整：强化状态下降低路径星星权重，专注鬼魂追击】
                    char cell = player->mat[nx][ny];
                    if (cell == 'o') {
                        move_value += 20.0;  // 【调整：从50.0降低到20.0】降低普通星星权重
                    } else if (cell == 'O') {
                        move_value += 30.0;  // 【调整：从120.0大幅降低到30.0】大幅降低超级星权重，专注鬼魂追击
                    }
                    
                    // 安全性考虑
                    int danger = assess_danger_level(player, nx, ny);
                    move_value -= danger * 5.0;
                    
                    if (move_value > best_move_value) {
                        best_move_value = move_value;
                        best_dir = d;
                    }
                }
            }
            
            if (best_dir != -1) {
                *next_x = x + dx[best_dir];
                *next_y = y + dy[best_dir];
                return 1;  // 成功处理
            }
        }
    }
    
    // 【调整：强化状态下严格限制超级星收集，仅在极端情况下考虑】
    if (remaining_power_time <= 2) {  // 【调整：从5调整到2】只有最后2回合且没有鬼魂可追时才考虑超级星
        build_star_cache(player);
        
        // 【新增：只有在没有可追击的鬼魂时才考虑超级星】
        int any_ghost_reachable = 0;
        for (int i = 0; i < 2; i++) {
            int ghost_dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
            if (ghost_dist <= remaining_power_time + 1) {
                any_ghost_reachable = 1;
                break;
            }
        }
        
        // 只有在没有鬼魂可追击时才寻找超级星
        if (!any_ghost_reachable) {
            int nearest_power_star_x = -1, nearest_power_star_y = -1;
            int min_power_dist = INF;
            
            for (int k = 0; k < power_star_cache_size; k++) {
                int dist = abs(x - power_star_cache[k].x) + abs(y - power_star_cache[k].y);
                if (dist < min_power_dist && dist <= remaining_power_time) {  // 【调整：移除+2的宽松条件】
                    min_power_dist = dist;
                    nearest_power_star_x = power_star_cache[k].x;
                    nearest_power_star_y = power_star_cache[k].y;
                }
            }
            
            if (nearest_power_star_x != -1) {
                if (astar_limited(player, x, y, nearest_power_star_x, nearest_power_star_y, next_x, next_y)) {
                    return 1;  // 成功处理
                }
            }
        }
        
        // 如果没有合适的超级星，尽可能收集普通星星
        int nearest_star_x = -1, nearest_star_y = -1;
        int min_star_dist = INF;
        
        for (int k = 0; k < star_cache_size; k++) {
            int dist = abs(x - star_cache[k].x) + abs(y - star_cache[k].y);
            if (dist < min_star_dist && dist <= remaining_power_time) {
                int star_danger = assess_danger_level(player, star_cache[k].x, star_cache[k].y);
                if (star_danger <= 1) {
                    min_star_dist = dist;
                    nearest_star_x = star_cache[k].x;
                    nearest_star_y = star_cache[k].y;
                }
            }
        }
        
        if (nearest_star_x != -1) {
            if (astar_limited(player, x, y, nearest_star_x, nearest_star_y, next_x, next_y)) {
                return 1;  // 成功处理
            }
        }
    }
    
    return 0;  // 未处理
}

//【新增：策略2 - 智能星星收集策略】
int handle_star_collection_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    // 安全检查：如果当前处于高危险状态，不进行星星收集
    int current_danger = assess_danger_level(player, x, y);
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    if (current_danger >= 2 || min_ghost_dist <= CRITICAL_DISTANCE) {
        return 0; // 让更高优先级的安全策略处理
    }
    
    build_star_cache(player);
    int best_star_x = -1, best_star_y = -1;
    float best_score = -1000.0;
    int map_size = player->row_cnt * player->col_cnt;
    
    // 计算与鬼魂的距离
    int ghost_distance = distance_to_ghost(player, x, y);
    
    // 获取对手信息
    int opponent_x = -1, opponent_y = -1, opponent_score = 0;
    get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score);
    int opponent_distance = INF;
    if (opponent_x != -1 && opponent_y != -1) {
        opponent_distance = abs(x - opponent_x) + abs(y - opponent_y);
    }
    
    // 大地图智能诱导策略
    if (map_size >= LARGE_MAP_THRESHOLD) {
        // 如果鬼魂距离过远（超过20），优先清理超级星周围的普通星来诱导鬼魂
        // 但如果对手距离过近（≤5），则优先吃超级星
        if (ghost_distance > GHOST_DISTANCE_FAR && opponent_distance > 5) {
            float best_lure_score = -1000.0f;
            int best_lure_x = -1, best_lure_y = -1;
            
            // 寻找超级星周围的普通星
            for (int k = 0; k < power_star_cache_size; k++) {
                int power_x = power_star_cache[k].x, power_y = power_star_cache[k].y;
                
                // 检查超级星周围3x3区域内的普通星
                for (int dx = -3; dx <= 3; dx++) {
                    for (int dy = -3; dy <= 3; dy++) {
                        int check_x = power_x + dx;
                        int check_y = power_y + dy;
                        
                        if (is_valid(player, check_x, check_y) && 
                            player->mat[check_x][check_y] == 'o' && 
                            !star_eaten[check_x][check_y]) {
                            
                            float lure_score = NORMAL_STAR_VALUE;
                            int distance = heuristic(player, x, y, check_x, check_y);
                            
                            // 距离因子
                            lure_score -= distance * 8;
                            
                            // 诱导奖励：距离超级星越近的普通星越有价值
                            int dist_to_power = abs(check_x - power_x) + abs(check_y - power_y);
                            lure_score += NORMAL_STAR_LURE_BONUS / (dist_to_power + 1);
                            
                            // 安全性检查
                            int danger = assess_danger_level(player, check_x, check_y);
                            if (danger >= 2) lure_score -= 50;
                            else if (danger == 1) lure_score -= 15;
                            
                            if (lure_score > best_lure_score) {
                                best_lure_score = lure_score;
                                best_lure_x = check_x;
                                best_lure_y = check_y;
                            }
                        }
                    }
                }
            }
            
            // 如果找到合适的诱导目标，优先选择
            if (best_lure_x != -1 && best_lure_score > -500.0f) {
                if (astar_limited(player, x, y, best_lure_x, best_lure_y, next_x, next_y)) {
                    return 1;
                }
            }
        }
        
        // 如果鬼魂距离过近（小于5），或对手距离过近（≤5），优先吃掉超级星
        if (ghost_distance < GHOST_DISTANCE_NEAR || opponent_distance <= 5) {
            for (int k = 0; k < power_star_cache_size; k++) {
                int star_x = power_star_cache[k].x, star_y = power_star_cache[k].y;
                float score = evaluate_star(player, x, y, star_x, star_y);
                
                // 紧急情况超级星奖励
                score += POWER_STAR_URGENT_BONUS;
                
                // 大地图优先外圈
                int margin = 2;
                if (star_x < margin || star_x >= player->row_cnt - margin || star_y < margin || star_y >= player->col_cnt - margin) {
                    score += 60;
                }
                
                if (score > best_score) {
                    best_score = score;
                    best_star_x = star_x;
                    best_star_y = star_y;
                }
            }
            
            // 如果找到超级星，优先选择
            if (best_star_x != -1) {
                if (astar_limited(player, x, y, best_star_x, best_star_y, next_x, next_y)) {
                    return 1;
                }
            }
        }
    }
    
    // 常规星星收集策略
    // 优先考虑超级星
    for (int k = 0; k < power_star_cache_size; k++) {
        int star_x = power_star_cache[k].x, star_y = power_star_cache[k].y;
        float score = evaluate_star(player, x, y, star_x, star_y);
        
        // 大地图优先外圈
        if (map_size >= LARGE_MAP_THRESHOLD) {
            int margin = 2;
            if (star_x < margin || star_x >= player->row_cnt - margin || star_y < margin || star_y >= player->col_cnt - margin) {
                score += 60;
            }
        }
        
        if (score > best_score) {
            best_score = score;
            best_star_x = star_x;
            best_star_y = star_y;
        }
    }
    
    // 普通星星
    for (int k = 0; k < star_cache_size; k++) {
        int star_x = star_cache[k].x, star_y = star_cache[k].y;
        float score = evaluate_star(player, x, y, star_x, star_y);
        
        // 大地图优先外圈
        if (map_size >= LARGE_MAP_THRESHOLD) {
            int margin = 2;
            if (star_x < margin || star_x >= player->row_cnt - margin || star_y < margin || star_y >= player->col_cnt - margin) {
                score += 60;
            }
        }
        
        if (score > best_score) {
            best_score = score;
            best_star_x = star_x;
            best_star_y = star_y;
        }
    }
    
    if (best_star_x != -1) {
        if (astar_limited(player, x, y, best_star_x, best_star_y, next_x, next_y)) {
            return 1;
        }
    }
    return 0;
}

//【新增：策略3 - 安全移动策略】
int handle_safe_movement_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    // 检查当前危险等级
    int current_danger = assess_danger_level(player, x, y);
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    // 如果当前处于高危险状态，强制使用紧急避险
    if (current_danger >= 2 || min_ghost_dist <= EMERGENCY_DISTANCE) {
        // 紧急避险：选择最安全的方向
        int best_dir = -1;
        float max_safety = -1000.0f;
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (!is_valid(player, nx, ny)) continue;
            
            int new_ghost_dist = distance_to_ghost(player, nx, ny);
            float safety = new_ghost_dist * 10.0f;
            
            // 检查鬼魂预测位置
            for (int i = 0; i < 2; i++) {
                int pred_x, pred_y;
                predict_ghost_movement(player, i, &pred_x, &pred_y);
                int pred_dist = abs(nx - pred_x) + abs(ny - pred_y);
                if (pred_dist <= 1) {
                    safety -= 50.0f;
                } else if (pred_dist <= 2) {
                    safety -= 20.0f;
                }
            }
            
            // 死胡同惩罚
            int exit_routes = 0;
            for (int dd = 0; dd < 4; dd++) {
                int check_x = nx + dx[dd], check_y = ny + dy[dd];
                if (is_valid(player, check_x, check_y)) exit_routes++;
            }
            if (exit_routes <= 1) safety -= 30.0f;
            
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
    }
    
    // 正常安全移动（带震荡检查）
    return choose_safe_direction_with_oscillation_check(player, x, y, next_x, next_y);
}

//【新增：策略4 - 智能探索策略】
int handle_intelligent_exploration_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_x = x, best_y = y;
    float max_score = -1000.0f;
    
    // 检测震荡模式
    int oscillation_type = detect_path_oscillation(x, y);
    
    // 如果检测到震荡，优先尝试打破震荡
    if (oscillation_type > 0) {
        int break_x, break_y;
        if (break_oscillation_pattern(player, x, y, &break_x, &break_y)) {
            int ghost_dist = distance_to_ghost(player, break_x, break_y);
            int danger = assess_danger_level(player, break_x, break_y);
            
            if (danger < 2 || ghost_dist > 2) {
                *next_x = break_x;
                *next_y = break_y;
                return 1;
            }
        }
    }
    
    // 计算地图中心
    int center_x = player->row_cnt / 2;
    int center_y = player->col_cnt / 2;
    int map_size = player->row_cnt * player->col_cnt;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            float explore_score = 0.0f;
            
            // 1. 基础安全价值（权重：30%）
            int ghost_dist = distance_to_ghost(player, nx, ny);
            explore_score += ghost_dist * 0.3f;
            
            // 2. 外圈优先策略（权重：40%）
            int dist_from_center = abs(nx - center_x) + abs(ny - center_y);
            int max_dist_from_center = (player->row_cnt + player->col_cnt) / 2;
            
            // 外圈奖励：距离中心越远奖励越高
            float outer_ring_bonus = (float)dist_from_center / max_dist_from_center;
            explore_score += outer_ring_bonus * 0.4f;
            
            // 3. 边缘探索奖励（权重：15%）
            int margin = (map_size >= LARGE_MAP_THRESHOLD) ? 3 : 2;
            if (nx < margin || nx >= player->row_cnt - margin || 
                ny < margin || ny >= player->col_cnt - margin) {
                explore_score += 0.15f;
            }
            
            // 4. 角落探索奖励（权重：10%）
            if ((nx <= 1 || nx >= player->row_cnt - 2) && 
                (ny <= 1 || ny >= player->col_cnt - 2)) {
                explore_score += 0.1f;
            }
            
            // 5. 避免震荡（权重：5%）
            if (nx == last_x && ny == last_y) {
                explore_score -= 0.05f;
            }
            
            // 6. 历史位置惩罚
            for (int i = 0; i < path_history_count; i++) {
                if (path_history_x[i] == nx && path_history_y[i] == ny) {
                    int recency = path_history_count - i;
                    explore_score -= recency * 0.01f;
                }
            }
            
            // 7. 震荡处理
            if (oscillation_type >= 2) {
                // 严重震荡：强制选择垂直或水平方向
                int horizontal_moves = 0, vertical_moves = 0;
                for (int i = 1; i < path_history_count && i < 4; i++) {
                    int idx_curr = (path_history_head - i + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
                    int idx_prev = (path_history_head - i - 1 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
                    
                    if (path_history_x[idx_curr] != path_history_x[idx_prev]) horizontal_moves++;
                    if (path_history_y[idx_curr] != path_history_y[idx_prev]) vertical_moves++;
                }
                
                if (horizontal_moves > vertical_moves) {
                    if (d == 0 || d == 2) explore_score += 0.3f;  // 上下移动
                } else {
                    if (d == 1 || d == 3) explore_score += 0.3f;  // 左右移动
                }
            } else if (oscillation_type == 1) {
                int dist_from_last = abs(nx - last_x) + abs(ny - last_y);
                if (dist_from_last > 1) explore_score += 0.1f;
            }
            
            if (explore_score > max_score) {
                max_score = explore_score;
                best_x = nx;
                best_y = ny;
            }
        }
    }
    
    // 最终确保有有效移动
    if (best_x == x && best_y == y) {
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !(nx == last_x && ny == last_y)) {
                best_x = nx;
                best_y = ny;
                break;
            }
        }
        
        if (best_x == x && best_y == y) {
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (is_valid(player, nx, ny)) {
                    best_x = nx;
                    best_y = ny;
                    break;
                }
            }
        }
    }
    
    *next_x = best_x;
    *next_y = best_y;
    return 1;
}

/**
 * 核心决策函数：解决震荡、强化追击、路径优化问题
 * 优先级：强化状态追击 > 超级星收集 > 普通星收集 > 智能探索
 */
 
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    update_ai_state(player);
    
    // 添加当前位置到路径历史记录
    add_to_path_history(x, y);
    
    // 标记当前位置的星星为已吃
    char current_cell = player->mat[x][y];
    if ((current_cell == 'o' || current_cell == 'O') && !star_eaten[x][y]) {
        star_eaten[x][y] = 1;
    }
    
    int next_x, next_y;
    clock_t current_time;
    double elapsed_ms;
    
    // 策略0：原有智能远离策略（最高优先级）
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_non_power_escape_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; 
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略1：强化状态追击（次高优先级）
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_power_hunt_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略2：安全移动（提升优先级，避免往鬼魂脸上送）
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_safe_movement_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略3：智能星星收集（降低优先级，确保安全第一）
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_star_collection_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略4：智能探索（最后保障）
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_intelligent_exploration_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
timeout_return:
    
    // 最终fallback：确保有有效移动
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            last_x = x; last_y = y;
            struct Point ret = {nx, ny};
            return ret;
        }
    }
    
    // 如果实在没有有效移动，返回当前位置
    last_x = x; last_y = y;
    struct Point ret = {x, y};
    return ret;
}

// bfs_spawn_approach目标判定：当前位置为出生点或其附近
struct SpawnApproachGoalData {
    int spawn_x, spawn_y;
};
static int bfs_spawn_approach_goal(struct Player *player, int x, int y, void *userdata) {
    struct SpawnApproachGoalData *data = (struct SpawnApproachGoalData*)userdata;
    if (x == data->spawn_x && y == data->spawn_y) return 1;
    int spawn_dist = abs(x - data->spawn_x) + abs(y - data->spawn_y);
    if (spawn_dist <= 2) return 1;
    return 0;
}
int bfs_spawn_approach(struct Player *player, int sx, int sy, int target_ghost, int *next_x, int *next_y) {
    if (target_ghost < 0 || target_ghost >= 2) return 0;
    if (ghost_spawn_x[target_ghost] == -1 || ghost_spawn_y[target_ghost] == -1) return 0;
    struct SpawnApproachGoalData data;
    data.spawn_x = ghost_spawn_x[target_ghost];
    data.spawn_y = ghost_spawn_y[target_ghost];
    int max_search = 150;
    int tx, ty;
    if (general_bfs(player, sx, sy, &tx, &ty, max_search, bfs_spawn_approach_goal, NULL, &data)) {
        // 回溯第一步
        int px = tx, py = ty;
        int prex[MAXN][MAXN], prey[MAXN][MAXN];
        int vis[MAXN][MAXN] = {0};
        int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
        qx[tail] = sx; qy[tail] = sy; tail++;
        vis[sx][sy] = 1;
        prex[sx][sy] = -1; prey[sx][sy] = -1;
        while (head < tail) {
            int x = qx[head], y = qy[head]; head++;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                    vis[nx][ny] = 1;
                    prex[nx][ny] = x; prey[nx][ny] = y;
                    qx[tail] = nx; qy[tail] = ny; tail++;
                    if (nx == tx && ny == ty) goto found_spawn;
                }
            }
        }
    found_spawn:
        while (!(prex[px][py] == sx && prey[px][py] == sy)) {
            int tpx = prex[px][py], tpy = prey[px][py];
            px = tpx; py = tpy;
        }
        *next_x = px; *next_y = py;
        return 1;
    }
    return 0;
}

// 通用BFS目标判定回调，返回1表示到达目标
typedef int (*BFSGoalFunc)(struct Player *player, int x, int y, void *userdata);
// 通用BFS路径价值回调，返回float，越大优先级越高（可为NULL）
typedef float (*BFSValueFunc)(struct Player *player, int from_x, int from_y, int to_x, int to_y, void *userdata);

// 通用BFS模板
int general_bfs(struct Player *player, int sx, int sy, int *next_x, int *next_y, int max_search,
                BFSGoalFunc is_goal, BFSValueFunc value_func, void *userdata) {
    int vis[MAXN][MAXN] = {0};
    int prex[MAXN][MAXN], prey[MAXN][MAXN];
    int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
    
    qx[tail] = sx; 
    qy[tail] = sy; 
    tail++;
    
    vis[sx][sy] = 1;
    prex[sx][sy] = -1; 
    prey[sx][sy] = -1;
    
    while (head < tail && tail < max_search) {
        int x = qx[head], y = qy[head]; head++;
        
        if (is_goal(player, x, y, userdata)) {
            *next_x = x;
            *next_y = y;
            return 1;
        }
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                vis[nx][ny] = 1;
                prex[nx][ny] = x; 
                prey[nx][ny] = y;
                qx[tail] = nx; 
                qy[tail] = ny; 
                tail++;
            }
        }
    }
    return 0;
}