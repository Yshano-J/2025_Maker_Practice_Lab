#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

static int star_eaten[MAXN][MAXN];

static const int dx[4] = {-1, 0, 1, 0};
static const int dy[4] = {0, 1, 0, -1};

// A* 节点结构
struct AStarNode {
    int x, y;          // 节点在地图中的坐标
    int g_cost;        // 从起点到当前节点的实际距离
    int h_cost;        // 启发函数值（到目标的估计距离）
    int f_cost;        // g_cost + h_cost（总评估成本）
    int parent_x, parent_y;  // 父节点坐标（用于路径回溯）
    int in_open;       // 是否在开放列表中（已发现，待探索）
    int in_closed;     // 是否在关闭列表中（已探索，不再考虑）
};

static struct AStarNode nodes[MAXN][MAXN];
static int open_list[MAXN * MAXN * 2];
static int open_size;

void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
}

int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// 改进的启发函数，考虑墙体影响
int heuristic(struct Player *player, int x1, int y1, int x2, int y2) {
    int manhattan = abs(x1 - x2) + abs(y1 - y2);
    
    // 短距离时曼哈顿距离较准确
    if (manhattan <= 3) {
        return manhattan;
    }
    
    // 检查直线路径上的墙体密度
    int wall_penalty = 0;
    int dx = (x2 > x1) ? 1 : ((x2 < x1) ? -1 : 0);
    int dy = (y2 > y1) ? 1 : ((y2 < y1) ? -1 : 0);
    
    int steps = (abs(x2 - x1) > abs(y2 - y1)) ? abs(x2 - x1) : abs(y2 - y1);
    int wall_count = 0;
    
    for (int i = 1; i <= steps && i <= 10; i++) {  // 只检查前10步，避免过度计算
        int check_x = x1 + dx * i;
        int check_y = y1 + dy * i;
        
        if (check_x >= 0 && check_x < player->row_cnt && 
            check_y >= 0 && check_y < player->col_cnt) {
            if (player->mat[check_x][check_y] == '#') {
                wall_count++;
            }
        }
    }
    
    // 根据墙体数量调整距离估计
    wall_penalty = wall_count * 2;  // 每个墙增加2的惩罚
    
    return manhattan + wall_penalty;
}

// 计算到破坏者的最短距离（用于风险评估）
int distance_to_ghost(struct Player *player, int x, int y) {
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = heuristic(player, x, y, player->ghost_posx[i], player->ghost_posy[i]);
        if (dist < min_dist) min_dist = dist;
    }
    return min_dist;
}

// 评估星星的价值（贪心评估函数）
float evaluate_star(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int star_value = (star_type == 'O') ? 15 : 10;  // 超级星更有价值
    
    int distance = heuristic(player, sx, sy, star_x, star_y);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    int opponent_dist = heuristic(player, star_x, star_y, player->opponent_posx, player->opponent_posy);
    
    // 降低风险惩罚，避免过度保守
    float danger_penalty = 0;
    if (ghost_dist <= 2 && player->your_status <= 0) {
        danger_penalty = 2.0 / (ghost_dist + 1);  // 降低惩罚
    }
    
    // 竞争评估
    float competition_penalty = 0;
    if (opponent_dist < distance && player->opponent_status != -1) {
        competition_penalty = 1.0;  // 降低竞争惩罚
    }
    
    // 强化状态奖励
    float power_bonus = 0;
    if (player->your_status > 0) {
        power_bonus = 3.0;  // 强化状态下更积极
        if (star_type == 'O') {
            // 强化状态下超级星的处理策略
            if (player->your_status <= 3) {
                // 强化即将结束，超级星优先级极高
                power_bonus += 8.0;
            } else if (player->your_status <= 8) {
                // 强化状态中期，超级星优先级高
                power_bonus += 5.0;
            } else {
                // 强化状态充足，超级星优先级适中
                power_bonus += 2.0;
            }
        }
    } else {
        // 普通状态下，超级星优先级更高
        if (star_type == 'O') {
            power_bonus += 4.0;
        }
    }
    
    // 距离奖励：优先近距离目标
    float distance_bonus = 0;
    if (distance <= 3) distance_bonus = 2.0;
    
    // 综合评分：价值密度 - 风险 - 竞争 + 奖励
    return (float)star_value / (distance + 1) - danger_penalty - competition_penalty + power_bonus + distance_bonus;
}

// BFS备用寻路（简单但可靠）
int bfs_backup(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int vis[MAXN][MAXN] = {0};
    int prex[MAXN][MAXN], prey[MAXN][MAXN];
    int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
    
    qx[tail] = sx; qy[tail] = sy; tail++;
    vis[sx][sy] = 1;
    prex[sx][sy] = -1; prey[sx][sy] = -1;
    
    while (head < tail) {
        int x = qx[head], y = qy[head]; head++;
        
        if (is_star(player, x, y)) {
            // 找到星星，回溯第一步
            int px = x, py = y;
            while (!(prex[px][py] == sx && prey[px][py] == sy)) {
                int tpx = prex[px][py], tpy = prey[px][py];
                px = tpx; py = tpy;
            }
            *next_x = px; *next_y = py;
            return 1;
        }
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny]) {
                vis[nx][ny] = 1;
                prex[nx][ny] = x; prey[nx][ny] = y;
                qx[tail] = nx; qy[tail] = ny; tail++;
            }
        }
    }
    return 0;
}

// 最小堆操作
void heap_push(int node_idx) {
    open_list[open_size++] = node_idx;
    int pos = open_size - 1;
    while (pos > 0) {
        int parent = (pos - 1) / 2;
        int curr_x = node_idx / MAXN, curr_y = node_idx % MAXN;
        int parent_x = open_list[parent] / MAXN, parent_y = open_list[parent] % MAXN;
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
        
        if (nodes[left_x][left_y].f_cost < nodes[pos_x][pos_y].f_cost) {
            smallest = left;
        }
        
        if (right < open_size) {
            int right_x = open_list[right] / MAXN, right_y = open_list[right] % MAXN;
            int smallest_x = open_list[smallest] / MAXN, smallest_y = open_list[smallest] % MAXN;
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

// A* 寻路算法
int Astar(struct Player *player, int sx, int sy, int tx, int ty, int *next_x, int *next_y) {
    // 初始化节点
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
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
    heap_push(sx * MAXN + sy); // 如果直接存储坐标对，堆操作会很复杂，所以用一个一维数组来存储
    
    while (open_size > 0) {
        int current_idx = heap_pop();
        int x = current_idx / MAXN, y = current_idx % MAXN;
        
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
    
    return 0;  // 找不到路径
}

// 判断是否应该不移动（等待策略）
int should_stay(struct Player *player, int x, int y) {
    // 1. 危险规避：周围都是破坏者，不移动更安全
    int current_ghost_dist = distance_to_ghost(player, x, y);
    int safe_moves = 0;
    int total_moves = 0;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            total_moves++;
            int ghost_dist = distance_to_ghost(player, nx, ny);
            if (ghost_dist >= current_ghost_dist) {
                safe_moves++;
            }
        }
    }
    
    // 只有在极度危险的情况下才不移动
    if (player->your_status <= 0 && current_ghost_dist <= 1 && safe_moves == 0 && total_moves > 0) {
        return 1;  // 不移动更安全
    }
    
    // 2. 等待对手：条件更严格，只有在非常明显的情况下才等待
    int opponent_closer_count = 0;
    int total_stars = 0;
    
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                total_stars++;
                int my_dist = heuristic(player, x, y, i, j);
                int opponent_dist = heuristic(player, i, j, player->opponent_posx, player->opponent_posy);
                
                // 对手明显更近的星星
                if (opponent_dist < my_dist - 3 && my_dist <= 4) {
                    opponent_closer_count++;
                }
            }
        }
    }
    
    // 只有当对手明显更近大部分星星时才等待
    if (total_stars > 0 && opponent_closer_count >= total_stars / 2 && 
        player->opponent_status != -1 && total_stars <= 3) {
        return 1;  // 等待对手先去
    }
    
    // 3. 强化状态管理：只有在回合数充足且安全的情况下才等待
    if (player->your_status >= 10) {  // 提高到10回合以上
        // 检查是否有超级星在附近，且对手距离较远
        for (int i = x - 2; i <= x + 2; i++) {
            for (int j = y - 2; j <= y + 2; j++) {
                if (i >= 0 && i < player->row_cnt && j >= 0 && j < player->col_cnt) {
                    if (player->mat[i][j] == 'O' && !star_eaten[i][j]) {
                        int my_dist = heuristic(player, x, y, i, j);
                        int opponent_dist = heuristic(player, i, j, player->opponent_posx, player->opponent_posy);
                        
                        // 条件更严格：距离很近，对手很远，强化状态很充足
                        if (my_dist <= 1 && opponent_dist > my_dist + 5 && player->your_status >= 15) {
                            return 1;  // 等待强化结束后再去拿超级星
                        }
                    }
                }
            }
        }
    }
    
    return 0;  // 正常移动
}

struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    
    // 标记当前位置的星星为已吃
    if (is_star(player, x, y)) {
        star_eaten[x][y] = 1;
    }
    
    // 检查是否应该不移动
    if (should_stay(player, x, y)) {
        struct Point ret = {x, y};  // 不移动，返回当前位置
        return ret;
    }
    
    // 寻找最优目标星星
    int best_star_x = -1, best_star_y = -1;
    float best_score = -1000.0;
    
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                float score = evaluate_star(player, x, y, i, j);
                if (score > best_score) {
                    best_score = score;
                    best_star_x = i;
                    best_star_y = j;
                }
            }
        }
    }
    
    // 如果找到目标星星，优先用A*寻路
    if (best_star_x != -1) {
        int next_x, next_y;
        if (Astar(player, x, y, best_star_x, best_star_y, &next_x, &next_y)) {
            // 再次检查移动后的位置是否安全
            int move_ghost_dist = distance_to_ghost(player, next_x, next_y);
            int current_ghost_dist = distance_to_ghost(player, x, y);
            
            // 如果移动会让我们更接近破坏者且没有强化状态，考虑不移动
            if (player->your_status <= 0 && move_ghost_dist < current_ghost_dist && 
                move_ghost_dist <= 2 && best_score < 5.0) {
                struct Point ret = {x, y};  // 不移动
                return ret;
            }
            
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // A*失败，尝试BFS找任意星星
    int next_x, next_y;
    if (bfs_backup(player, x, y, &next_x, &next_y)) {
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // 没有星星可达，智能探索策略
    int explore_x = x, explore_y = y;
    int max_score = -1000;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            int ghost_dist = distance_to_ghost(player, nx, ny);
            int explore_score = ghost_dist;  // 优先远离破坏者
            
            // 倾向于向地图中心移动
            int center_x = player->row_cnt / 2;
            int center_y = player->col_cnt / 2;
            int center_dist = abs(nx - center_x) + abs(ny - center_y);
            explore_score -= center_dist / 2;
            
            if (explore_score > max_score) {
                max_score = explore_score;
                explore_x = nx;
                explore_y = ny;
            }
        }
    }
    
    // 如果探索得分很低，选择不移动
    if (max_score < 0) {
        struct Point ret = {x, y};  // 不移动
        return ret;
    }
    
    struct Point ret = {explore_x, explore_y};
    return ret;
}
