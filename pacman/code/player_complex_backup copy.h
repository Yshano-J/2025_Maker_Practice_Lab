#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

// 分数常量
#define GHOST_HUNT_BASE_SCORE 200.0f    // 鬼魂追击基础分数
#define NORMAL_STAR_VALUE 10            // 普通星星价值
#define POWER_STAR_VALUE 20             // 强化星星价值

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

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;  // 记录上一步位置
static int step_count = 0;  // 步数计数器

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

// 【新增：分层路径规划相关结构】
#define REGION_SIZE 8  // 区域大小（8x8）
#define MAX_REGIONS 64  // 最大区域数量

struct RegionInfo {
    int center_x, center_y;  // 区域中心点
    int star_count;          // 区域内星星数量
    int power_star_count;    // 区域内超级星数量
    int danger_level;        // 区域危险等级
};

static struct RegionInfo region_map[MAX_REGIONS];
static int region_count = 0;
static int region_map_initialized = 0;

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
    
    // 【新增：分层路径规划初始化】
    region_map_initialized = 0;
    region_count = 0;
    memset(region_map, 0, sizeof(region_map));
    
    // 【新增：A*算法相关初始化】
    memset(nodes, 0, sizeof(nodes));
    memset(open_list, 0, sizeof(open_list));
    open_size = 0;
    
    // 【新增：性能监控变量初始化】
    // 如果有性能监控变量，在这里初始化
    
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
    
    // 【新增：额外的对手状态信息记录】
    // 记录对手的强化状态（用于后续策略调整）
    // opponent_status > 0 表示对手处于强化状态
    // opponent_status == 0 表示对手处于普通状态
    // opponent_status == -1 表示对手已死亡（已在上面检查）
    
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
            total_interference_value += blocking_value * 0.3;  // 权重0.3，避免过度影响主要策略
        }
    }
    
    // 2. 路径阻挡评估
    float path_blocking_value = evaluate_path_blocking(player, our_x, our_y, move_x, move_y);
    total_interference_value += path_blocking_value * 0.2;  // 权重0.2
    
    // 3. 鬼魂引导评估
    float ghost_guidance_value = evaluate_ghost_guidance(player, our_x, our_y, move_x, move_y);
    total_interference_value += ghost_guidance_value * 0.25;  // 权重0.25
    
    return total_interference_value;
}

// 优化的破坏者移动预测函数【增强：智能路径预测、墙壁检测、多步预测】
void predict_ghost_movement(struct Player *player, int ghost_idx, int *pred_x, int *pred_y) {
    // 边界检查：确保鬼魂索引有效
    if (ghost_idx < 0 || ghost_idx >= 2) {
        *pred_x = player->your_posx;  // 默认返回玩家位置
        *pred_y = player->your_posy;
        return;
    }
    
    int gx = player->ghost_posx[ghost_idx];
    int gy = player->ghost_posy[ghost_idx];
    int px = player->your_posx;
    int py = player->your_posy;
    
    // 【优化1：增强的目标导向预测，加入智能追击逻辑】
    if (player->your_status == 0) {  // 只在非强化状态预测追击行为
        int dist_to_player = abs(gx - px) + abs(gy - py);
        
        // 如果距离合理（2-8步），预测鬼魂会向玩家方向移动
        if (dist_to_player >= 2 && dist_to_player <= 8) {
            // 构建候选移动方向并评估
            struct {
                int dir;
                int next_x, next_y;
                float score;
            } candidates[4];
            int candidate_count = 0;
            
            // 评估鬼魂各个移动方向
            for (int d = 0; d < 4; d++) {
                int next_x = gx + dx[d];
                int next_y = gy + dy[d];
                
                if (is_valid(player, next_x, next_y)) {
                    candidates[candidate_count].dir = d;
                    candidates[candidate_count].next_x = next_x;
                    candidates[candidate_count].next_y = next_y;
                    
                    // 计算移动后到玩家的距离
                    int new_dist = abs(next_x - px) + abs(next_y - py);
                    float score = 1000.0f / (new_dist + 1);  // 距离越近分数越高
                    
                    // 【新增：多步路径畅通性检查】
                    // 检查该方向接下来几步是否有墙壁阻挡
                    int wall_penalty = 0;
                    int continuous_path = 0;
                    for (int step = 1; step <= 4; step++) {
                        int future_x = next_x + dx[d] * step;
                        int future_y = next_y + dy[d] * step;
                        
                        if (future_x < 0 || future_x >= player->row_cnt || 
                            future_y < 0 || future_y >= player->col_cnt ||
                            player->mat[future_x][future_y] == '#') {
                            wall_penalty += (5 - step) * 15;  // 越近的墙壁惩罚越重
                            break;
                        }
                        continuous_path++;
                    }
                    
                    // 奖励畅通路径
                    score += continuous_path * 10;
                    
                    // 【新增：方向稳定性奖励】
                    // 如果这个方向与历史移动方向一致，给予奖励
                    if (ghost_last_x[ghost_idx] != -1 && ghost_last_y[ghost_idx] != -1) {
                        int hist_dx = gx - ghost_last_x[ghost_idx];
                        int hist_dy = gy - ghost_last_y[ghost_idx];
                        
                        if (dx[d] == hist_dx && dy[d] == hist_dy) {
                            score += 80.0f;  // 方向一致性奖励增加
                        }
                        
                        // 【新增：方向变化合理性检查】
                        // 如果鬼魂需要改变方向，优先选择90度转向而不是180度掉头
                        if (dx[d] != hist_dx || dy[d] != hist_dy) {
                            // 检查是否是掉头（180度转向）
                            if (dx[d] == -hist_dx && dy[d] == -hist_dy) {
                                score -= 40.0f;  // 惩罚掉头
                            } else {
                                score += 20.0f;  // 奖励90度转向
                            }
                        }
                    }
                    
                    // 【新增：基于玩家移动趋势的智能预测】
                    // 如果玩家正在移动，预测玩家下一步位置并相应调整鬼魂目标
                    if (last_x != -1 && last_y != -1) {
                        int player_dx = px - last_x;
                        int player_dy = py - last_y;
                        
                        // 预测玩家下一步位置
                        int pred_player_x = px + player_dx;
                        int pred_player_y = py + player_dy;
                        
                        // 如果预测位置有效，调整鬼魂目标
                        if (is_valid(player, pred_player_x, pred_player_y)) {
                            int pred_dist = abs(next_x - pred_player_x) + abs(next_y - pred_player_y);
                            score += 500.0f / (pred_dist + 1);  // 朝向玩家预测位置的额外奖励
                        }
                    }
                    
                    candidates[candidate_count].score = score - wall_penalty;
                    candidate_count++;
                }
            }
            
            // 找到最佳候选
            if (candidate_count > 0) {
                int best_idx = 0;
                for (int i = 1; i < candidate_count; i++) {
                    if (candidates[i].score > candidates[best_idx].score) {
                        best_idx = i;
                    }
                }
                
                *pred_x = candidates[best_idx].next_x;
                *pred_y = candidates[best_idx].next_y;
                return;
            }
        }
    } else {
        // 【新增：强化状态下的逃避行为预测】
        // 强化状态下，鬼魂会尝试远离玩家
        int escape_candidates[4];
        int escape_count = 0;
        
        for (int d = 0; d < 4; d++) {
            int next_x = gx + dx[d];
            int next_y = gy + dy[d];
            
            if (is_valid(player, next_x, next_y)) {
                int dist_to_player = abs(next_x - px) + abs(next_y - py);
                int current_dist = abs(gx - px) + abs(gy - py);
                
                // 如果这个方向能远离玩家，加入候选
                if (dist_to_player >= current_dist) {
                    escape_candidates[escape_count++] = d;
                }
            }
        }
        
        // 如果有逃避方向，选择最佳逃避方向
        if (escape_count > 0) {
            int best_escape = -1;
            int max_escape_dist = -1;
            
            for (int i = 0; i < escape_count; i++) {
                int d = escape_candidates[i];
                int next_x = gx + dx[d];
                int next_y = gy + dy[d];
                int dist = abs(next_x - px) + abs(next_y - py);
                
                if (dist > max_escape_dist) {
                    max_escape_dist = dist;
                    best_escape = d;
                }
            }
            
            if (best_escape != -1) {
                *pred_x = gx + dx[best_escape];
                *pred_y = gy + dy[best_escape];
                return;
            }
        }
    }
    
    // 【优化2：改进的历史位置分析，加入智能转向预测】
    if (ghost_last_x[ghost_idx] != -1 && ghost_last_y[ghost_idx] != -1) {
        int move_dx = gx - ghost_last_x[ghost_idx];
        int move_dy = gy - ghost_last_y[ghost_idx];
        
        // 预测继续当前方向移动
        int next_x = gx + move_dx;
        int next_y = gy + move_dy;
        
        // 【新增：增强的有效性检查】
        if (is_valid(player, next_x, next_y)) {
            // 检查前方是否有墙壁阻挡未来移动
            int future_blocked = 0;
            int path_quality = 0;
            
            for (int step = 1; step <= 3; step++) {
                int future_x = next_x + move_dx * step;
                int future_y = next_y + move_dy * step;
                
                if (future_x < 0 || future_x >= player->row_cnt || 
                    future_y < 0 || future_y >= player->col_cnt ||
                    player->mat[future_x][future_y] == '#') {
                    future_blocked = 1;
                    break;
                }
                path_quality++;
            }
            
            // 如果前方路径质量好，使用这个预测
            if (!future_blocked && path_quality >= 2) {
                *pred_x = next_x;
                *pred_y = next_y;
                return;
            }
        }
        
        // 【新增：当前方向被墙壁阻挡时，智能转向预测】
        // 选择最接近当前移动趋势的可行方向
        struct {
            int dir;
            float score;
        } turn_candidates[4];
        int turn_count = 0;
        
        for (int d = 0; d < 4; d++) {
            int alt_x = gx + dx[d];
            int alt_y = gy + dy[d];
            
            if (is_valid(player, alt_x, alt_y)) {
                float score = 100.0f;
                
                // 避免掉头
                if (dx[d] == -move_dx && dy[d] == -move_dy) {
                    score -= 100.0f;  // 大幅惩罚掉头
                } else {
                    // 计算方向差异（曼哈顿距离）
                    int dir_diff = abs(dx[d] - move_dx) + abs(dy[d] - move_dy);
                    score += (4 - dir_diff) * 20;  // 方向差异越小分数越高
                }
                
                // 检查该方向的路径质量
                int path_steps = 0;
                for (int step = 1; step <= 3; step++) {
                    int check_x = alt_x + dx[d] * step;
                    int check_y = alt_y + dy[d] * step;
                    
                    if (is_valid(player, check_x, check_y)) {
                        path_steps++;
                    } else {
                        break;
                    }
                }
                score += path_steps * 15;  // 路径质量奖励
                
                turn_candidates[turn_count].dir = d;
                turn_candidates[turn_count].score = score;
                turn_count++;
            }
        }
        
        // 选择最佳转向
        if (turn_count > 0) {
            int best_turn = 0;
            for (int i = 1; i < turn_count; i++) {
                if (turn_candidates[i].score > turn_candidates[best_turn].score) {
                    best_turn = i;
                }
            }
            
            int best_dir = turn_candidates[best_turn].dir;
            *pred_x = gx + dx[best_dir];
            *pred_y = gy + dy[best_dir];
            return;
        }
    }
    
    // 【优化3：增强的环境感知预测】
    // 评估鬼魂可能的移动方向
    int movement_scores[4] = {0};
    int valid_moves = 0;
    
    for (int d = 0; d < 4; d++) {
        int next_x = gx + dx[d];
        int next_y = gy + dy[d];
        
        if (is_valid(player, next_x, next_y)) {
            valid_moves++;
            int score = 100;  // 基础分数
            
            // 【新增：基于玩家位置的方向偏好】
            if (player->your_status == 0) {
                // 非强化状态，鬼魂倾向于追击玩家
                int dist_to_player = abs(next_x - px) + abs(next_y - py);
                score += (1000 / (dist_to_player + 1));
            } else {
                // 强化状态，鬼魂倾向于远离玩家
                int dist_to_player = abs(next_x - px) + abs(next_y - py);
                score += dist_to_player * 25;
            }
            
            // 【新增：避免死胡同的倾向】
            int exit_routes = 0;
            for (int dd = 0; dd < 4; dd++) {
                int check_x = next_x + dx[dd];
                int check_y = next_y + dy[dd];
                if (is_valid(player, check_x, check_y)) {
                    exit_routes++;
                }
            }
            score += exit_routes * 20;  // 出路越多越好
            
            // 【新增：地形偏好 - 优先开阔区域】
            if (exit_routes >= 3) {
                score += 50;  // 开阔区域奖励
            }
            
            movement_scores[d] = score;
        }
    }
    
    // 选择最佳移动方向
    if (valid_moves > 0) {
        int best_dir = -1;
        int max_score = -1;
        
        for (int d = 0; d < 4; d++) {
            if (movement_scores[d] > max_score) {
                max_score = movement_scores[d];
                best_dir = d;
            }
        }
        
        if (best_dir != -1) {
            *pred_x = gx + dx[best_dir];
            *pred_y = gy + dy[best_dir];
            return;
        }
    }
    
    // 最终fallback：返回当前位置
    *pred_x = gx;
    *pred_y = gy;
}

// 改进的安全移动方向选择（适用于所有地图大小）
int choose_safe_direction(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_dir = -1;
    int max_safety = -1000;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        int safety_score = 0;
        
        // 计算到鬼魂的最小距离（权重根据强化状态调整）
        int min_ghost_dist = distance_to_ghost(player, nx, ny);
        
        // 强化状态下的安全评估
        if (player->your_status > 0) {
            if (player->your_status > 8) {
                safety_score += min_ghost_dist * 2;  // 时间充足，权重较低
            } else if (player->your_status > 3) {
                safety_score += min_ghost_dist * 6;  // 时间不多，需要谨慎
            } else {
                safety_score += min_ghost_dist * 10;  // 即将结束，必须极度谨慎
            }
        } else {
            safety_score += min_ghost_dist * 8;  // 非强化状态，高权重
        }
        
        // 预测鬼魂位置的安全性
        int predicted_safety = 1000;  // 默认安全
        for (int i = 0; i < 2; i++) {
            int pred_x, pred_y;
            predict_ghost_movement(player, i, &pred_x, &pred_y);
            int pred_dist = abs(nx - pred_x) + abs(ny - pred_y);
            if (pred_dist < predicted_safety) {
                predicted_safety = pred_dist;
            }
        }
        safety_score += predicted_safety * 3;  // 预测安全性奖励
        
        // 避免回头（除非极度危险）
        if (nx == last_x && ny == last_y) {
            int current_danger = assess_danger_level(player, x, y);
            if (current_danger >= 2) {
                safety_score -= 5;  // 危险时减少回头惩罚
            } else {
                safety_score -= 15;  // 安全时增加回头惩罚
            }
        }
        
        // 检查是否移动到死胡同，但要考虑死胡同中的超级星
        int exit_routes = 0;
        for (int dd = 0; dd < 4; dd++) {
            int check_x = nx + dx[dd];
            int check_y = ny + dy[dd];
            if (is_valid(player, check_x, check_y)) {
                exit_routes++;
            }
        }
        
        if (exit_routes <= 1) {
            int dead_end_penalty = 20;  // 默认死胡同惩罚
            
            // 检查死胡同方向是否有超级星且有反杀价值
            if (player->your_status <= 0) {  // 非强化状态才考虑超级星
                // 检查这个方向延伸的几步内是否有超级星
                for (int depth = 1; depth <= 4; depth++) {
                    int search_x = nx + dx[d] * (depth - 1);
                    int search_y = ny + dy[d] * (depth - 1);
                    
                    if (!is_valid(player, search_x, search_y)) break;
                    
                    if (player->mat[search_x][search_y] == 'O') {  // 发现超级星
                        float counter_value = evaluate_power_star_counter_attack_value(player, x, y, search_x, search_y);
                        
                        if (counter_value >= 300.0) {
                            dead_end_penalty = -30;  // 高价值反杀：死胡同变成奖励
                        } else if (counter_value >= 150.0) {
                            dead_end_penalty = -10;  // 中等价值反杀：减少惩罚
                        } else if (counter_value > 0) {
                            dead_end_penalty = 5;   // 低价值反杀：小幅减少惩罚
                        }
                        break;  // 找到超级星就停止搜索
                    }
                }
            }
            
            safety_score -= dead_end_penalty;
        }
        
        // 寻找最近的安全星星方向奖励
        int closest_safe_star_dist = 1000;
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
                if (is_star(player, i, j)) {    
                    int star_danger = assess_danger_level(player, i, j);
                    if (star_danger < 2) {  // 只考虑相对安全的星星
                        int star_dist_current = abs(nx - i) + abs(ny - j);
                        int star_dist_original = abs(x - i) + abs(y - j);
                        if (star_dist_current < star_dist_original && star_dist_current < closest_safe_star_dist) {
                            closest_safe_star_dist = star_dist_current;
                            safety_score += 8;  // 靠近安全星星的方向
                        }
                    }
                }
            }
        }
        
        // 边界安全性：靠近墙壁通常更安全
        int wall_nearby = 0;
        for (int dd = 0; dd < 4; dd++) {
            int check_x = nx + dx[dd];
            int check_y = ny + dy[dd];
            if (check_x < 0 || check_x >= player->row_cnt || 
                check_y < 0 || check_y >= player->col_cnt || 
                player->mat[check_x][check_y] == '#') {
                wall_nearby++;
            }
        }
        safety_score += wall_nearby * 2;  // 墙壁附近相对安全
        
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
    if (player->your_status <= 0) return -1000.0;  // 没有强化状态不能追击
    
    int ghost_x = player->ghost_posx[ghost_idx];
    int ghost_y = player->ghost_posy[ghost_idx];
    int distance = abs(sx - ghost_x) + abs(sy - ghost_y);
    
    float score = GHOST_HUNT_BASE_SCORE;  // 基础价值200分
    
    // 【强化：大幅提升鬼魂追击基础分数，确保优先级高于超级星】
    if (player->your_status > POWER_TIME_ABUNDANT) {
        score += 400.0;  // 【强化：从150.0大幅提升到400.0】时间充足时巨幅提升基础价值
    } else if (player->your_status > POWER_TIME_MODERATE) {
        score += 350.0;   // 【强化：从80.0大幅提升到350.0】时间适中时大幅提升
    } else {
        score += 200.0;   // 【强化：从40.0大幅提升到200.0】即使时间不多也要高优先级追击
    }
    
    // 距离因子：距离越近价值越高，强化状态下更激进
    float distance_factor = 1.0 / (distance + 1);
    score *= distance_factor * 5.0;  // 【优化：从4.0提升到5.0】更激进
    
    // 【优化：更宽松的强化时间管理】
    if (player->your_status <= distance) {
        score -= 10.0;  // 【优化：从-20.0进一步减少到-10.0】更宽松
    } else if (player->your_status <= distance + 1) {
        score -= 5.0;   // 【优化：从-10.0进一步减少到-5.0】更宽松
    }
    
    // 【强化：强化时间激进策略，确保鬼魂追击绝对优先】
    if (player->your_status <= 3) {
        score += 100.0;  // 【优化：从-10.0改为+50.0】时间不足时也要积极追击
        
        // 【新增：最后时刻的拼搏奖励】
        if (player->your_status <= 2) {
            score += 80.0;  // 最后2回合，拼死追击
        }
        if (player->your_status <= 1) {
            score += 100.0; // 最后1回合，全力追击
        }
    } else if (player->your_status <= 5) {
        score += 400.0;  // 【强化：从10.0大幅提升到400.0】时间不多时鬼魂追击高优先级
    } else if (player->your_status <= 8) {
        score += 600.0;  // 【强化：从30.0大幅提升到600.0】时间适中时鬼魂追击高优先级
    } else {
        // 时间充足时大幅奖励激进追击
        score += 800.0;  // 【强化：从80.0提升到100.0】时间充足时也要保持鬼魂追击优先
    }
    
    // 【新增：路径上的星星收益评估】
    // 简化路径评估：检查从当前位置到鬼魂位置的直线路径上的星星
    int path_star_bonus = 0;
    int steps_to_ghost = abs(ghost_x - sx) + abs(ghost_y - sy);
    
    if (steps_to_ghost <= 8) {  // 只对较近的鬼魂评估路径收益
        // 简化的路径星星计算
        int path_dx = (ghost_x > sx) ? 1 : ((ghost_x < sx) ? -1 : 0);
        int path_dy = (ghost_y > sy) ? 1 : ((ghost_y < sy) ? -1 : 0);
        
        int check_x = sx;
        int check_y = sy;
        
        for (int step = 0; step < steps_to_ghost && step < 8; step++) {
            if (is_valid(player, check_x, check_y)) {
                char cell = player->mat[check_x][check_y];
                if (cell == 'o') {
                    path_star_bonus += 15;  // 路径上的普通星星
                } else if (cell == 'O') {
                    path_star_bonus += 50;  // 路径上的超级星星
                }
            }
            
            // 移动到下一个位置
            if (abs(check_x - ghost_x) > 0) check_x += path_dx;
            if (abs(check_y - ghost_y) > 0) check_y += path_dy;
        }
    }
    
    score += path_star_bonus;
    
    // 【优化：出生地伏击策略整合】
    // 如果鬼魂距离出生地较远，且我们有足够时间，考虑出生地伏击
    if (ghost_spawn_x[ghost_idx] != -1 && ghost_spawn_y[ghost_idx] != -1) {
        int ghost_to_spawn_dist = abs(ghost_x - ghost_spawn_x[ghost_idx]) + abs(ghost_y - ghost_spawn_y[ghost_idx]);
        int our_to_spawn_dist = abs(sx - ghost_spawn_x[ghost_idx]) + abs(sy - ghost_spawn_y[ghost_idx]);
        
        // 如果鬼魂距离出生地较远，且我们更接近出生地，考虑伏击
        if (ghost_to_spawn_dist >= 6 && our_to_spawn_dist < ghost_to_spawn_dist - 2 && player->your_status > 8) {
            score += 60.0;  // 出生地伏击奖励
        }
    }
    
    // 检查附近的强化星星，如果鬼魂距离强化星星很近，降低追击价值
    for (int k = 0; k < power_star_cache_size; k++) {
        int power_star_dist_from_ghost = abs(ghost_x - power_star_cache[k].x) + abs(ghost_y - power_star_cache[k].y);
        
        // 如果鬼魂距离强化星星很近，追击风险增加
        if (power_star_dist_from_ghost <= 3) {
            score -= 15.0;  // 【优化：从-25.0减少到-15.0】鬼魂靠近强化星星，对手可能获得强化
        }
    }
    
    // 预测鬼魂移动，调整追击难度
    int pred_x, pred_y;
    predict_ghost_movement(player, ghost_idx, &pred_x, &pred_y);
    int pred_distance = abs(sx - pred_x) + abs(sy - pred_y);
    
    // 如果鬼魂在远离，降低追击价值
    if (pred_distance > distance) {
        score -= 20.0;  // 【优化：从-30.0减少到-20.0】鬼魂在逃跑
    } else if (pred_distance < distance) {
        score += 30.0;  // 【优化：从20.0提升到30.0】鬼魂在靠近
    }
    
    // 地形优势：角落和死胡同中的鬼魂更容易追到
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int check_x = ghost_x + dx[d];
        int check_y = ghost_y + dy[d];
        if (is_valid(player, check_x, check_y)) {
            escape_routes++;
        }
    }
    
    if (escape_routes <= 2) {
        score += 60.0;  // 【优化：从40.0提升到60.0】鬼魂在死胡同或角落
    } else if (escape_routes == 3) {
        score += 30.0;  // 【优化：从20.0提升到30.0】鬼魂选择有限
    }
    
    // 双鬼距离：如果两个鬼魂很近，追击风险增加
    if (ghost_idx == 0) {
        int other_ghost_dist = abs(ghost_x - player->ghost_posx[1]) + abs(ghost_y - player->ghost_posy[1]);
        if (other_ghost_dist <= 3) {
            score -= 15.0;  // 【优化：从-25.0减少到-15.0】双鬼聚集，追击风险大
        }
    } else {
        int other_ghost_dist = abs(ghost_x - player->ghost_posx[0]) + abs(ghost_y - player->ghost_posy[0]);
        if (other_ghost_dist <= 3) {
            score -= 15.0;  // 【优化：从-25.0减少到-15.0】
        }
    }
    
    // 【新增：分数导向的追击奖励 - 动态调整】
    int target_score = 8000;  // 目标分数
    int low_threshold = target_score / 3;    // 2666分
    int mid_threshold = target_score / 2;    // 4000分
    int high_threshold = target_score * 2/3; // 5333分
    
    if (player->your_score < low_threshold) {
        score += 100.0;  // 分数严重不足时，追击价值极大提升
    } else if (player->your_score < mid_threshold) {
        score += 80.0;   // 分数不足时，追击价值大幅提升
    } else if (player->your_score < high_threshold) {
        score += 60.0;   // 分数接近目标时，适度提升
    } else if (player->your_score < target_score) {
        score += 40.0;   // 接近目标时，轻微提升
    }
    
    return score;
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
int bfs_ghost_hunt(struct Player *player, int sx, int sy, int target_ghost, int *next_x, int *next_y) {
    if (target_ghost < 0 || target_ghost >= 2) return 0;
    
    int ghost_x = player->ghost_posx[target_ghost];
    int ghost_y = player->ghost_posy[target_ghost];
    
    int vis[MAXN][MAXN] = {0};
    int prex[MAXN][MAXN], prey[MAXN][MAXN];
    int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
    
    // 【新增：路径价值评估数组】
    float path_value[MAXN][MAXN];
    for (int i = 0; i < MAXN; i++) {
        for (int j = 0; j < MAXN; j++) {
            path_value[i][j] = 0.0f;
        }
    }
    
    qx[tail] = sx; 
    qy[tail] = sy; 
    tail++;
    
    vis[sx][sy] = 1;
    prex[sx][sy] = -1; 
    prey[sx][sy] = -1;
    path_value[sx][sy] = 0.0f;
    
    // 追击模式：搜索深度适当限制，但比普通BFS更激进
    int max_search = 250;  // 【优化：从200提升到250】
    int searched = 0;
    
    while (head < tail && searched < max_search) {
        int x = qx[head], y = qy[head]; 
        head++;
        searched++;
        
        // 到达鬼魂位置
        if (x == ghost_x && y == ghost_y) {
            // 【优化：回溯时选择价值最高的路径】
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
        
        // 预测鬼魂可能移动到的位置也算有效目标
        int pred_x, pred_y;
        predict_ghost_movement(player, target_ghost, &pred_x, &pred_y);
        if (x == pred_x && y == pred_y) {
            // 到达预测位置也算成功
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
        
        // 【优化：增强的方向排序，考虑路径价值】
        typedef struct {
            int dir;
            float priority;
        } DirectionPriority;
        
        DirectionPriority directions[4];
        
        int valid_directions = 0;
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                directions[valid_directions].dir = d;
                
                // 计算优先级：距离鬼魂越近 + 路径星星价值
                int dist_to_ghost = abs(nx - ghost_x) + abs(ny - ghost_y);
                float priority = 1000.0f / (dist_to_ghost + 1);
                
                // 【新增：路径星星收益评估】
                char cell = player->mat[nx][ny];
                if (cell == 'o') {
                    priority += 50.0f;  // 普通星星收益
                } else if (cell == 'O') {
                    priority += 120.0f; // 超级星星收益
                }
                
                // 【新增：考虑强化时间的路径价值】
                if (player->your_status > 0) {
                    // 强化时间越少，越不应该绕路收集星星
                    if (player->your_status <= 3) {
                        if (cell == 'o') priority += 20.0f;  // 时间紧迫时星星价值降低
                        if (cell == 'O') priority += 40.0f;  // 但超级星仍有价值
                    } else if (player->your_status <= 6) {
                        if (cell == 'o') priority += 35.0f;  // 中等时间时适度奖励
                        if (cell == 'O') priority += 80.0f;  // 超级星高价值
                    } else {
                        if (cell == 'o') priority += 50.0f;  // 时间充足时充分奖励
                        if (cell == 'O') priority += 120.0f; // 超级星极高价值
                    }
                }
                
                // 【新增：路径连续性奖励】
                if (path_value[x][y] > 0) {
                    priority += path_value[x][y] * 0.3f;  // 继承前面路径的价值
                }
                
                directions[valid_directions].priority = priority;
                valid_directions++;
            }
        }
        
        // 按优先级排序
        for (int i = 0; i < valid_directions; i++) {
            for (int j = i + 1; j < valid_directions; j++) {
                if (directions[j].priority > directions[i].priority) {
                    DirectionPriority temp = directions[i];
                    directions[i] = directions[j];
                    directions[j] = temp;
                }
            }
        }
        
        // 按优先级顺序添加到队列
        for (int i = 0; i < valid_directions; i++) {
            int d = directions[i].dir;
            int nx = x + dx[d], ny = y + dy[d];
            
            vis[nx][ny] = 1;
            prex[nx][ny] = x; 
            prey[nx][ny] = y;
            path_value[nx][ny] = directions[i].priority;  // 记录路径价值
            qx[tail] = nx; 
            qy[tail] = ny; 
            tail++;
        }
    }
    
    return 0;  // 追击失败
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
    int predicted_min_dist = INF;
    
    // 考虑鬼魂预测位置
    for (int i = 0; i < 2; i++) {
        int pred_x, pred_y;
        predict_ghost_movement(player, i, &pred_x, &pred_y);
        int pred_dist = abs(x - pred_x) + abs(y - pred_y);
        if (pred_dist < predicted_min_dist) {
            predicted_min_dist = pred_dist;
        }
    }
    
    // 【新增：根据地图大小动态调整安全距离】
    int map_size = player->row_cnt * player->col_cnt;
    int safe_distance, danger_distance_immediate, danger_distance_close;
    
    if (map_size <= SMALL_MAP_THRESHOLD) {
        // 小地图：距离要求更宽松
        safe_distance = 1;
        danger_distance_immediate = 1;
        danger_distance_close = 1;
    } else if (map_size >= LARGE_MAP_THRESHOLD) {
        // 大地图：距离要求更严格
        safe_distance = 3;
        danger_distance_immediate = 2;
        danger_distance_close = 3;
    } else {
        // 中等地图：标准距离要求
        safe_distance = 2;
        danger_distance_immediate = 1;
        danger_distance_close = 2;
    }
    
    // 评估逃跑路径数量（基于策略文档的地形因素）
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int escape_x = x + dx[d];
        int escape_y = y + dy[d];
        if (is_valid(player, escape_x, escape_y)) {
            // 检查这个逃跑方向是否远离鬼魂
            int escape_ghost_dist = distance_to_ghost(player, escape_x, escape_y);
            if (escape_ghost_dist >= min_ghost_dist) {  // 不会更接近鬼魂
                escape_routes++;
            }
        }
    }
    
    // 综合当前和预测距离评估危险等级
    int overall_dist = (min_ghost_dist + predicted_min_dist) / 2;
    
    // 地形修正：逃跑路径越少，危险等级越高
    int terrain_penalty = 0;
    if (escape_routes <= 1) {
        terrain_penalty = 1;  // 死胡同或几乎无路可逃
    } else if (escape_routes == 2) {
        terrain_penalty = 0;  // 走廊，中等风险
    }
    // escape_routes >= 3 时，terrain_penalty = 0，开阔地带较安全
    
    // 【优化：地图大小自适应的强化状态评估】
    if (player->your_status > 0) {
        // 强化状态时间充足，基本安全
        if (player->your_status > POWER_TIME_ABUNDANT) {
            return 0;  
        }
        // 强化状态时间不多，需要谨慎
        else if (player->your_status > POWER_TIME_MODERATE) {
            if (overall_dist <= danger_distance_immediate) {
                return 1;  // 中等危险，不能太激进
            } else {
                return 0;  // 相对安全
            }
        }
        // 强化状态即将结束，很危险
        else {
            if (overall_dist <= danger_distance_immediate) {
                return 2;  // 高危险，必须立即远离
            } else if (overall_dist <= danger_distance_close) {
                return 1;  // 中等危险
            } else {
                return 0;  // 安全
            }
        }
    } else {
        // 【优化：地图大小自适应的非强化状态危险评估】
        int base_danger = 0;
        if (overall_dist <= danger_distance_immediate) {
            base_danger = 2;  // 高危险
        } else if (overall_dist <= danger_distance_close) {
            base_danger = 1;  // 中等危险
        } else if (overall_dist <= safe_distance) {
            // 【新增：在安全距离边缘时，地形因素影响更大】
            base_danger = (escape_routes <= 1) ? 1 : 0;
        } else {
            base_danger = 0;  // 安全
        }
        
        // 应用地形修正：死胡同时提升危险等级
        int final_danger = base_danger + terrain_penalty;
        
        // 【新增：小地图特殊处理 - 更加宽松的安全评估】
        if (map_size <= SMALL_MAP_THRESHOLD) {
            // 小地图中，只有在非常接近时才认为高危险
            if (overall_dist <= 1 && escape_routes <= 1) {
                final_danger = 2;  // 高危险
            } else if (overall_dist <= 1) {
                final_danger = 1;  // 中等危险
            } else {
                final_danger = 0;  // 安全
            }
        }
        
        return (final_danger > 2) ? 2 : final_danger;  // 最高危险等级为2
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
    
    return manhattan + wall_count / 2;  // 大地图减少墙体惩罚
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
float evaluate_star(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int star_value = (star_type == 'O') ? 20 : 10;
    
    int distance = heuristic(player, sx, sy, star_x, star_y);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    int map_size = player->row_cnt * player->col_cnt;
    
    // 小地图专用优化策略 - 稳定性优化版
    if (map_size <= SMALL_MAP_THRESHOLD) {
        float score = star_value;
        
        // 距离因子：距离越近分数越高
        float distance_factor = 1.0 / (distance + 1);
        score *= distance_factor * 10.0;
        
        // 目标稳定性奖励：如果是当前目标，给予额外奖励
        if (star_x == current_target_x && star_y == current_target_y) {
            score += 30.0;  // 稳定目标奖励
        }
        
            //【调整：根据强化状态调整超级星优先级】非强化状态下超级星价值高，强化状态下降低
            if (star_type == 'O') {
                if (player->your_status > 0) {
                    score += 150.0;  // 【调整：强化状态下基础价值降低到150.0】优先追击鬼魂
                    
                    // 【优化：强化状态下超级星价值动态调整】
                    if (player->your_status <= 5) {
                        score += 100.0;  // 【调整：从350.0大幅降低到80.0】即将结束时降低超级星优先级
                        
                        // 【新增：时间紧迫时的额外奖励】
                        if (player->your_status <= 3) {
                            score += 250.0;  // 【优化：从150.0提升到250.0】最后3回合，超级星价值暴涨
                        }
                        
                        // 【新增：强化状态下超级星的续航价值】
                        if (player->your_status <= 2) {
                            score += 350.0;  // 最后2回合，超级星是唯一续航手段
                        }
                    } else if (player->your_status <= 10) {
                        score += 60.0;  // 【调整：从150.0降低到60.0】中等时间时适度降低
                    } else {
                        score += 40.0;   // 【调整：从80.0降低到40.0】时间充足时轻微提升
                    }
                } else {
                    // 【非强化状态下的超级星处理】
                    score += 750.0;  // 【保持：非强化状态下基础价值450.0】获得强化状态是获胜关键
                    score += 500.0;  // 【优化：从180.0提升到500.0】额外奖励，确保获得强化
                    
                    // 非强化状态：评估反杀价值
                    float counter_attack_value = evaluate_power_star_counter_attack_value(player, sx, sy, star_x, star_y);
                    
                    if (counter_attack_value > 0) {
                        score += counter_attack_value;  // 加上反杀价值
                        int current_danger = assess_danger_level(player, star_x, star_y);
                        
                        // 【优化：反杀价值很高时，极大提升优先级】
                        if (counter_attack_value >= 300.0) {
                            score += 400.0;  // 【优化：从300.0提升到400.0】更激进
                            
                            // 【优化：高价值反杀时，大幅降低危险惩罚】
                            if (current_danger >= 2) {
                                score += 300.0;  // 【优化：从200.0提升到300.0】极大补偿高危险惩罚
                            } else if (current_danger == 1) {
                                score += 150.0;  // 【优化：从100.0提升到150.0】
                            }
                        } else if (counter_attack_value >= 150.0) {
                            score += 250.0;  // 【优化：从180.0提升到250.0】
                            
                            // 【优化：中等价值反杀时，显著降低危险惩罚】
                            if (current_danger >= 2) {
                                score += 180.0;  // 【优化：从120.0提升到180.0】
                            } else if (current_danger == 1) {
                                score += 90.0;   // 【优化：从60.0提升到90.0】中等危险补偿
                            }
                        } else if (counter_attack_value >= 50.0) {
                            score += 120.0;   // 【优化：从80.0提升到120.0】低价值反杀也给予奖励
                            
                            // 【新增：低价值反杀时，轻微降低危险惩罚】
                            if (current_danger >= 2) {
                                score += 80.0;  // 【优化：从50.0提升到80.0】高危险时给予补偿
                            }
                        }
                    }
                    
                    // 【新增：战略性超级星收集奖励 - 动态调整】
                    // 当我们分数不足时，超级星价值倍增
                    int target_score = 10000;  // 目标分数
                    int low_threshold = target_score / 3;    // 3333分
                    int mid_threshold = target_score / 2;    // 5000分
                    int high_threshold = target_score * 2/3; // 6667分
                    
                    if (player->your_score < low_threshold) {
                        score += 400.0;  // 分数严重不足时，超级星价值极大提升
                    } else if (player->your_score < mid_threshold) {
                        score += 300.0;  // 分数不足时，超级星价值大幅提升
                    } else if (player->your_score < high_threshold) {
                        score += 200.0;  // 分数接近目标时，适度提升
                    } else if (player->your_score < target_score) {
                        score += 100.0;  // 接近目标时，轻微提升
                    }
                
                // 【优化：死胡同超级星特殊处理升级】
                int escape_routes = 0;
                for (int d = 0; d < 4; d++) {
                    int check_x = star_x + dx[d];
                    int check_y = star_y + dy[d];
                    if (is_valid(player, check_x, check_y)) {
                        escape_routes++;
                    }
                }
                
                if (escape_routes <= 2 && distance <= 4) {
                    score += 150.0;  // 【优化：从80.0提升到150.0】死胡同近距离超级星，巨大机会奖励
                } else if (escape_routes <= 1 && distance <= 6) {
                    score += 200.0;  // 【优化：从120.0提升到200.0】真正的死胡同，更大奖励
                } else if (escape_routes <= 2 && distance <= 8) {
                    score += 100.0;  // 【新增：中等距离死胡同奖励】
                }
                
                // 【新增：分数不足时的激进策略 - 动态调整】
                int target_score = 10000;  // 目标分数
                int very_low_threshold = target_score / 4;    // 2500分
                int low_threshold = target_score / 2;         // 5000分
                int high_threshold = target_score * 3/4;      // 7500分
                
                if (player->your_score < very_low_threshold) {
                    score += 300.0;  // 分数极度不足时，超级星价值暴涨
                } else if (player->your_score < low_threshold) {
                    score += 200.0;  // 分数严重不足时，超级星价值暴涨
                } else if (player->your_score < high_threshold) {
                    score += 120.0;   // 分数不足时，适度提升
                } else if (player->your_score < target_score) {
                    score += 60.0;   // 接近目标时，轻微提升
                }
            }
            
            // 【优化：已吃超级星数量的惩罚进一步降低】
            if (power_star_eaten_count >= 2) {
                score -= 15.0;   // 【优化：保持轻微降低】
            }
            // 【新增：前期超级星奖励】
            if (power_star_eaten_count == 0) {
                score += 80.0;  // 第一个超级星额外奖励
            }
        }
        
        // 动态风险评估（基于鬼魂预测）
        int current_danger = assess_danger_level(player, star_x, star_y);
        
        // 改进的强化状态处理（超级星优先）
        if (player->your_status > 0) {
            // 强化状态时间充足
            if (player->your_status > POWER_TIME_ABUNDANT) {
                score += 8.0;  // 【优化：从5.0提升到8.0】基础奖励提升
                
                // 超级星在时间充足时价值调整
                if (star_type == 'O') {
                    score += 15.0;  // 【优化：从-5.0改为+15.0】时间充足时超级星仍有价值
                }
                
                // 只有安全且非常近的星星值得考虑
                if (current_danger == 0 && distance <= 1) {
                    score += 25.0;  // 【优化：从15.0提升到25.0】安全且伸手可得的星星
                }
            }
            // 强化状态时间适中
            else if (player->your_status > POWER_TIME_MODERATE) {
                score += 25.0;  // 【优化：从15.0提升到25.0】适中奖励
                
                // 安全性仍然重要，但不如即将结束时关键
                if (current_danger >= 2) {
                    score -= 20.0;  // 【优化：从-30.0减少到-20.0】高危险时降分减少
                } else if (current_danger == 1) {
                    score -= 5.0;   // 【优化：从-10.0减少到-5.0】中等危险时轻微降分
                }
                
                // 超级星处理
                if (star_type == 'O') {
                    if (current_danger < 2) {  // 只有相对安全时才考虑
                        score += 25.0;  // 【优化：从10.0提升到25.0】中等时间时超级星有一定价值
                    } else {
                        score -= 5.0;   // 【优化：从-15.0减少到-5.0】危险时避免超级星惩罚减少
                    }
                }
                
                // 近距离星星奖励（但要考虑安全性）
                if (distance <= 1 && current_danger < 2) {
                    score += 30.0;  // 【优化：从20.0提升到30.0】安全的近距离星星
                } else if (distance <= 2 && current_danger == 0) {
                    score += 15.0;  // 【优化：从10.0提升到15.0】安全的较近星星
                }
            }
            // 强化状态即将结束（1-3回合）
            else {
                // 即将结束时仍要考虑超级星的巨大价值
                if (current_danger >= 2) {
                    score -= 25.0;  // 【优化：从-40.0减少到-25.0】高危险时仍给机会
                } else if (current_danger == 1) {
                    score -= 10.0;  // 【优化：从-20.0减少到-10.0】中等危险时轻微降分
                } else {
                    score += 120.0; // 【优化：从80.0提升到120.0】安全时更大幅加分
                }
                
                // 超级星在即将结束时只有安全时才考虑
                if (star_type == 'O') {
                    if (current_danger == 0) {
                        score += 50.0;  // 【优化：从30.0提升到50.0】安全的超级星非常重要
                    } else if (current_danger == 1) {
                        score += 20.0;  // 【新增：中等危险时也给予奖励】
                    } else {
                        score -= 10.0;  // 【优化：从-20.0减少到-10.0】不安全的超级星要避免
                    }
                }
                
                // 只有非常近且安全的星星值得冒险
                if (distance <= 1 && current_danger == 0) {
                    score += 60.0;  // 【优化：从40.0提升到60.0】安全的伸手可得星星
                } else if (distance <= 2 && current_danger == 0) {
                    score += 30.0;  // 【优化：从20.0提升到30.0】安全的很近星星
                } else if (distance > 3) {
                    score -= 15.0;  // 【优化：从-25.0减少到-15.0】远距离星星风险太大
                }
            }
        } else {
            // 非强化状态的风险评估 - 对超级星极度宽松，优先获得强化
            if (star_type == 'O') {
                // 超级星特殊处理：大幅降低风险惩罚
                if (current_danger >= 2) {
                    score -= 5.0;   // 【优化：从-10.0进一步降低到-5.0】超级星值得冒险
                } else if (current_danger == 1) {
                    score += 5.0;   // 【优化：从-2.0改为+5.0】中等危险时也给予奖励
                }
                
                // 如果是安全的超级星，额外奖励
                if (current_danger == 0) {
                    score += 80.0;  // 【优化：从50.0提升到80.0】安全超级星巨额奖励
                }
                
                // 【新增：超级星距离奖励】
                if (distance <= 2) {
                    score += 60.0;  // 近距离超级星额外奖励
                } else if (distance <= 4) {
                    score += 30.0;  // 中距离超级星奖励
                }
            } else {
                // 普通星星的风险评估保持原有逻辑
                if (current_danger >= 2) {
                    score -= 30.0;  // 高危险时降分
                } else if (current_danger == 1) {
                    score -= 10.0;  // 中等危险时轻微降分
                }
            }
        }
        
        // 连续安全移动奖励：如果最近移动很安全，可以稍微冒险
        if (consecutive_safe_moves >= 5) {
            score += 10.0;  // 长期安全后可以适当激进
        }
        
        // 游戏阶段适应：后期星星稀少时更激进
        int total_stars = 0;
        for (int i = 0; i < player->row_cnt; i++) {
            for (int j = 0; j < player->col_cnt; j++) {
                if (is_star(player, i, j)) total_stars++;
            }
        }
        
        if (total_stars <= 10) {
            score += 15.0;  // 后期星星稀少，每个星星都很重要
            if (star_type == 'O') {
                score += 25.0;  // 后期超级星极其重要
            }
        }
        
        // 10000分目标专用分数策略优化
        // 根据当前分数和游戏进度动态调整激进度
        float score_boost = 0.0;
        int current_score = player->your_score;
        
        // 分数不足时更加激进地追求高价值目标
        int target_score = 10000;  // 目标分数
        if (current_score < target_score / 4) {  // 2500分
            // 分数严重不足，超级星价值翻倍
            if (star_type == 'O') {
                score_boost += 150.0;
            } else {
                score_boost += 15.0;  // 普通星星也需要积极收集
            }
        } else if (current_score < target_score / 2) {  // 4000分
            // 分数不足，适度提升价值
            if (star_type == 'O') {
                score_boost += 100.0;
            } else {
                score_boost += 10.0;
            }
        } else if (current_score < target_score * 3/4) {  // 6000分
            // 接近目标，保持激进
            if (star_type == 'O') {
                score_boost += 50.0;
            } else {
                score_boost += 5.0;
            }
        } else if (current_score < target_score) {  // 8000分
            // 最后冲刺阶段
            if (star_type == 'O') {
                score_boost += 25.0;
            }
        }
        
        // 游戏后期（步数较多）时更加激进
        if (step_count > 100) {
            score_boost += 20.0;
            if (star_type == 'O') {
                score_boost += 30.0;  // 后期超级星更重要
            }
        }
        
        score += score_boost;
        
        // 位置安全性奖励
        int edge_bonus = 0;
        if (star_x == 0 || star_x == player->row_cnt-1 || 
            star_y == 0 || star_y == player->col_cnt-1) {
            edge_bonus = 8;
        }
        if ((star_x <= 1 || star_x >= player->row_cnt-2) && 
            (star_y <= 1 || star_y >= player->col_cnt-2)) {
            edge_bonus += 5;  // 角落位置额外奖励
        }
        score += edge_bonus;
        
        // 路径可达性检查：避免选择被鬼魂完全封锁的星星
        int reachable_paths = 0;
        for (int d = 0; d < 4; d++) {
            int check_x = star_x + dx[d];
            int check_y = star_y + dy[d];
            if (is_valid(player, check_x, check_y)) {
                int path_danger = assess_danger_level(player, check_x, check_y);
                if (path_danger < 2) {
                    reachable_paths++;
                }
            }
        }
        
        if (reachable_paths == 0 && player->your_status <= 0) {
            score -= 30.0;  // 完全被鬼魂封锁的星星
        }
        
        return score;
    }
    
    // 大地图逻辑保持不变，但也加入一些稳定性考虑
    float distance_factor = 1.0;
    float danger_threshold = 1;
    float near_threshold = 2;
    float mid_threshold = 5;
    
    if (map_size > LARGE_MAP_THRESHOLD) {  // 大地图
        distance_factor = 1.5;  // 大地图距离影响更大
        danger_threshold = 2;   // 大地图危险阈值放宽
        near_threshold = 4;     // 大地图近距离阈值调整
        mid_threshold = 10;     // 大地图中距离阈值调整
    }
    
    // 目标稳定性奖励（大地图也适用）
    float stability_bonus = 0;
    if (star_x == current_target_x && star_y == current_target_y) {
        stability_bonus = 15.0;  // 大地图稳定目标奖励
    }
    
    // 风险评估
    float danger_penalty = 0;
    if (ghost_dist <= danger_threshold && player->your_status <= 0) {
        danger_penalty = 3.0;
    }
    
    // 强化状态奖励
    float power_bonus = 0;
    if (player->your_status > 0) {
        power_bonus = 5.0;
        if (star_type == 'O') {
            if (player->your_status <= 3) {
                power_bonus += 15.0;
            } else if (player->your_status <= 8) {
                power_bonus += 10.0;
            } else {
                power_bonus += 5.0;
            }
        }
    } else {
        if (star_type == 'O') {
            power_bonus += 8.0;
        }
    }
    
    // 距离奖励（大地图调整）
    float distance_bonus = 0;
    if (distance <= near_threshold) {
        distance_bonus = 5.0;
    } else if (distance <= mid_threshold) {
        distance_bonus = 2.0;
    }
    
    // 大地图特殊处理：超级星在大地图中更重要
    if (map_size > LARGE_MAP_THRESHOLD && star_type == 'O') {
        power_bonus += 5.0;  // 大地图超级星额外奖励
    }
    
    // 地图优化策略：优先外圈星星，采用螺旋式收集
    float region_bonus = 0;
    int current_region = get_region(player, sx, sy);
    int star_region = get_region(player, star_x, star_y);
    int target_region_stars = count_stars_in_region(player, star_region);
    
    // 优先收集外圈星星（避免穿越中心墙体区域）
    if (star_region == 1) {  // 外圈星星
        region_bonus += 8.0;
        
        // 如果当前也在外圈，进一步提升优先级
        if (current_region == 1) {
            region_bonus += 6.0;
        }
    } else {  // 内圈星星
        // 只有外圈星星很少时才考虑内圈
        if (target_region_stars >= 3) {
            region_bonus = -5.0;  // 降低内圈优先级
        } else {
            region_bonus = 2.0;   // 外圈快空了，可以考虑内圈
        }
    }
    
    // 螺旋式路径奖励：优先选择能形成连续路径的星星
    int center_x = player->row_cnt / 2;
    int center_y = player->col_cnt / 2;
    
    // 计算角度，鼓励顺时针移动
    if (star_region == 1) {  // 只对外圈星星应用螺旋策略
        int dx_current = sx - center_x;
        int dy_current = sy - center_y;
        int dx_star = star_x - center_x;
        int dy_star = star_y - center_y;
        
        // 简化的角度判断：鼓励顺时针方向的星星
        if ((dx_current >= 0 && dy_current >= 0 && dx_star >= dy_current && dy_star >= dx_current) ||  // 右下象限顺时针
            (dx_current <= 0 && dy_current >= 0 && dx_star <= dy_current && dy_star >= -dx_current) || // 左下象限顺时针
            (dx_current <= 0 && dy_current <= 0 && dx_star <= -dy_current && dy_star <= dx_current) || // 左上象限顺时针
            (dx_current >= 0 && dy_current <= 0 && dx_star >= -dy_current && dy_star <= -dx_current)) { // 右上象限顺时针
            region_bonus += 4.0;  // 奖励顺时针方向的星星
        }
    }
    
    return (float)star_value * distance_factor / (distance + 1) + power_bonus + distance_bonus + region_bonus + stability_bonus - danger_penalty;
}

// 优化的BFS（小地图和大地图通用）
int bfs_backup(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int vis[MAXN][MAXN] = {0};
    int prex[MAXN][MAXN], prey[MAXN][MAXN];
    int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
    int map_size = player->row_cnt * player->col_cnt;
    
    qx[tail] = sx; 
    qy[tail] = sy; 
    tail++;
    
    vis[sx][sy] = 1;
    prex[sx][sy] = -1; 
    prey[sx][sy] = -1;
    
    // 小地图不限制搜索深度，大地图限制以提升性能
    int max_search = (map_size <= SMALL_MAP_THRESHOLD) ? map_size * 2 : 
                     ((map_size > LARGE_MAP_THRESHOLD) ? MAX_BFS_SEARCHES_LARGE : 500);
    int searched = 0;
    
    while (head < tail && searched < max_search) {
        int x = qx[head], y = qy[head]; head++;
        searched++;
        
        if (is_star(player, x, y)) {
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
    
    int spawn_x = ghost_spawn_x[target_ghost];
    int spawn_y = ghost_spawn_y[target_ghost];
    
    int vis[MAXN][MAXN] = {0};
    int prex[MAXN][MAXN], prey[MAXN][MAXN];
    int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
    
    qx[tail] = sx; 
    qy[tail] = sy; 
    tail++;
    
    vis[sx][sy] = 1;
    prex[sx][sy] = -1; 
    prey[sx][sy] = -1;
    
    // 出生地接近模式：搜索深度适当限制
    int max_search = 150;  // 10x10地图足够了
    int searched = 0;
    
    while (head < tail && searched < max_search) {
        int x = qx[head], y = qy[head]; 
        head++;
        searched++;
        
        // 到达出生地位置
        if (x == spawn_x && y == spawn_y) {
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
        
        // 在出生地附近也算成功（准备等待）
        int spawn_dist = abs(x - spawn_x) + abs(y - spawn_y);
        if (spawn_dist <= 2) {
            // 到达出生地附近，可以开始等待
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
        
        // 优先搜索更接近出生地的方向
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
    
    return 0;  // 出生地接近失败
}

// 区域划分函数：针对圆形地图的径向分区
int get_region(struct Player *player, int x, int y) {
    int center_x = player->row_cnt / 2;
    int center_y = player->col_cnt / 2;
    
    // 计算到中心的距离
    int dist_to_center = abs(x - center_x) + abs(y - center_y);
    
    // 根据距离分为内圈(0)和外圈(1)
    int radius_threshold = (player->row_cnt + player->col_cnt) / 6;
    return (dist_to_center < radius_threshold) ? 0 : 1;
}

// 计算指定区域的星星数量
int count_stars_in_region(struct Player *player, int region) {
    int count = 0;
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (get_region(player, i, j) == region && is_star(player, i, j)) {
                count++;
            }
        }
    }
    return count;
}

// 【新增：分层路径规划 - 区域初始化】
void initialize_region_map(struct Player *player) {
    if (region_map_initialized) return;
    
    int map_size = player->row_cnt * player->col_cnt;
    if (map_size <= LARGE_MAP_THRESHOLD) {
        region_map_initialized = 1;
        return;  // 小地图不需要区域划分
    }
    
    region_count = 0;
    int regions_per_row = (player->row_cnt + REGION_SIZE - 1) / REGION_SIZE;//每一行有多少个区域
    int regions_per_col = (player->col_cnt + REGION_SIZE - 1) / REGION_SIZE;//每一列有多少个区域
    
    for (int i = 0; i < regions_per_row && region_count < MAX_REGIONS; i++) {
        for (int j = 0; j < regions_per_col && region_count < MAX_REGIONS; j++) {
            region_map[region_count].center_x = i * REGION_SIZE + REGION_SIZE / 2;//区域中心x坐标
            region_map[region_count].center_y = j * REGION_SIZE + REGION_SIZE / 2;//区域中心y坐标
            
            // 确保中心点在地图范围内
            if (region_map[region_count].center_x >= player->row_cnt) {
                region_map[region_count].center_x = player->row_cnt - 1;
                //如果区域中心x坐标超出地图范围，则设置为地图最后一行
            }
            if (region_map[region_count].center_y >= player->col_cnt) {
                region_map[region_count].center_y = player->col_cnt - 1;
                //如果区域中心y坐标超出地图范围，则设置为地图最后一列
            }
            
            region_map[region_count].star_count = 0;//区域星星数量
            region_map[region_count].power_star_count = 0;//区域星星数量
            region_map[region_count].danger_level = 0;//区域危险等级
            
            region_count++;
        }
    }
    
    region_map_initialized = 1;
}

// 【新增：获取位置所属区域】
int get_region_index(struct Player *player, int x, int y) {
    if (!region_map_initialized) {
        initialize_region_map(player);
    }
    
    int region_row = x / REGION_SIZE;//区域行号：x
    int region_col = y / REGION_SIZE;//区域列号: y
    int regions_per_col = (player->col_cnt + REGION_SIZE - 1) / REGION_SIZE;//每一列有多少个区域
    
    int region_idx = region_row * regions_per_col + region_col;//区域索引：区域行号*每一列有多少个区域+区域列号
    return (region_idx < region_count) ? region_idx : -1;//如果区域索引小于区域数量，则返回区域索引，否则返回-1
}

// 【新增：更新区域信息】
void update_region_info(struct Player *player) {
    if (!region_map_initialized) return;
    
    // 重置区域信息
    for (int i = 0; i < region_count; i++) {
        region_map[i].star_count = 0;
        region_map[i].power_star_count = 0;
        region_map[i].danger_level = 0;
    }
    
    // 更新星星信息
    for (int i = 0; i < star_cache_size; i++) {
        int region_idx = get_region_index(player, star_cache[i].x, star_cache[i].y);
        if (region_idx >= 0) {
            region_map[region_idx].star_count++;
        }
    }
    
    // 更新强化星星信息
    for (int i = 0; i < power_star_cache_size; i++) {
        int region_idx = get_region_index(player, power_star_cache[i].x, power_star_cache[i].y);
        if (region_idx >= 0) {
            region_map[region_idx].power_star_count++;
        }
    }
    
    // 更新危险等级
    for (int i = 0; i < region_count; i++) {
        int region_x = region_map[i].center_x;
        int region_y = region_map[i].center_y;
        
        // 计算鬼魂到区域中心的距离
        int min_ghost_dist = INF;
        for (int j = 0; j < 2; j++) {
            int ghost_dist = abs(region_x - player->ghost_posx[j]) + abs(region_y - player->ghost_posy[j]);
            if (ghost_dist < min_ghost_dist) {
                min_ghost_dist = ghost_dist;
            }
        }
        
        if (min_ghost_dist <= REGION_SIZE) {
            region_map[i].danger_level = 2;  // 高危险
        } else if (min_ghost_dist <= REGION_SIZE * 2) {
            region_map[i].danger_level = 1;  // 中等危险
        } else {
            region_map[i].danger_level = 0;  // 安全
        }
    }
}

// 【新增：区域级别的路径规划】
int find_best_target_region(struct Player *player, int current_x, int current_y) {
    if (!region_map_initialized) return -1;
    
    update_region_info(player);
    
    int best_region = -1;
    float best_score = -1000.0f;
    
    for (int i = 0; i < region_count; i++) {
        float score = 0.0f;
        
        // 星星价值
        score += region_map[i].star_count * 15.0f;
        score += region_map[i].power_star_count * 200.0f;
        
        // 距离因子
        int dist_to_region = abs(current_x - region_map[i].center_x) + abs(current_y - region_map[i].center_y);
        score += 100.0f / (dist_to_region + 1);
        
        // 危险惩罚
        if (player->your_status <= 0) {  // 非强化状态
            score -= region_map[i].danger_level * 30.0f;
                }
                
                if (score > best_score) {
                    best_score = score;
            best_region = i;
        }
    }
    
    return best_region;
}

// 【新增：分层路径规划的主函数】
int hierarchical_pathfinding(struct Player *player, int sx, int sy, int tx, int ty, int *next_x, int *next_y) {
    int map_size = player->row_cnt * player->col_cnt;
    
    // 小地图直接使用A*
    if (map_size <= LARGE_MAP_THRESHOLD) {
        return astar_limited(player, sx, sy, tx, ty, next_x, next_y);
    }
    
    // 大地图使用分层规划
    initialize_region_map(player);
    
    int source_region = get_region_index(player, sx, sy);
    int target_region = get_region_index(player, tx, ty);
    
    if (source_region == -1 || target_region == -1) {
        return 0;  // 无效区域
    }
    
    // 如果在同一区域，直接使用A*
    if (source_region == target_region) {
        return astar_limited(player, sx, sy, tx, ty, next_x, next_y);
    }
    
    // 不同区域：先规划到目标区域的边界
    int target_center_x = region_map[target_region].center_x;
    int target_center_y = region_map[target_region].center_y;
    
    // 找到目标区域的边界点（靠近源点的边界）
    int boundary_x = target_center_x;
    int boundary_y = target_center_y;
    
    // 简化边界点计算：向源点方向移动
    if (target_center_x > sx) {
        boundary_x = target_center_x - REGION_SIZE / 2;
        } else {
        boundary_x = target_center_x + REGION_SIZE / 2;
    }
    
    if (target_center_y > sy) {
        boundary_y = target_center_y - REGION_SIZE / 2;
    } else {
        boundary_y = target_center_y + REGION_SIZE / 2;
    }
    
    // 确保边界点有效
    if (boundary_x < 0) boundary_x = 0;
    if (boundary_x >= player->row_cnt) boundary_x = player->row_cnt - 1;
    if (boundary_y < 0) boundary_y = 0;
    if (boundary_y >= player->col_cnt) boundary_y = player->col_cnt - 1;
    
    // 使用A*规划到边界点
    return astar_limited(player, sx, sy, boundary_x, boundary_y, next_x, next_y);
}

//【新增：策略0 - 非强化状态智能远离策略】
int handle_non_power_escape_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    if (player->your_status != 0) return 0;  // 只处理非强化状态
    
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    // 如果距离破坏者太近，需要主动远离
    if (min_ghost_dist <= 3) {
        // 评估对方AI到最近超级星的距离（考虑竞争因素）
        int nearest_power_star_dist_to_opponent = INF;
        int opponent_x, opponent_y, opponent_score;
        if (get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
            build_star_cache(player);
            for (int k = 0; k < power_star_cache_size; k++) {
                int dist = abs(opponent_x - power_star_cache[k].x) + abs(opponent_y - power_star_cache[k].y);
                if (dist < nearest_power_star_dist_to_opponent) {
                    nearest_power_star_dist_to_opponent = dist;
                }
            }
        }
        
        // 智能远离：选择最安全的移动方向
        int best_escape_dir = -1;
        int max_escape_value = -1000;
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny)) {
                int escape_value = 0;
                
                // 基础安全价值：距离破坏者越远越好
                int new_ghost_dist = distance_to_ghost(player, nx, ny);
                escape_value += new_ghost_dist * 30;
                
                // 避免死路：检查后续移动选择
                int exit_count = 0;
                for (int dd = 0; dd < 4; dd++) {
                    int check_x = nx + dx[dd], check_y = ny + dy[dd];
                    if (is_valid(player, check_x, check_y)) {
                        exit_count++;
                    }
                }
                escape_value += exit_count * 10;  // 出路越多越安全
                
                // 如果这个位置有星星，适度奖励（但安全优先）
                char cell = player->mat[nx][ny];
                if (cell == 'o') {
                    escape_value += 15;  // 普通星星小奖励
                } else if (cell == 'O') {
                    escape_value += 25;  // 超级星中等奖励
                    
                    // 但要考虑与对手的竞争：如果对手更近，则降低价值
                    if (nearest_power_star_dist_to_opponent != INF) {
                        int our_dist_to_power = abs(nx - power_star_cache[0].x) + abs(ny - power_star_cache[0].y);  // 简化计算
                        if (our_dist_to_power > nearest_power_star_dist_to_opponent + 2) {
                            escape_value -= 20;  // 对手更有优势，降低价值
                        }
                    }
                }
                
                // 避免震荡
                if (nx == last_x && ny == last_y) {
                    escape_value -= 50;  // 惩罚回头
                }
                
                // 在逃跑时也考虑对手干扰机会
                int our_safety = assess_danger_level(player, nx, ny);
                if (our_safety <= 1) {  // 相对安全时
                    float interference_value = evaluate_opponent_interference(player, x, y, nx, ny);
                    if (interference_value > 0) {
                        escape_value += (int)(interference_value * 0.5);  // 逃跑时降低干扰权重
                    }
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
            return 1;  // 成功处理
        }
    }
    
    return 0;  // 未处理
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
        
        // 计算对手分值的一半 vs 破坏者得分(200分)的比较
        float opponent_half_score = opponent_score * 0.5f;
        float ghost_score = 200.0f;
        
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
    
    // 优先级判断：对手价值 vs 破坏者价值
    if (should_hunt_opponent && (opponent_hunt_value > 200.0f || min_ghost_dist > 8)) {
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
                if (hierarchical_pathfinding(player, x, y, nearest_power_star_x, nearest_power_star_y, next_x, next_y)) {
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
            if (hierarchical_pathfinding(player, x, y, nearest_star_x, nearest_star_y, next_x, next_y)) {
                return 1;  // 成功处理
            }
        }
    }
    
    return 0;  // 未处理
}

//【新增：策略2 - 智能星星收集策略】
int handle_star_collection_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    build_star_cache(player);
    
    int best_star_x = -1, best_star_y = -1;
    float best_score = -1000.0;
    int danger_level = assess_danger_level(player, x, y);
    
    // 优先考虑超级星
    for (int k = 0; k < power_star_cache_size; k++) {
        int star_x = power_star_cache[k].x, star_y = power_star_cache[k].y;
        int star_danger = assess_danger_level(player, star_x, star_y);
        int star_dist = abs(x - star_x) + abs(y - star_y);
        
        // 超级星判断逻辑
        int should_consider_power_star = 0;
        
        // 资源封锁策略检查
        float blocking_value = evaluate_resource_blocking(player, x, y, star_x, star_y);
        if (blocking_value > 50.0) {
            should_consider_power_star = 1;
        }
        
        // 基础安全条件
        if (star_danger < 2) {
            should_consider_power_star = 1;
        }
        // 死胡同反杀机会
        else if (star_danger >= 2) {
            float counter_value = evaluate_power_star_counter_attack_value(player, x, y, star_x, star_y);
            
            int escape_routes = 0;
            for (int d = 0; d < 4; d++) {
                int check_x = star_x + dx[d];
                int check_y = star_y + dy[d];
                if (is_valid(player, check_x, check_y)) {
                    escape_routes++;
                }
            }
            
            if (escape_routes <= 2 && counter_value >= 200.0f && star_dist <= 4) {
                should_consider_power_star = 1;
            }
            else if (counter_value >= 300.0f && star_dist <= 3) {
                should_consider_power_star = 1;
            }
            else if (counter_value >= 150.0f && star_dist <= 2) {
                should_consider_power_star = 1;
            }
        }
        
        if (should_consider_power_star) {
            float score = evaluate_star(player, x, y, star_x, star_y);
            
            if (blocking_value > 0) {
                score += blocking_value * 0.8;
            }
            
            if (player->your_score < 2000) {
                score += 150.0;
            }
            
            if (star_dist <= 5) {
                score += 30.0;
            } else if (star_dist <= 10) {
                score += 10.0;
            }
            
            if (score > best_score) {
                best_score = score;
                best_star_x = star_x;
                best_star_y = star_y;
            }
        }
    }
    
    // 考虑普通星星（只有在没有好的超级星时）
    if (best_score < 100.0) {
        for (int k = 0; k < star_cache_size; k++) {
            int star_danger = assess_danger_level(player, star_cache[k].x, star_cache[k].y);
            int star_dist = abs(x - star_cache[k].x) + abs(y - star_cache[k].y);
            
            if (star_danger < 2 || player->your_status > 0) {
                float score = evaluate_star(player, x, y, star_cache[k].x, star_cache[k].y);
                
                if (score > best_score) {
                    best_score = score;
                    best_star_x = star_cache[k].x;
                    best_star_y = star_cache[k].y;
                }
            }
        }
    }
    
    // 执行星星收集移动
    if (best_star_x != -1) {
        if (hierarchical_pathfinding(player, x, y, best_star_x, best_star_y, next_x, next_y)) {
            return 1;  // 成功处理
        }
    }
    
    return 0;  // 未处理
}

//【新增：策略3 - 安全移动策略】
int handle_safe_movement_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    return choose_safe_direction_with_oscillation_check(player, x, y, next_x, next_y);
}

//【新增：策略4 - 智能探索策略】
int handle_intelligent_exploration_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_x = x, best_y = y;
    int max_score = -1000;
    
    // 检测震荡模式
    int oscillation_type = detect_path_oscillation(x, y);
    
    // 如果检测到震荡，优先尝试打破震荡
    if (oscillation_type > 0) {
        int break_x, break_y;
        if (break_oscillation_pattern(player, x, y, &break_x, &break_y)) {
            // 验证打破震荡的移动是否安全
            int ghost_dist = distance_to_ghost(player, break_x, break_y);
            int danger = assess_danger_level(player, break_x, break_y);
            
            if (danger < 2 || ghost_dist > 2) {
                *next_x = break_x;
                *next_y = break_y;
                return 1;  // 成功处理
            }
        }
    }
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            int ghost_dist = distance_to_ghost(player, nx, ny);
            int explore_score = ghost_dist * 3;
            
            // 增强的历史位置惩罚
            int history_penalty = 0;
            for (int i = 0; i < path_history_count; i++) {
                if (path_history_x[i] == nx && path_history_y[i] == ny) {
                    int recency = path_history_count - i;
                    history_penalty += recency * 20;
                }
            }
            explore_score -= history_penalty;
            
            // 强力防震荡：大幅惩罚回头
            if (nx == last_x && ny == last_y) {
                explore_score -= 150;
            }
            
            // 智能方向选择
            int center_x = player->row_cnt / 2;
            int center_y = player->col_cnt / 2;
            int dist_from_center = abs(nx - center_x) + abs(ny - center_y);
            
            // 根据震荡类型选择不同策略
            if (oscillation_type >= 2) {
                // 严重震荡：强制选择垂直或水平方向
                int horizontal_moves = 0, vertical_moves = 0;
                
                for (int i = 1; i < path_history_count && i < 4; i++) {
                    int idx_curr = (path_history_head - i + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
                    int idx_prev = (path_history_head - i - 1 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
                    
                    if (path_history_x[idx_curr] != path_history_x[idx_prev]) {
                        horizontal_moves++;
                    }
                    if (path_history_y[idx_curr] != path_history_y[idx_prev]) {
                        vertical_moves++;
                    }
                }
                
                if (horizontal_moves > vertical_moves) {
                    if (d == 0 || d == 2) {  // 上下移动
                        explore_score += 300;
                    }
                } else {
                    if (d == 1 || d == 3) {  // 左右移动
                        explore_score += 300;
                    }
                }
            } else if (oscillation_type == 1) {
                int dist_from_last = abs(nx - last_x) + abs(ny - last_y);
                if (dist_from_last > 1) {
                    explore_score += 100;
                }
                explore_score += dist_from_center * 2;
            }
            
            // 边缘探索奖励
            if (nx < 2 || nx >= player->row_cnt - 2 || ny < 2 || ny >= player->col_cnt - 2) {
                explore_score += 15;
            }
            
            // 角落探索额外奖励
            if ((nx <= 1 || nx >= player->row_cnt - 2) && (ny <= 1 || ny >= player->col_cnt - 2)) {
                explore_score += 25;
            }
            
            // 对手干扰策略评估
            float interference_value = evaluate_opponent_interference(player, x, y, nx, ny);
            if (interference_value > 0) {
                explore_score += (int)interference_value;
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
        // 紧急情况：随机选择一个有效移动（非回头）
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !(nx == last_x && ny == last_y)) {
                best_x = nx;
                best_y = ny;
                break;
            }
        }
        
        // 如果实在没有，选择任意有效移动
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
    return 1;  // 总是成功处理
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
    
    // 策略0：非强化状态下的智能远离策略
    if (handle_non_power_escape_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; 
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略1：强化状态下的积极追击（最高优先级）
    if (handle_power_hunt_strategy(player, x, y, &next_x, &next_y)) {
    last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
    return ret;
}

    // 策略2：智能星星收集
    if (handle_star_collection_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略3：安全移动（当没有可收集的星星时）
    if (handle_safe_movement_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 策略4：智能探索（确保总是能移动，避免震荡）
    if (handle_intelligent_exploration_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
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