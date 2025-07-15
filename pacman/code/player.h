#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// ���޸߷ֳ���
#define ULTRA_TARGET_SCORE 10000
#define EXTREME_LOW_THRESHOLD 1500    // 1500�����¼��ȼ���
#define ULTRA_LOW_THRESHOLD 3000      // 3000�����³�������
#define HIGH_PERFORMANCE_THRESHOLD 6000  // 6000�����ϱ��ֹ�����

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int consecutive_low_score = 0;

// ����ʱ���Ż� - 100ms��������
static clock_t start_time;
static const int MAX_TIME_LIMIT_MS = 90;  

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// ������·��׷��
static int elite_path_x[50], elite_path_y[50];
static int elite_path_count = 0;
static int super_target_x = -1, super_target_y = -1;
static int target_lock_duration = 0;

// ���Ԥ��
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

// ������·����¼
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

// ������·���ظ�
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

// �ಽ������ΪԤ��
void predict_ghost_moves(struct Player *player) {
    for (int ghost_id = 0; ghost_id < 2; ghost_id++) {
        int gx = player->ghost_posx[ghost_id];
        int gy = player->ghost_posy[ghost_id];
        
        ghost_predicted_x[ghost_id][0] = gx;
        ghost_predicted_y[ghost_id][0] = gy;
        
        // Ԥ��4��
        for (int step = 1; step <= 4; step++) {
            // �򻯣�������������ƶ�
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

// ���ް�ȫ��� - ���ݷ�����̬����
int is_extremely_safe(struct Player *player, int x, int y, int future_step) {
    if (!is_valid(player, x, y)) return 0;
    
    // ǿ��״̬����ȫ����
    if (player->your_status > 0) {
        if (player->your_status > 5) return 1;  // ʱ�������ȫ����
        // ��ʹʱ�䲻��ҲҪ������ֻ����ֱ����ײ
        for (int i = 0; i < 2; i++) {
            if (x == player->ghost_posx[i] && y == player->ghost_posy[i]) return 0;
        }
        return 1;
    }
    
    // ���ݷ���������ȫ����
    int required_distance;
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        required_distance = 1;  // ��������ʱֻҪ��1�����
        consecutive_low_score++;
    } else if (player->your_score < ULTRA_LOW_THRESHOLD) {
        required_distance = 2;  // �����ϵ�ʱҪ��2�����
        consecutive_low_score = max(0, consecutive_low_score - 1);
    } else {
        required_distance = 3;  // ��������ʱҪ��3�����
        consecutive_low_score = 0;
    }
    
    // �����ͷ�ʱ��һ�����Ͱ�ȫҪ��
    if (consecutive_low_score > 10) {
        required_distance = max(1, required_distance - 1);
    }
    
    // ���Ԥ��Ĺ��λ��
    for (int i = 0; i < 2; i++) {
        int step = min(future_step, 4);
        int ghost_x = ghost_predicted_x[i][step];
        int ghost_y = ghost_predicted_y[i][step];
        int dist = abs(x - ghost_x) + abs(y - ghost_y);
        if (dist < required_distance) return 0;
    }
    
    return 1;
}

// �ռ����ֺ���
float ultimate_score_move(struct Player *player, int from_x, int from_y, int to_x, int to_y) {
    if (!is_valid(player, to_x, to_y)) return -100000.0f;
    
    float score = 0.0f;
    
    // 1. �������Ǽ�ֵ - ���ݷ�����̬����
    if (is_star(player, to_x, to_y)) {
        char star_type = player->mat[to_x][to_y];
        if (star_type == 'O') {
            score += 3000.0f;  // �����ǳ��߼�ֵ
        } else {
            score += 800.0f;   // С���Ǹ߼�ֵ
        }
        
        // ��������ʱ����������Ǽ�ֵ
        if (player->your_score < EXTREME_LOW_THRESHOLD) {
            score *= 3.0f;
        } else if (player->your_score < ULTRA_LOW_THRESHOLD) {
            score *= 2.0f;
        }
    }
    
    // 2. ��ȫ�����Ż� - �����ܵķ�������
    if (!is_extremely_safe(player, to_x, to_y, 0)) {
    if (player->your_status > 0) {
            score += 500.0f;  // ǿ��״̬�¹���ð��
        } else {
            // ��������ķ��յ���
            if (player->your_score < EXTREME_LOW_THRESHOLD) {
                score -= 200.0f;  // ���ͷ�ʱ�ʶȳͷ�Σ��
            } else {
                score -= 2000.0f; // ��������ʱ���سͷ�Σ��
            }
        }
    }
    
    // 3. �������Ǿۺ϶�
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
    
    // 4. ���ָ��� - ���ǿ��
    float interference_value = 0.0f;
    if (player->opponent_score > 6000) {
        interference_value = 1000.0f;  // �߷ֶ���ǿ������
    } else if (player->opponent_score > 3000) {
        interference_value = 600.0f;
    } else {
        interference_value = 300.0f;
    }
    
    int opp_dist = abs(to_x - player->opponent_posx) + abs(to_y - player->opponent_posy);
    if (opp_dist <= 3) {
        score += interference_value / (opp_dist + 1);
    }
    
    // 5. ·����ʷ�ͷ� - ǿ����
    int path_penalty = recent_path_penalty(to_x, to_y);
    score -= path_penalty;
    
    // 6. ����������ƶ���
    int mobility = 0;
    for (int d = 0; d < 4; d++) {
        int check_x = to_x + dx[d];
        int check_y = to_y + dy[d];
        if (is_valid(player, check_x, check_y)) {
            mobility++;
        }
    }
    score += mobility * 100.0f;
    
    // 7. ����ѹ��ϵͳ
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        score += 1000.0f;  // ���ͷ���ʱ�����ӷ�
        if (nearby_stars > 0) {
            score += 2000.0f;  // ������ʱ�����ӷ�
        }
    }
    
    // 8. Ŀ������Խ���
    if (to_x == super_target_x && to_y == super_target_y) {
        score += 300.0f * target_lock_duration;
    }
    
    return score;
}

// ����A*Ѱ·
int super_astar_pathfind(struct Player *player, int start_x, int start_y, int target_x, int target_y, int *next_x, int *next_y) {
    if (is_timeout()) return 0;
    
    static int g_cost[MAXN][MAXN], f_cost[MAXN][MAXN];
    static int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    static int open_x[500], open_y[500];  // �����б�
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
    
    // ��̬��������
    int max_iterations = 150;
    if (player->your_score < EXTREME_LOW_THRESHOLD) {
        max_iterations = 200;  // �ͷ�ʱ���Ӽ�����
    }
    
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
            
            // ��ȫ��� - ������
            if (!is_extremely_safe(player, nx, ny, 1) && player->your_status == 0) {
                if (player->your_score >= ULTRA_LOW_THRESHOLD) continue;
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
    
    // ����·����ʷ
    record_elite_path(x, y);
    
    // Ԥ�����ƶ�
    predict_ghost_moves(player);
    
    // ��ǳԵ�������
    if (last_x != -1 && last_y != -1 && is_star(player, last_x, last_y)) {
        star_eaten[last_x][last_y] = 1;
    }
    
    // ����Ŀ������ - ����Χ�ͼ�ֵ����
    int best_target_x = -1, best_target_y = -1;
    float best_target_value = -1.0f;
    
    int search_radius = (player->your_score < EXTREME_LOW_THRESHOLD) ? 10 : 8;
    
    for (int i = max(0, x - search_radius); i <= min(player->row_cnt - 1, x + search_radius) && !is_timeout(); i++) {
        for (int j = max(0, y - search_radius); j <= min(player->col_cnt - 1, y + search_radius); j++) {
            if (!is_star(player, i, j)) continue;
            
            int dist = abs(i - x) + abs(j - y);
            if (dist == 0) continue;
            
            float value = 0.0f;
            
            // ���ǻ�����ֵ
            char star_type = player->mat[i][j];
            if (star_type == 'O') {
                value = 5000.0f / (dist + 1);
            } else {
                value = 1500.0f / (dist + 1);
            }
            
            // ��������ʱ������ֵ
            if (player->your_score < EXTREME_LOW_THRESHOLD) {
                value *= 4.0f;
            } else if (player->your_score < ULTRA_LOW_THRESHOLD) {
                value *= 2.5f;
            }
            
            // ��ȫ�Կ���
            if (!is_extremely_safe(player, i, j, min(3, dist/3))) {
                if (player->your_status == 0) {
                    value *= 0.3f;  // Σ��λ�ý��ͼ�ֵ
                } else {
                    value *= 1.5f;  // ǿ��״̬������ֵ
                }
            }
            
            // �ۺϽ���
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
    
    // Ŀ������ϵͳ
    if (best_target_x == super_target_x && best_target_y == super_target_y) {
        target_lock_duration++;
    } else {
        target_lock_duration = 0;
        super_target_x = best_target_x;
        super_target_y = best_target_y;
    }
    
    // ʹ��A*Ѱ·��Ŀ��
    if (best_target_x != -1) {
        int next_x, next_y;
        if (super_astar_pathfind(player, x, y, best_target_x, best_target_y, &next_x, &next_y)) {
                struct Point ret = {next_x, next_y};
                return ret;
            }
    }
    
    // A*ʧ��ʱʹ���ռ�̰�Ĳ���
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
    
    // �������İ�ȫ�ƶ�
    if (final_x == x && final_y == y) {
    for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
                // �����Ͱ�ȫҪ��
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
    
    // ���ձ���
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
