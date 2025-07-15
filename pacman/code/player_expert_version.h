#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// ר�Ҽ�AI����
#define SUPER_STAR_VALUE 2000.0f     // �����ǻ�����ֵ
#define NORMAL_STAR_VALUE 150.0f     // ��ͨ�ǻ�����ֵ
#define GHOST_HUNT_VALUE 1000.0f     // ���׷����ֵ
#define COMPETITION_BONUS 800.0f     // ��������
#define BLOCK_BONUS 500.0f           // �赲����

// ��̬������ֵ
#define EXTREME_LOW_THRESHOLD 2000   // ���ȼ�����ֵ
#define LOW_THRESHOLD 4000           // ������ֵ  
#define HIGH_THRESHOLD 8000          // ������ֵ

// ʱ�����
static clock_t start_time;
static const int MAX_TIME_LIMIT_MS = 85;  // 85msʱ������

// ��Ϸ״̬׷��
static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;
static int opponent_last_x = -1, opponent_last_y = -1;
static int total_stars = 0;
static int game_phase = 0; // 0:early, 1:mid, 2:late

// �߼�����
static int path_history[20][2];  // ·����ʷ
static int path_history_size = 0;
static int target_x = -1, target_y = -1;  // ����Ŀ��
static int target_lock_count = 0;
static int ghost_predicted_x[2][5], ghost_predicted_y[2][5];  // ���Ԥ��

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// ʱ����
int is_timeout() {
    clock_t current_time = clock();
    double elapsed_ms = ((double)(current_time - start_time)) / CLOCKS_PER_SEC * 1000;
    return elapsed_ms > MAX_TIME_LIMIT_MS;
}

// ��ʼ��
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    memset(path_history, 0, sizeof(path_history));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
    opponent_last_x = -1;
    opponent_last_y = -1;
    path_history_size = 0;
    target_x = -1;
    target_y = -1;
    target_lock_count = 0;
    
    // ������������
    total_stars = 0;
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (player->mat[i][j] == 'o' || player->mat[i][j] == 'O') {
                total_stars++;
            }
        }
    }
}

// ��������
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// ·����ʷ����
void add_to_path_history(int x, int y) {
    if (path_history_size >= 20) {
        for (int i = 0; i < 19; i++) {
            path_history[i][0] = path_history[i + 1][0];
            path_history[i][1] = path_history[i + 1][1];
        }
        path_history_size = 19;
    }
    path_history[path_history_size][0] = x;
    path_history[path_history_size][1] = y;
    path_history_size++;
}

// ���·���ظ��ͷ�
int get_path_penalty(int x, int y) {
    int penalty = 0;
    for (int i = max(0, path_history_size - 8); i < path_history_size; i++) {
        if (path_history[i][0] == x && path_history[i][1] == y) {
            penalty += (8 - (path_history_size - 1 - i)) * 50;  // Խ�����ظ��ͷ�Խ��
        }
    }
    return penalty;
}

// ���¶���λ��
void update_opponent_position(struct Player *player) {
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            char c = player->mat[i][j];
            if (c == 'A' || c == 'B') {
                if (!(i == player->your_posx && j == player->your_posy)) {
                    opponent_last_x = opponent_x;
                    opponent_last_y = opponent_y;
                    opponent_x = i;
                    opponent_y = j;
                    return;
                }
            }
        }
    }
}

// ������Ϸ�׶�
void update_game_phase(struct Player *player) {
    int remaining_stars = 0;
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                remaining_stars++;
            }
        }
    }
    
    if (remaining_stars > total_stars * 0.7f) {
        game_phase = 0; // ����
    } else if (remaining_stars > total_stars * 0.3f) {
        game_phase = 1; // ����
    } else {
        game_phase = 2; // ����
    }
}

// ���㵽���ľ���
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

// ����ƶ�Ԥ�� - 4��ǰհ
void predict_ghost_moves(struct Player *player) {
    for (int ghost_id = 0; ghost_id < 2; ghost_id++) {
        int gx = player->ghost_posx[ghost_id];
        int gy = player->ghost_posy[ghost_id];
        
        ghost_predicted_x[ghost_id][0] = gx;
        ghost_predicted_y[ghost_id][0] = gy;
        
        // Ԥ��4���ƶ�
        for (int step = 1; step <= 4; step++) {
            if (is_timeout()) break;
            
            int target_player_x = player->your_posx;
            int target_player_y = player->your_posy;
            
            int best_dist = INF;
            int next_gx = gx, next_gy = gy;
            
            for (int d = 0; d < 4; d++) {
                int new_gx = gx + dx[d];
                int new_gy = gy + dy[d];
                
                if (is_valid(player, new_gx, new_gy)) {
                    int dist = abs(new_gx - target_player_x) + abs(new_gy - target_player_y);
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

// ��̬��ȫ����
int is_extremely_safe(struct Player *player, int x, int y, int future_step) {
    if (!is_valid(player, x, y)) return 0;
    
    // ǿ��״̬������ʣ��ʱ�����
    if (player->your_status > 0) {
        if (player->your_status > 5) return 1;  // ʱ�������ȫ����
        // ʱ�䲻��ҲҪ������ֻ����ֱ����ײ
        for (int i = 0; i < 2; i++) {
            if (x == player->ghost_posx[i] && y == player->ghost_posy[i]) return 0;
        }
        return 1;
    }
    
    // ���ݷ�����̬������ȫ����
    int required_distance;
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        required_distance = 1;  // ��������ʱֻҪ��1�����
    } else if (player->your_score < LOW_THRESHOLD) {
        required_distance = 2;  // �����ϵ�ʱҪ��2�����
    } else {
        required_distance = 3;  // ��������ʱҪ��3�����
    }
    
    // ��鵱ǰ���λ��
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < required_distance) return 0;
    }
    
    // ���Ԥ��Ĺ��λ��
    for (int i = 0; i < 2; i++) {
        int step = min(future_step, 4);
        if (step > 0) {
            int ghost_x = ghost_predicted_x[i][step];
            int ghost_y = ghost_predicted_y[i][step];
            int dist = abs(x - ghost_x) + abs(y - ghost_y);
            if (dist < required_distance) return 0;
        }
    }
    
    return 1;
}

// A*�㷨ʵ��
int astar_pathfind(struct Player *player, int start_x, int start_y, int target_x, int target_y, int *next_x, int *next_y) {
    if (is_timeout()) return 0;
    if (start_x == target_x && start_y == target_y) return 0;
    
    static int g_cost[MAXN][MAXN], f_cost[MAXN][MAXN];
    static int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    static int open_x[400], open_y[400];
    static int closed[MAXN][MAXN];
    
    // ��ʼ��
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
    
    // ��̬��������������
    int max_iterations = (player->your_score < EXTREME_LOW_THRESHOLD) ? 180 : 120;
    int iterations = 0;
    
    while (open_size > 0 && iterations < max_iterations && !is_timeout()) {
        iterations++;
        
        // �����Žڵ�
        int best_idx = 0;
        for (int i = 1; i < open_size; i++) {
            if (f_cost[open_x[i]][open_y[i]] < f_cost[open_x[best_idx]][open_y[best_idx]]) {
                best_idx = i;
            }
        }
        
        int current_x = open_x[best_idx];
        int current_y = open_y[best_idx];
        
        // �Ƴ���ǰ�ڵ�
        for (int i = best_idx; i < open_size - 1; i++) {
            open_x[i] = open_x[i + 1];
            open_y[i] = open_y[i + 1];
        }
        open_size--;
        
        closed[current_x][current_y] = 1;
        
        // �ҵ�Ŀ��
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
        
        // ��չ�ھ�
        for (int d = 0; d < 4; d++) {
            int nx = current_x + dx[d];
            int ny = current_y + dy[d];
            
            if (!is_valid(player, nx, ny) || closed[nx][ny]) continue;
            
            // ��ȫ���
            if (!is_extremely_safe(player, nx, ny, abs(nx - start_x) + abs(ny - start_y)) && player->your_status == 0) {
                if (player->your_score >= LOW_THRESHOLD) continue;
            }
            
            int tentative_g = g_cost[current_x][current_y] + 1;
            
            if (tentative_g < g_cost[nx][ny]) {
                parent_x[nx][ny] = current_x;
                parent_y[nx][ny] = current_y;
                g_cost[nx][ny] = tentative_g;
                f_cost[nx][ny] = tentative_g + abs(nx - target_x) + abs(ny - target_y);
                
                // ��ӵ������б�
                int found = 0;
                for (int i = 0; i < open_size; i++) {
                    if (open_x[i] == nx && open_y[i] == ny) {
                        found = 1;
                        break;
                    }
                }
                
                if (!found && open_size < 400) {
                    open_x[open_size] = nx;
                    open_y[open_size] = ny;
                    open_size++;
                }
            }
        }
    }
    
    return 0;
}

// ר�Ҽ���������
float evaluate_star_expert(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int my_dist = abs(star_x - sx) + abs(star_y - sy);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    
    // ������ֵ
    float value = (star_type == 'O') ? SUPER_STAR_VALUE : NORMAL_STAR_VALUE;
    
    // ��̬��������
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        value *= 3.0f;  // ��������ʱ���������ֵ
    } else if (player->your_score < LOW_THRESHOLD) {
        value *= 2.0f;  // �����ϵ�ʱ������ֵ
    }
    
    // ����ɱ� - ��̬����
    float distance_cost = my_dist * (game_phase == 2 ? 10.0f : 6.0f);
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        distance_cost *= 0.7f;  // �ͷ�ʱ���;���ɱ�
    }
    value -= distance_cost;
    
    // ���־������� - ��ǿ��
    if (opponent_x != -1) {
        int opponent_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        
        if (my_dist < opponent_dist) {
            value += COMPETITION_BONUS * (opponent_dist - my_dist) / (float)(opponent_dist + 1);
        } else if (star_type == 'O' && my_dist - opponent_dist <= 2) {
            value += COMPETITION_BONUS * 0.9f; // ��Ҫ������ҪĿ��
        } else if (my_dist - opponent_dist > 5) {
            value *= 0.5f; // ̫Զ��Ŀ�����������ȼ�
        }
    }
    
    // ��ȫ������
    if (!is_extremely_safe(player, star_x, star_y, my_dist / 3)) {
        if (player->your_status > 0) {
            value += 300.0f;  // ǿ��״̬�¹���ð��
        } else {
            if (player->your_score < EXTREME_LOW_THRESHOLD) {
                value -= 400.0f;  // ���ͷ�ʱ�ʶȳͷ�Σ��
            } else {
                value -= 1200.0f; // ��������ʱ���سͷ�Σ��
            }
        }
    }
    
    // �ۺϼ�ֵ����
    int nearby_stars = 0;
    for (int radius = 1; radius <= 4; radius++) {
        for (int i = max(0, star_x - radius); i <= min(player->row_cnt - 1, star_x + radius); i++) {
            for (int j = max(0, star_y - radius); j <= min(player->col_cnt - 1, star_y + radius); j++) {
                if (is_star(player, i, j) && (i != star_x || j != star_y)) {
                    int dist = abs(i - star_x) + abs(j - star_y);
                    if (dist <= radius) {
                        nearby_stars++;
                        value += (300.0f / (dist + 1)) * (5 - radius) / 4.0f;
                    }
                }
            }
        }
    }
    
    // ��Ϸ�׶ε���
    if (game_phase == 0) {
        if (star_type == 'O') value += 300.0f;
    } else if (game_phase == 2) {
        value += 200.0f;
        if (star_type == 'O') value += 500.0f;
    }
    
    // Ŀ����������
    if (star_x == target_x && star_y == target_y) {
        value += 150.0f * target_lock_count;
    }
    
    // ·����ʷ�ͷ�
    value -= get_path_penalty(star_x, star_y);
    
    return value;
}

// Ѱ��������� - ר�Ұ�
int find_best_star_expert(struct Player *player, int sx, int sy, int *best_x, int *best_y) {
    float best_value = -INF;
    int found = 0;
    
    // ��̬������Χ
    int search_radius = (player->your_score < EXTREME_LOW_THRESHOLD) ? 12 : 8;
    
    for (int i = max(0, sx - search_radius); i <= min(player->row_cnt - 1, sx + search_radius) && !is_timeout(); i++) {
        for (int j = max(0, sy - search_radius); j <= min(player->col_cnt - 1, sy + search_radius); j++) {
            if (is_star(player, i, j)) {
                float value = evaluate_star_expert(player, sx, sy, i, j);
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

// ר�Ҽ����׷��
int expert_ghost_hunt(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (player->your_status <= 0) return 0;
    
    // ������Ĺ��
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
    
    // ����׷���ж� - ����ʣ��ʱ��;���
    int hunt_threshold = (game_phase == 2) ? 15 : 10;
    if (player->your_status >= 4 || min_dist <= hunt_threshold) {
        return astar_pathfind(player, sx, sy, 
                             player->ghost_posx[target_ghost], 
                             player->ghost_posy[target_ghost], 
                             next_x, next_y);
    }
    
    return 0;
}

// �߼��赲����
int expert_block_opponent(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (opponent_x == -1 || game_phase == 0) return 0;
    
    // Ѱ�Ҷ��ֿ���Ŀ��ĸ߼�ֵ����
    int best_star_x = -1, best_star_y = -1;
    float best_threat = 0.0f;
    
    for (int i = 0; i < player->row_cnt && !is_timeout(); i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j) && player->mat[i][j] == 'O') { // ֻ���ǳ�����
                int opp_dist = abs(i - opponent_x) + abs(j - opponent_y);
                int my_dist = abs(i - sx) + abs(j - sy);
                
                // ����������Ը��ӽ��������л����赲
                if (opp_dist < my_dist && opp_dist <= 4 && my_dist <= 8) {
                    float threat = BLOCK_BONUS / (float)(opp_dist + 1);
                    if (threat > best_threat) {
                        best_threat = threat;
                        best_star_x = i;
                        best_star_y = j;
                    }
                }
            }
        }
    }
    
    // �����ƶ�������赲λ��
    if (best_star_x != -1) {
        return astar_pathfind(player, sx, sy, best_star_x, best_star_y, next_x, next_y);
    }
    
    return 0;
}

// ��ȫ�ƶ� - ��ǿ��
int safe_move_expert(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_dir = -1;
    float max_safety = -INF;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        float safety = distance_to_ghost(player, nx, ny);
        
        // ����Ԥ��Ĺ��λ��
        for (int i = 0; i < 2; i++) {
            int pred_dist = abs(nx - ghost_predicted_x[i][1]) + abs(ny - ghost_predicted_y[i][1]);
            safety += pred_dist * 0.5f;
        }
        
        // ������ָ���
        if (opponent_x != -1) {
            int opp_dist = abs(nx - opponent_x) + abs(ny - opponent_y);
            if (opp_dist >= 2) safety += 2.0f;
        }
        
        // �����ǵ�λ�üӷ�
        if (is_star(player, nx, ny)) {
            safety += (player->mat[nx][ny] == 'O') ? 15.0f : 8.0f;
        }
        
        // ·����ʷ�ͷ�
        safety -= get_path_penalty(nx, ny) * 0.1f;
        
        // �ƶ�������
        int mobility = 0;
        for (int dd = 0; dd < 4; dd++) {
            if (is_valid(player, nx + dx[dd], ny + dy[dd])) {
                mobility++;
            }
        }
        safety += mobility * 1.0f;
        
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

// �����ߺ��� - ר�Ҽ�AI
struct Point walk(struct Player *player) {
    start_time = clock();
    step_count++;
    
    int x = player->your_posx, y = player->your_posy;
    
    // ������Ϸ״̬
    update_opponent_position(player);
    update_game_phase(player);
    add_to_path_history(x, y);
    predict_ghost_moves(player);
    
    // ��������״̬
    if (is_star(player, x, y)) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // ����1: ǿ��״̬ר�Ҽ�׷�����
    if (player->your_status > 0) {
        if (expert_ghost_hunt(player, x, y, &next_x, &next_y)) {
            last_x = x;
            last_y = y;
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // ����2: �߼��赲���ԣ��к��ڣ�
    if (game_phase >= 1 && player->your_status <= 0 && !is_timeout()) {
        if (expert_block_opponent(player, x, y, &next_x, &next_y)) {
            if (is_extremely_safe(player, next_x, next_y, 1)) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // ����3: ר�Ҽ������ռ�
    int star_target_x, star_target_y;
    if (find_best_star_expert(player, x, y, &star_target_x, &star_target_y) && !is_timeout()) {
        // Ŀ������ϵͳ
        if (star_target_x == target_x && star_target_y == target_y) {
            target_lock_count++;
        } else {
            target_lock_count = 0;
            target_x = star_target_x;
            target_y = star_target_y;
        }
        
        if (astar_pathfind(player, x, y, star_target_x, star_target_y, &next_x, &next_y)) {
            if (is_extremely_safe(player, next_x, next_y, 1) || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // ����4: ��ȫ�ƶ� - ר�Ұ�
    if (safe_move_expert(player, x, y, &next_x, &next_y)) {
        last_x = x;
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����5: �����ƶ�������ֶΣ�
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            // �����Ͱ�ȫҪ��
            int min_dist = INF;
            for (int i = 0; i < 2; i++) {
                int dist = abs(nx - player->ghost_posx[i]) + abs(ny - player->ghost_posy[i]);
                min_dist = min(min_dist, dist);
            }
            
            int required = (player->your_score < EXTREME_LOW_THRESHOLD) ? 1 : 2;
            if (min_dist >= required || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {nx, ny};
                return ret;
            }
        }
    }
    
    // ���ձ�����ԭ�ز���
    last_x = x;
    last_y = y;
    struct Point ret = {x, y};
    return ret;
} 


//�Ѳ��ԣ���������