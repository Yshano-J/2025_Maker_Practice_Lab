#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// ��ͼ���ೣ��
#define SMALL_MAP_THRESHOLD 100    // С��ͼ��С��ֵ��10x10��
#define LARGE_MAP_THRESHOLD 900    // ���ͼ��С��ֵ��30x30��

// ǿ��״̬ʱ�䳣��
#define POWER_TIME_ABUNDANT 8      // ǿ��ʱ�������ֵ
#define POWER_TIME_MODERATE 3      // ǿ��ʱ��������ֵ

// ��ȫ���볣��
#define DANGER_DISTANCE_IMMEDIATE 1  // ����Σ�վ���
#define DANGER_DISTANCE_CLOSE 2      // ������Σ��
#define SAFE_DISTANCE_PREFERRED 3    // ƫ�ð�ȫ����

// ���������ϸ�İ�ȫ���볣��
#define EMERGENCY_DISTANCE 1          // �������վ���
#define CRITICAL_DISTANCE 2           // �ٽ�Σ�վ���
#define SAFE_DISTANCE_MIN 3           // ��С��ȫ����
#define SAFE_DISTANCE_PREFERRED_NEW 4 // �µ�ƫ�ð�ȫ����

// ��������
#define GHOST_HUNT_BASE_SCORE 300.0f    // ���׷������������������
#define NORMAL_STAR_VALUE 15            // ��ͨ���Ǽ�ֵ��������
#define POWER_STAR_VALUE 50             // ǿ�����Ǽ�ֵ�����������

// Ȩ�س���
#define DISTANCE_WEIGHT 50              // ����Ȩ��
#define SAFETY_WEIGHT 100              // ��ȫȨ��
#define STAR_WEIGHT 200                // ����Ȩ��
#define GHOST_AVOID_WEIGHT 150         // ������Ȩ��
#define OPPONENT_INTERFERENCE_WEIGHT 80 // ���ָ���Ȩ��

// �������ܳ���
#define MAX_ASTAR_ITERATIONS_SMALL 300  // С��ͼA*����������
#define MAX_ASTAR_ITERATIONS_LARGE 100  // ���ͼA*����������
#define MAX_BFS_SEARCHES_SMALL 200      // С��ͼBFS���������
#define MAX_BFS_SEARCHES_LARGE 1000     // ���ͼBFS���������

// Ŀ���ȶ��Գ���
#define TARGET_STABILITY_MIN_COUNT 3    // Ŀ���ȶ�����С����
#define TARGET_MAX_DISTANCE 8           // Ŀ�����������

// ǿ��̽������
#define FORCE_EXPLORATION_INTERVAL 50   // ǿ��̽���������
#define FORCE_EXPLORATION_DISTANCE 10   // ǿ��̽����С����

// �������յ�����
#define GHOST_DISTANCE_FAR 20          // �������Զ��ֵ
#define GHOST_DISTANCE_NEAR 5          // �����������ֵ
#define POWER_STAR_LURE_BONUS 80       // �������յ�����
#define NORMAL_STAR_LURE_BONUS 40      // ��ͨ���յ�����
#define POWER_STAR_URGENT_BONUS 200    // ������������ǽ���

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;  // ��¼��һ��λ��
static int step_count = 0;  // ����������

// ʱ����Ʊ���
static clock_t start_time;  // ��ʼʱ��

// ��������·����ʷ��¼�����ڼ��ͱ����𵴡�
#define PATH_HISTORY_SIZE 5  // ��¼���5��λ��
static int path_history_x[PATH_HISTORY_SIZE];
static int path_history_y[PATH_HISTORY_SIZE];
static int path_history_head = 0;  // ѭ��������ͷָ��
static int path_history_count = 0;  // ��ǰ��¼����ʷ����

// �ƻ��߳����ؼ�¼������ǿ��״̬�µ�����׷����
static int ghost_spawn_x[2] = {-1, -1};  // �ƻ��߳�ʼX����
static int ghost_spawn_y[2] = {-1, -1};  // �ƻ��߳�ʼY����
static int spawn_recorded = 0;           // �Ƿ��Ѽ�¼������

// AI�ȶ��Կ��Ʊ���
static int current_target_x = -1, current_target_y = -1;  // ��ǰĿ������
static int target_stable_count = 0;  // Ŀ���ȶ�������
static int last_score = 0;  // ��һ�غϷ���
static int ghost_last_x[2] = {-1, -1}, ghost_last_y[2] = {-1, -1};  // �����һ��λ��
static int danger_level = 0;  // ��ǰΣ�յȼ� 0-��ȫ 1-�е� 2-Σ��
static int consecutive_safe_moves = 0;  // ������ȫ�ƶ�����
static int power_star_eaten_count = 0;  // �ѳԳ���������

// ���ǻ���ṹ�������ظ�������ͼ
struct StarInfo {
    int x, y;
    char type;  // 'o' �� 'O'
    int valid;  // ������������Ƿ���Ч�������������¡�
};

static struct StarInfo star_cache[MAXN * MAXN];
static struct StarInfo power_star_cache[MAXN * MAXN];
static int star_cache_size = 0;
static int power_star_cache_size = 0;
static int cache_last_update_step = 0;  // �����������������²�����

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// A* �ڵ�ṹ
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

//���Ż������Ƶĳ�ʼ��������
void init(struct Player *player) {
    // ��ʼ��ʱ�����
    start_time = clock();
    
    memset(star_eaten, 0, sizeof(star_eaten));
    memset(ghost_last_x, -1, sizeof(ghost_last_x));
    memset(ghost_last_y, -1, sizeof(ghost_last_y));
    
    // ����AI״̬����
    current_target_x = -1;
    current_target_y = -1;
    target_stable_count = 0;
    danger_level = 0;
    consecutive_safe_moves = 0;
    power_star_eaten_count = 0;
    last_score = 0;
    
    // �����ƻ��߳����ؼ�¼
    ghost_spawn_x[0] = ghost_spawn_x[1] = -1;
    ghost_spawn_y[0] = ghost_spawn_y[1] = -1;
    spawn_recorded = 0;
    
    last_x = -1;
    last_y = -1;
    step_count = 0;
    
    // ����������ʼ��·����ʷ��¼��
    memset(path_history_x, -1, sizeof(path_history_x));
    memset(path_history_y, -1, sizeof(path_history_y));
    path_history_head = 0;
    path_history_count = 0;
    
    // ���Ż������ƻ����ʼ����
    star_cache_size = 0;
    power_star_cache_size = 0;
    memset(power_star_cache, 0, sizeof(power_star_cache));
    memset(star_cache, 0, sizeof(star_cache));
    cache_last_update_step = 0;
    
    // ��������A*�㷨��س�ʼ����
    memset(nodes, 0, sizeof(nodes));
    memset(open_list, 0, sizeof(open_list));
    open_size = 0;
    
    // ����������Ϸ״̬���á�
    // ȷ�����о�̬����������ȷ����
    if (player) {
        // ���������Ϣ�����ض���ʼ��
        int map_size = player->row_cnt * player->col_cnt;
        
        // ���ݵ�ͼ��С����ĳЩ����
        if (map_size <= SMALL_MAP_THRESHOLD) {
            // С��ͼ�����ʼ��
        } else if (map_size >= LARGE_MAP_THRESHOLD) {
            // ���ͼ�����ʼ��
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

// ��������·����ʷ��¼��������
void add_to_path_history(int x, int y) {
    path_history_x[path_history_head] = x;
    path_history_y[path_history_head] = y;
    path_history_head = (path_history_head + 1) % PATH_HISTORY_SIZE;
    
    if (path_history_count < PATH_HISTORY_SIZE) {
        path_history_count++;
    }
}

// ���㵽�ƻ��ߵľ���
int distance_to_ghost(struct Player *player, int x, int y) {
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_dist) min_dist = dist;
    }
    return min_dist;
}

//����������ǿ��·���𵴼�⺯����
int detect_path_oscillation(int current_x, int current_y) {
    if (path_history_count < 3) {
        return 0;  // ��ʷ��¼̫�٣��޷������
    }
    
    // ���2��ѭ����A��B��A��B��
    if (path_history_count >= 4) {
        int idx1 = (path_history_head - 2 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        int idx2 = (path_history_head - 4 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        
        if (path_history_x[idx1] == current_x && path_history_y[idx1] == current_y &&
            path_history_x[idx2] == current_x && path_history_y[idx2] == current_y) {
            return 2;  // ��⵽2��ѭ��
        }
    }
    
    // ���3��ѭ����A��B��C��A��B��C��
    if (path_history_count >= 6) {
        int idx1 = (path_history_head - 3 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        
        if (path_history_x[idx1] == current_x && path_history_y[idx1] == current_y) {
            return 3;  // ��⵽3��ѭ��
        }
    }
    
    // �����������4��ѭ����A��B��C��D��A��B��C��D����
    if (path_history_count >= 8) {
        int idx1 = (path_history_head - 4 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        int idx2 = (path_history_head - 8 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
        
        if (path_history_x[idx1] == current_x && path_history_y[idx1] == current_y &&
            path_history_x[idx2] == current_x && path_history_y[idx2] == current_y) {
            return 4;  // ��⵽4��ѭ��
        }
    }
    
    // �����������������ģʽ��A��B��A��B��A����
    if (path_history_count >= 5) {
        int back_forth_count = 0;
        for (int i = 1; i < path_history_count - 1; i++) {
            int idx_prev = (path_history_head - i - 1 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
            int idx_curr = (path_history_head - i + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
            int idx_next = (path_history_head - i + 1 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
            
            // ����Ƿ������ƶ�
            if (path_history_x[idx_prev] == path_history_x[idx_next] &&
                path_history_y[idx_prev] == path_history_y[idx_next] &&
                !(path_history_x[idx_curr] == path_history_x[idx_prev] &&
                  path_history_y[idx_curr] == path_history_y[idx_prev])) {
                back_forth_count++;
            }
        }
        
        if (back_forth_count >= 2) {
            return 5;  // ��⵽������ģʽ
        }
    }
    
    // ����������������𵴣���С�����ڷ����ƶ�����
    if (path_history_count >= PATH_HISTORY_SIZE) {
        // ������ʷλ�õı߽��
        int min_x = current_x, max_x = current_x;
        int min_y = current_y, max_y = current_y;
        
        for (int i = 0; i < path_history_count; i++) {
            if (path_history_x[i] < min_x) min_x = path_history_x[i];
            if (path_history_x[i] > max_x) max_x = path_history_x[i];
            if (path_history_y[i] < min_y) min_y = path_history_y[i];
            if (path_history_y[i] > max_y) max_y = path_history_y[i];
        }
        
        // �����һ��С�����ڷ����ƶ�
        int area_width = max_x - min_x + 1;
        int area_height = max_y - min_y + 1;
        
        if (area_width <= 3 && area_height <= 3) {
            return 6;  // ��⵽������
        }
    }
    
    // ���̾����ظ�����
    int visit_count = 0;
    for (int i = 0; i < path_history_count; i++) {
        if (path_history_x[i] == current_x && path_history_y[i] == current_y) {
            visit_count++;
        }
    }
    
    if (visit_count >= 2) {
        return 1;  // ��⵽�ظ�����
    }
    
    return 0;  // δ��⵽��
}

//����������ǿ�Ĵ�����ģʽ������
int break_oscillation_pattern(struct Player *player, int current_x, int current_y, int *next_x, int *next_y) {
    // ����1������ѡ��ֱ�����ƶ��������ǰ��Ҫ��ˮƽ�ƶ���
    int horizontal_moves = 0, vertical_moves = 0;
    
    // ����������ƶ�����
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
    
    // ȷ�����ȷ������ˮƽ�ƶ��࣬���ȴ�ֱ����֮��Ȼ
    int priority_dirs[4];
    if (horizontal_moves > vertical_moves) {
        // ���ȴ�ֱ�ƶ�
        priority_dirs[0] = 0;  // ��
        priority_dirs[1] = 2;  // ��
        priority_dirs[2] = 1;  // ��
        priority_dirs[3] = 3;  // ��
    } else {
        // ����ˮƽ�ƶ�
        priority_dirs[0] = 1;  // ��
        priority_dirs[1] = 3;  // ��
        priority_dirs[2] = 0;  // ��
        priority_dirs[3] = 2;  // ��
    }
    
    // ������������2A�����ܷ���ѡ�񣬿��ǰ�ȫ�Ժ�Ŀ�굼��
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
            
            // ������λ���Ƿ�����ʷ��¼��
            int in_history = 0;
            int history_recency = 0;
            
            for (int j = 0; j < path_history_count; j++) {
                if (path_history_x[j] == nx && path_history_y[j] == ny) {
                    in_history = 1;
                    history_recency = path_history_count - j;  // Խ������ʷ��¼recencyԽ��
                    break;
                }
            }
            
            // ���������ʷ��¼�У�����ӷ�
            if (!in_history) {
                score += 500.0f;
            } else {
                // ����ʷ��¼�У�����ʱ��������
                score += history_recency * 10.0f;
            }
            
            // ����������ȫ��������
            int ghost_dist = distance_to_ghost(player, nx, ny);
            score += ghost_dist * 15.0f;  // ������ԽԶԽ��
            
            // ��������Ŀ�굼����������
            // Ѱ����������ǣ��������ǵķ�����轱��
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
                score += 100.0f / (min_star_dist + 1);  // ��������Խ��Խ��
            }
            
            // ����������������ͬ��
            int exit_count = 0;
            for (int dd = 0; dd < 4; dd++) {
                int check_x = nx + dx[dd];
                int check_y = ny + dy[dd];
                if (is_valid(player, check_x, check_y)) {
                    exit_count++;
                }
            }
            score += exit_count * 20.0f;  // ��·Խ��Խ��
            
            // ���������߽罱����
            if (nx == 0 || nx == player->row_cnt - 1 || ny == 0 || ny == player->col_cnt - 1) {
                score += 50.0f;  // �߽�λ�������ڴ�����
            }
            
            candidates[candidate_count].dir = d;
            candidates[candidate_count].x = nx;
            candidates[candidate_count].y = ny;
            candidates[candidate_count].score = score;
            candidate_count++;
        }
    }
    
    // �����ѡλ��
    for (int i = 0; i < candidate_count; i++) {
        for (int j = i + 1; j < candidate_count; j++) {
            if (candidates[j].score > candidates[i].score) {
                CandidateMove temp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = temp;
            }
        }
    }
    
    // ѡ����Ѻ�ѡ
    if (candidate_count > 0) {
        *next_x = candidates[0].x;
        *next_y = candidates[0].y;
        return 1;
    }
    
    // ����3���������λ�ö�����ʷ�У�ѡ����Զ����ʷλ��
    int best_dir = -1;
    int max_history_distance = -1;
    
    for (int i = 0; i < 4; i++) {
        int d = priority_dirs[i];
        int nx = current_x + dx[d];
        int ny = current_y + dy[d];
        
        if (is_valid(player, nx, ny)) {
            // �����λ������ʷ�е���Զ����
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
    
    return 0;  // �޷�������
}

// ����������ǿ�İ�ȫ�ƶ�ѡ�������𵴼�⡿
int choose_safe_direction_with_oscillation_check(struct Player *player, int x, int y, int *next_x, int *next_y) {
    // ���ȼ���Ƿ�����
    int oscillation_type = detect_path_oscillation(x, y);
    
    if (oscillation_type > 0) {
        // ��⵽�𵴣����Դ���
        if (break_oscillation_pattern(player, x, y, next_x, next_y)) {
            return 1;
        }
    }
    
    // ���û���𵴻��޷����ƣ�ʹ��ԭ�еİ�ȫ�ƶ�ѡ��
    return choose_safe_direction(player, x, y, next_x, next_y);
}

// ��ȡ����λ�úͷ�����ȷ�����ַ���Ϊ������
int get_opponent_info(struct Player *player, int *opponent_x, int *opponent_y, int *opponent_score) {
    // ��֤�������
    if (!player || !opponent_x || !opponent_y || !opponent_score) {
        return 0;  // ������Ч
    }
    
    // �������Ƿ�����Ҵ��
    if (player->opponent_status == -1) {
        return 0;  // ����������
    }
    
    // ��֤����λ���Ƿ���Ч
    if (player->opponent_posx < 0 || player->opponent_posx >= player->row_cnt ||
        player->opponent_posy < 0 || player->opponent_posy >= player->col_cnt) {
        return 0;  // ����λ����Ч
    }
    
    // ��ȡ���ֻ�����Ϣ
    *opponent_x = player->opponent_posx;
    *opponent_y = player->opponent_posy;
    *opponent_score = player->opponent_score;
    
    // ��֤���ַ����Ƿ�Ϊ������ֻ�����ֵĶ��ֲ�ֵ��׷����
    if (*opponent_score <= 0) {
        return 0;  // ���ַ�������������ֵ��׷��
    }
    return 1;  // �ɹ���ȡ������Ϣ
}

// ���������ǵķ�ɱ��ֵ�����ǻ��ǿ��������棩
float evaluate_power_star_counter_attack_value(struct Player *player, int sx, int sy, int power_x, int power_y) {
    if (player->your_status > 0) return 0.0;  // �Ѿ�ǿ������������ֵ
    
    float total_value = 0.0;
    
    // ���㵽�����ǵľ���
    int dist_to_power = abs(sx - power_x) + abs(sy - power_y);
    
    // ����ÿ������׷����ֵ
    for (int i = 0; i < 2; i++) {
        int ghost_x = player->ghost_posx[i];
        int ghost_y = player->ghost_posy[i];
        
        // ��굽�����ǵľ���
        int ghost_to_power_dist = abs(ghost_x - power_x) + abs(ghost_y - power_y);
        
        // ���ǵ����ľ��루���ǿ����
        int power_to_ghost_dist = abs(power_x - ghost_x) + abs(power_y - ghost_y);
        
        // �ؼ��жϣ�����������ڹ�굽��ǰ��ó����ǣ�����ǿ��ʱ���㹻׷��
        int time_advantage = ghost_to_power_dist - dist_to_power;  // ���ǵ�ʱ������
        
        if (time_advantage >= 1) {  // ������1�غϵ�ʱ������
            // Ԥ��ǿ��ʱ�䣨ͨ�������Ǹ�10�غ�ǿ����
            int estimated_power_time = 10;
            
            // ���ǿ��ʱ���㹻׷�����
            if (estimated_power_time > power_to_ghost_dist + 2) {
                float ghost_value = GHOST_HUNT_BASE_SCORE;
                
                // ����Խ������ֵԽ��
                float distance_factor = 1.0 / (power_to_ghost_dist + 1);
                ghost_value *= distance_factor;
                
                // ʱ������Խ�󣬼�ֵԽ��
                if (time_advantage >= 3) {
                    ghost_value *= 1.5;  // ���ʱ������
                } else if (time_advantage >= 2) {
                    ghost_value *= 1.2;  // �ʶ�ʱ������
                }
                
                // ���������ͬ�м�ֵ���ߣ�������׷����
                int ghost_escape_routes = 0;
                for (int d = 0; d < 4; d++) {
                    int check_x = ghost_x + dx[d];
                    int check_y = ghost_y + dy[d];
                    if (is_valid(player, check_x, check_y)) {
                        ghost_escape_routes++;
                    }
                }
                
                if (ghost_escape_routes <= 2) {
                    ghost_value *= 1.3;  // ���������ͬ��������׷��
                }
                
                total_value += ghost_value;
            }
        }
    }
    
    // ˫��׷�������������ͬʱ׷���������
    if (total_value > GHOST_HUNT_BASE_SCORE * 1.5) {
        total_value += 100.0;  // ˫ɱ����
    }
    
    return total_value;
}

// ����������Դ�������� - �������Ŀ��ĳ����ǡ�
float evaluate_resource_blocking(struct Player *player, int our_x, int our_y, int power_star_x, int power_star_y) {
    int opponent_x, opponent_y, opponent_score;
    
    // ��ȡ������Ϣ
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return 0.0;  // �޶�����Ϣ���޷�ִ�з���
    }
    
    int our_dist = abs(our_x - power_star_x) + abs(our_y - power_star_y);
    int opponent_dist = abs(opponent_x - power_star_x) + abs(opponent_y - power_star_y);
    
    // ����������ֵ
    float blocking_value = 0.0;
    
    // ������ֱ����Ǹ��ӽ������ǣ����Ƿ���
    if (opponent_dist < our_dist) {
        int distance_disadvantage = our_dist - opponent_dist;
        
        // ������ֵ��������Ƴɷ���
        if (distance_disadvantage <= 3) {
            blocking_value = 200.0 - distance_disadvantage * 50.0;  // ��������ԽС��������ֵԽ��
            
            // ���ַ���Խ�ߣ�������ֵԽ��
            if (opponent_score > player->your_score) {
                blocking_value += 100.0;  // ��������ʱ����������Ҫ
            } else if (opponent_score > player->your_score * 0.8) {
                blocking_value += 50.0;   // ���ֽӽ�ʱ���ʶȷ���
            }
            
            // ������λ�õ���Ҫ��
            if (power_star_x <= 2 || power_star_x >= player->row_cnt - 3 || 
                power_star_y <= 2 || power_star_y >= player->col_cnt - 3) {
                blocking_value += 30.0;  // ��Ե�����Ǹ���Ҫ
            }
            
            // ��Ϸ���ڷ�������Ҫ
            if (step_count > 100) {
                blocking_value += 40.0;  // ������Դϡȱ����������Ҫ
            }
        }
    }
    
    return blocking_value;
}

// ��������·���赲���� - ����խͨ���赲���֡�
float evaluate_path_blocking(struct Player *player, int our_x, int our_y, int move_x, int move_y) {
    int opponent_x, opponent_y, opponent_score;
    
    // ��ȡ������Ϣ
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return 0.0;  // �޶�����Ϣ���޷�ִ���赲
    }
    
    // Ԥ����ֵ��ƶ�����
    int pred_opponent_x, pred_opponent_y;
    predict_opponent_movement(player, opponent_x, opponent_y, &pred_opponent_x, &pred_opponent_y);
    
    // ����ƶ�λ���Ƿ��ڶ��ֿ��ܵ�·����
    float blocking_value = 0.0;
    
    // ������ֵ������ƶ�λ�õľ���
    int opponent_to_move_dist = abs(opponent_x - move_x) + abs(move_y - opponent_y);
    
    // ֻ���ڶ�����ԽϽ�ʱ�ſ����赲
    if (opponent_to_move_dist <= 5) {
        // ������λ���Ƿ�����խͨ��
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
        
        // ��խͨ����3��ǽ �� 2��ǽ���γ����ȣ�
        if (adjacent_walls >= 2) {
            narrow_passage_score = (adjacent_walls - 1) * 30;  // ǽԽ�࣬ͨ��Խխ
        }
        
        // ����Ƿ��ڶ��ֵ��ƶ�·����
        int on_opponent_path = 0;
        
        // �򵥼�飺������ǵ��ƶ�λ���ڶ��ֵ�ǰλ�ú�Ԥ��λ��֮��
        if ((move_x == opponent_x && abs(move_y - opponent_y) <= 2) ||
            (move_y == opponent_y && abs(move_x - opponent_x) <= 2)) {
            on_opponent_path = 1;
        }
        
        // �����ڶ���ǰ��������ǵ�·����
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
        
        // ��鳬����
        for (int k = 0; k < power_star_cache_size; k++) {
            int dist = abs(opponent_x - power_star_cache[k].x) + abs(opponent_y - power_star_cache[k].y);
            if (dist < min_star_dist) {
                min_star_dist = dist;
                nearest_star_x = power_star_cache[k].x;
                nearest_star_y = power_star_cache[k].y;
            }
        }
        
        // ����ҵ��˶��ֵ�Ŀ�����ǣ�����Ƿ���·����
        if (nearest_star_x != -1) {
            // �򻯵�·����飺�����Ƿ��ڶ��ֵ����ǵĴ���·����
            int opponent_to_star_x = nearest_star_x - opponent_x;
            int opponent_to_star_y = nearest_star_y - opponent_y;
            int opponent_to_move_x = move_x - opponent_x;
            int opponent_to_move_y = move_y - opponent_y;
            
            // ����ƶ���������ֵ����ǵķ�������
            if ((opponent_to_star_x * opponent_to_move_x >= 0 && opponent_to_star_y * opponent_to_move_y >= 0) &&
                (abs(opponent_to_move_x) + abs(opponent_to_move_y) <= abs(opponent_to_star_x) + abs(opponent_to_star_y))) {
                on_opponent_path = 1;
            }
        }
        
        if (on_opponent_path && narrow_passage_score > 0) {
            blocking_value = narrow_passage_score + 50.0;  // �����赲��ֵ
            
            // ���ַ���Խ�ߣ��赲��ֵԽ��
            if (opponent_score > player->your_score) {
                blocking_value += 60.0;  // ��������ʱ���赲����Ҫ
            }
            
            // ������Ǵ��ڰ�ȫ״̬���赲��ֵ����
            int our_safety = assess_danger_level(player, move_x, move_y);
            if (our_safety == 0) {
                blocking_value += 40.0;  // ���ǰ�ȫʱ�����Ը������赲
            }
        }
    }
    
    return blocking_value;
}

// ������������������� - �����������֡�
float evaluate_ghost_guidance(struct Player *player, int our_x, int our_y, int move_x, int move_y) {
    int opponent_x, opponent_y, opponent_score;
    
    // ��ȡ������Ϣ
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return 0.0;  // �޶�����Ϣ���޷�ִ������
    }
    
    // ֻ����������԰�ȫʱ�ſ����������
    int our_safety = assess_danger_level(player, move_x, move_y);
    if (our_safety >= 2) {
        return 0.0;  // ���ǲ���ȫ����ִ������
    }
    
    float guidance_value = 0.0;
    
    // ����ƶ��Ƿ�������Ǹ��ӽ����֣��Ӷ�������꣩
    int current_dist_to_opponent = abs(our_x - opponent_x) + abs(our_y - opponent_y);
    int new_dist_to_opponent = abs(move_x - opponent_x) + abs(move_y - opponent_y);
    
    // ֻ�е����ֲ�̫Զʱ�ſ�������
    if (current_dist_to_opponent <= 8 && new_dist_to_opponent < current_dist_to_opponent) {
        // �����λ�ú����ǵ����λ��
        for (int i = 0; i < 2; i++) {
            int ghost_x = player->ghost_posx[i];
            int ghost_y = player->ghost_posy[i];
            
            int ghost_to_us_dist = abs(ghost_x - move_x) + abs(ghost_y - move_y);
            int ghost_to_opponent_dist = abs(ghost_x - opponent_x) + abs(ghost_y - opponent_y);
            
            // �����������Ǳ�����ָ���������಻�󣬿�������
            if (ghost_to_us_dist <= ghost_to_opponent_dist + 2 && ghost_to_us_dist <= 5) {
                // ��������ƶ����Ƿ���ù����ӽ�����
                int new_ghost_to_us = abs(ghost_x - move_x) + abs(ghost_y - move_y);
                int original_ghost_to_us = abs(ghost_x - our_x) + abs(ghost_y - our_y);
                
                // ��������ƶ�����ַ��򣬹��׷������ʱ����ӽ�����
                if (new_dist_to_opponent < current_dist_to_opponent) {
                    guidance_value += 40.0;  // ����������ֵ
                    
                    // ���ַ���Խ�ߣ�������ֵԽ��
                    if (opponent_score > player->your_score) {
                        guidance_value += 50.0;  // ��������ʱ����������Ҫ
                    }
                    
                    // ������ִ������Σ�յ�λ�ã�������ֵ����
                    int opponent_danger = assess_danger_level(player, opponent_x, opponent_y);
                    if (opponent_danger >= 1) {
                        guidance_value += 30.0;  // �����Ѿ���Σ�գ���������Ч
                    }
                    
                    // ���Խ�ӽ�������Ч��Խ��
                    if (ghost_to_us_dist <= 3) {
                        guidance_value += 20.0;  // ���ܽ�����������Ч
                    }
                }
            }
        }
    }
    
    return guidance_value;
}

// ���������ۺ϶��ָ��Ų���������
float evaluate_opponent_interference(struct Player *player, int our_x, int our_y, int move_x, int move_y) {
    float total_interference_value = 0.0;
    
    // 1. ��Դ��������
    build_star_cache(player);
    for (int k = 0; k < power_star_cache_size; k++) {
        float blocking_value = evaluate_resource_blocking(player, our_x, our_y, 
                                                         power_star_cache[k].x, power_star_cache[k].y);
        
        // �������ƶ������Ǹ��ӽ���Ҫ�����ĳ�����
        int current_dist = abs(our_x - power_star_cache[k].x) + abs(our_y - power_star_cache[k].y);
        int new_dist = abs(move_x - power_star_cache[k].x) + abs(move_y - power_star_cache[k].y);
        
        if (new_dist < current_dist && blocking_value > 0) {
            total_interference_value += blocking_value * OPPONENT_INTERFERENCE_WEIGHT / 100.0;  // ʹ��ͳһȨ�س���
        }
    }
    
    // 2. ·���赲����
    float path_blocking_value = evaluate_path_blocking(player, our_x, our_y, move_x, move_y);
    total_interference_value += path_blocking_value * OPPONENT_INTERFERENCE_WEIGHT / 150.0;  // ʹ��ͳһȨ�س���
    
    // 3. �����������
    float ghost_guidance_value = evaluate_ghost_guidance(player, our_x, our_y, move_x, move_y);
    total_interference_value += ghost_guidance_value * OPPONENT_INTERFERENCE_WEIGHT / 120.0;  // ʹ��ͳһȨ�س���
    
    return total_interference_value;
}

// �Ż����ƻ����ƶ�Ԥ�⺯������ǿ������·��Ԥ�⡢ǽ�ڼ�⡢�ಽԤ�⡿
void predict_ghost_movement(struct Player *player, int ghost_idx, int *pred_x, int *pred_y) {
    // �߽���
    if (ghost_idx < 0 || ghost_idx >= 2) {
        *pred_x = player->your_posx;
        *pred_y = player->your_posy;
        return;
    }
    
    int gx = player->ghost_posx[ghost_idx];
    int gy = player->ghost_posy[ghost_idx];
    int px = player->your_posx;
    int py = player->your_posy;
    
    // �򻯣�����ǿ��״̬�Ļ���Ԥ��
    int best_dir = -1;
    int best_score = -1000;
    
            for (int d = 0; d < 4; d++) {
                int next_x = gx + dx[d];
                int next_y = gy + dy[d];
                
        if (!is_valid(player, next_x, next_y)) continue;
        
        int score = 0;
        
        // ����ǿ��״̬�ķ���ƫ��
        if (player->your_status == 0) {
            // ��ǿ��״̬�����׷�����
            int dist_to_player = abs(next_x - px) + abs(next_y - py);
            score += 1000 / (dist_to_player + 1);
        } else {
            // ǿ��״̬�����Զ�����
            int dist_to_player = abs(next_x - px) + abs(next_y - py);
            score += dist_to_player * 20;
        }
        
        // ��ʷ����һ����
                    if (ghost_last_x[ghost_idx] != -1 && ghost_last_y[ghost_idx] != -1) {
                        int hist_dx = gx - ghost_last_x[ghost_idx];
                        int hist_dy = gy - ghost_last_y[ghost_idx];
                        
                        if (dx[d] == hist_dx && dy[d] == hist_dy) {
                score += 50;  // ����һ���Խ���
            } else if (dx[d] == -hist_dx && dy[d] == -hist_dy) {
                score -= 30;  // �����ͷ
            }
        }
        
        // ��·��������
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

// �Ľ��İ�ȫ�ƶ�����ѡ�����������е�ͼ��С��
int choose_safe_direction(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_dir = -1;
    float max_safety = -1000.0f;
    
    // ���㵱ǰΣ�յȼ�
    int current_danger = assess_danger_level(player, x, y);
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    // ��������ģʽ������������1��ǿ��ѡ���ȫ�ķ���
    if (min_ghost_dist <= EMERGENCY_DISTANCE) {
        float emergency_safety = -1000.0f;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (!is_valid(player, nx, ny)) continue;
            
            int new_ghost_dist = distance_to_ghost(player, nx, ny);
            float safety = new_ghost_dist * 10.0f; // ����ģʽ�¾���Ȩ�ؼ���
            
            // �����������Ƿ�Զ�����й��
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
            
            // ����ͬ�ͷ�
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
    
    // ������ȫ�ƶ�ģʽ
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        float safety_score = 0.0f;
        
        // 1. �������밲ȫ��ֵ��Ȩ�أ�50% - ����Ȩ�أ�
        int new_ghost_dist = distance_to_ghost(player, nx, ny);
        safety_score += new_ghost_dist * 0.5f;
        
        // 2. ����ƶ�Ԥ�⣨Ȩ�أ�40% - �������Ȩ�أ�
        for (int i = 0; i < 2; i++) {
            int pred_x, pred_y;
            predict_ghost_movement(player, i, &pred_x, &pred_y);
            int pred_dist = abs(nx - pred_x) + abs(ny - pred_y);
            safety_score += pred_dist * 0.2f; // ÿ�����20%Ȩ��
            
            // ���Ԥ��λ�÷ǳ���������ͷ�
            if (pred_dist <= 1) {
                safety_score -= 20.0f;
            } else if (pred_dist <= 2) {
                safety_score -= 10.0f;
            }
        }
        
        // 3. �����ͷ��Ȩ�أ�5%��
        if (nx == last_x && ny == last_y) {
            safety_score -= 0.05f;
        }
        
        // 4. ����ͬ������Ȩ�أ�15%��
        int exit_routes = 0;
        for (int dd = 0; dd < 4; dd++) {
            int check_x = nx + dx[dd], check_y = ny + dy[dd];
            if (is_valid(player, check_x, check_y)) exit_routes++;
        }
        
        if (exit_routes <= 1) {
            safety_score -= 0.15f;
            
            // ����ͬ�еĳ����Ƿ�ɱ����������ǿ��״̬��
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
        
        // 5. �����ռ���ȫ�ԣ�Ȩ�أ�5%��
        char cell = player->mat[nx][ny];
        if (cell == 'o' || cell == 'O') {
            int star_danger = assess_danger_level(player, nx, ny);
            if (star_danger < 2) {
                safety_score += 0.05f;
            } else {
                safety_score -= 0.1f; // Σ�����ǳͷ�
            }
        }
        
        // 6. ��Ȧ���Ȳ��ԣ�Ȩ�أ�10%��
        int center_x = player->row_cnt / 2;
        int center_y = player->col_cnt / 2;
        int dist_from_center = abs(nx - center_x) + abs(ny - center_y);
        int max_dist_from_center = (player->row_cnt + player->col_cnt) / 2;
        
        float outer_ring_bonus = (float)dist_from_center / max_dist_from_center;
        safety_score += outer_ring_bonus * 0.1f;
        
        // 7. ��������ǰΣ�յȼ�Ӱ��
        if (current_danger >= 2) {
            // ��Σ��ʱ��ֻ�зǳ���ȫ�ķ���ſ���
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

// ����׷���ض����ļ�ֵ���Ż���·�����桢ʱ����������ز��ԡ�
float evaluate_ghost_hunt(struct Player *player, int sx, int sy, int ghost_idx) {
    if (player->your_status <= 0) return -1000.0f;
    
    int ghost_x = player->ghost_posx[ghost_idx];
    int ghost_y = player->ghost_posy[ghost_idx];
    int distance = abs(sx - ghost_x) + abs(sy - ghost_y);
    
    float score = 1.0f;  // ������ֵ1.0
    
    // 1. �������ӣ�Ȩ�أ�40%��
    score += 1.0f / (distance + 1) * 0.4f;
    
    // 2. ǿ��ʱ�����Ȩ�أ�30%��
    if (player->your_status > 8) {
        score += 0.3f;  // ʱ�����
    } else if (player->your_status > 3) {
        score += 0.2f;  // ʱ������
    } else {
        score += 0.1f;  // ʱ�����
    }
    
    // 3. �������ƣ�Ȩ�أ�20%��
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int check_x = ghost_x + dx[d], check_y = ghost_y + dy[d];
        if (is_valid(player, check_x, check_y)) escape_routes++;
    }
    
    if (escape_routes <= 2) {
        score += 0.2f;  // ���������ͬ
    } else if (escape_routes == 3) {
        score += 0.1f;  // ���ѡ������
    }
    
    // 4. Ԥ���ƶ���Ȩ�أ�10%��
    int pred_x, pred_y;
    predict_ghost_movement(player, ghost_idx, &pred_x, &pred_y);
    int pred_distance = abs(sx - pred_x) + abs(sy - pred_y);
    
    if (pred_distance < distance) {
        score += 0.1f;  // ����ڿ���
    } else if (pred_distance > distance) {
        score -= 0.05f;  // �����Զ��
    }
    
    return score;
}
}

// ����׷�����ֵļ�ֵ
float evaluate_opponent_hunt(struct Player *player, int sx, int sy, int opponent_x, int opponent_y, int opponent_score) {
    if (player->your_status <= 0) return -1000.0;  // û��ǿ��״̬����׷��
    
    // �ؼ���֤��ȷ�����ַ���Ϊ����
    if (opponent_score <= 0) {
        return -1000.0;  // ���ַ���Ϊ0��������ֵ��׷��
    }
    
    int distance = abs(sx - opponent_x) + abs(sy - opponent_y);
    
    // ������ֵ�����ַ�����һ�룬������ͱ���
    float score = (float)opponent_score * 0.5;
    
    // ȷ�����׷����ֵ�����ֶ���������һ����ֵ��
    if (score < 50.0) {
        score = 50.0;  // ���50�ּ�ֵ
    }
    
    // �������ӣ�����Խ����ֵԽ��
    float distance_factor = 1.0 / (distance + 1);
    score *= distance_factor * 2.0;
    
    // ǿ��ʱ�����ȷ�����㹻ʱ��׷������
    if (player->your_status <= distance) {
        score -= score * 0.8;  // ʱ�䲻�����������
    } else if (player->your_status <= distance + 2) {
        score -= score * 0.4;  // ʱ����ţ��ʶȽ���
    }
    
    // ���ֿ���Ҳ���ƶ�������׷���Ѷ�
    if (distance > 3) {
        score -= 50.0;  // ����̫Զ��������������
    }
    
    // �������ƣ����������ͬ�еĶ��ָ�����׷��
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int check_x = opponent_x + dx[d];
        int check_y = opponent_y + dy[d];
        if (is_valid(player, check_x, check_y)) {
            escape_routes++;
        }
    }
    
    if (escape_routes <= 2) {
        score += 60.0;  // ����������ͬ�����
    } else if (escape_routes == 3) {
        score += 30.0;  // ����ѡ������
    }
    
    // ������������������й�꣬׷�����ֿ����з���
    for (int i = 0; i < 2; i++) {
        int ghost_to_opponent = abs(opponent_x - player->ghost_posx[i]) + abs(opponent_y - player->ghost_posy[i]);
        if (ghost_to_opponent <= 2) {
            score -= 40.0;  // ���ָ����й�꣬׷�����մ�
        }
    }
    
    // ���Ż�����϶�����Ϣ�Ķ�̬׷����ֵ�жϡ�
    // ���ݶ��ַ������ҷ���������Թ�ϵ��̬����
    int score_gap = opponent_score - player->your_score;
    
1    // ����׷����ֵ�����ַ���Խ�ߣ�׷����ֵԽ��
    if (opponent_score >= 5000) {
        score += 150.0;  // �߷ֶ��֣�׷����ֵ����
        if (score_gap > 500) {
            score += 100.0;  // �������Ⱥܶ�ʱ��׷������Ҫ
        }
    } else if (opponent_score >= 2500) {
        score += 100.0;  // �и߷ֶ���
        if (score_gap > 300) {
            score += 80.0;   // ��������ʱ��׷����Ҫ
        }
    } else if (opponent_score >= 1000) {
        score += 60.0;   // �еȷֶ���
        if (score_gap > 200) {
            score += 50.0;   // ��������ʱ���ʶ�����
        }
    } else if (opponent_score >= 500) {
        score += 30.0;   // �ͷֶ��֣������м�ֵ
    }
    
    // ������������ǿ��״̬���ǡ�
    // ������ִ���ǿ��״̬��׷����ֵ���ͣ���Ϊ���ֿ��ܷ�ɱ��
    if (player->opponent_status > 0) {
        score -= 50.0;  // ����ǿ��״̬��׷����������
        if (player->opponent_status > 5) {
            score -= 50.0;  // ����ǿ��ʱ�䳤�����ո���
        }
    }
    
    return score;
}

// ������Ԥ�������һ����Ϊ�����ڲ����ĵ��Ķ���Ԥ��ģ�飩
void predict_opponent_movement(struct Player *player, int opponent_x, int opponent_y, int *pred_x, int *pred_y) {
    // Ĭ�Ϸ��ص�ǰλ��
    *pred_x = opponent_x;
    *pred_y = opponent_y;
    
    // �߽���
    if (opponent_x < 0 || opponent_x >= player->row_cnt || 
        opponent_y < 0 || opponent_y >= player->col_cnt) {
        return;
    }
    
    // �򵥵Ķ�����ΪԤ�⣺������ֻ�������������ƶ�
    build_star_cache(player);
    
    int best_dir = -1;
    float best_value = -1000.0f;
    
    for (int d = 0; d < 4; d++) {
        int next_x = opponent_x + dx[d];
        int next_y = opponent_y + dy[d];
        
        if (is_valid(player, next_x, next_y)) {
            float move_value = 0;
            
            // �����ƶ������ǵļ�ֵ
            char cell = player->mat[next_x][next_y];
            if (cell == 'o') {
                move_value += 100.0f;  // ��ͨ����
            } else if (cell == 'O') {
                move_value += 200.0f;  // ��������
            }
            
            // Ѱ��������ǵķ�����
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
            
            // ����Խ����ֵԽ��
            if (min_star_dist < INF) {
                move_value += (20.0f / (min_star_dist + 1));
            }
            
            // �����ƶ���Σ��λ�ã��ӽ����ǵ�λ�ã�
            int dist_to_us = abs(next_x - player->your_posx) + abs(next_y - player->your_posy);
            if (player->your_status > 0 && dist_to_us <= 3) {
                move_value -= 150.0f;  // ����ǿ��״̬ʱ�����ֻ�ܿ�
            }
            
            if (move_value > best_value) {
                best_value = move_value;
                best_dir = d;
            }
        }
    }
    
    // ����Ԥ��λ��
    if (best_dir != -1) {
        *pred_x = opponent_x + dx[best_dir];
        *pred_y = opponent_y + dy[best_dir];
    }
}

// ѡ����Ѷ���׷��Ŀ��
float choose_best_opponent_target(struct Player *player, int sx, int sy, int *target_x, int *target_y) {
    if (player->your_status <= 0) return -1000.0;  // û��ǿ��״̬
    
    int opponent_x, opponent_y, opponent_score;
    
    // ��ȡ������Ϣ
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return -1000.0;  // �޷���ȡ������Ϣ
    }
    
    // ˫����֤��ȷ�����ַ���Ϊ����
    if (opponent_score <= 0) {
        return -1000.0;  // ���ַ���Ϊ0��������׷��
    }
    
    // ����׷��������ֵļ�ֵ
    float opponent_value = evaluate_opponent_hunt(player, sx, sy, opponent_x, opponent_y, opponent_score);
    
    // ������֤��ȷ��������ֵҲΪ��
    if (opponent_value > 0) {
        *target_x = opponent_x;
        *target_y = opponent_y;
        return opponent_value;
    }
    
    return -1000.0;  // ׷�����ֲ�����
}

// ���Ż�����ǿ�Ĺ��׷��Ѱ·������·����������������
// bfs_ghost_huntĿ���ж�����ǰλ��Ϊ����Ԥ��λ��
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
// bfs_ghost_hunt·����ֵ������+���ǽ���+ǿ��״̬
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
        // ���ݵ�һ��
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

// bfs_opponent_huntĿ���ж�����ǰλ��Ϊ���ֻ���Ԥ��λ��
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
        // ���ݵ�һ��
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

// ר�ŵĶ���׷��Ѱ·��Ԥ������ƶ���
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
    
    // ����׷��ģʽ�����Ƕ��ֿ��ܵ��ƶ�
    int max_search = 150;  // ����׷���������
    int searched = 0;
    
    while (head < tail && searched < max_search) {
        int x = qx[head], y = qy[head]; 
        head++;
        searched++;
        
        // ������ֵ�ǰλ��
        if (x == target_x && y == target_y) {
            // ���ݵ�һ��
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
        
        // ���Ƕ��ֿ����ƶ�����λ�ã�Ԥ�����أ�
        for (int pred_d = 0; pred_d < 4; pred_d++) {
            int pred_x = target_x + dx[pred_d];
            int pred_y = target_y + dy[pred_d];
            
            if (is_valid(player, pred_x, pred_y) && x == pred_x && y == pred_y) {
                // ������ֿ��ܵ��ƶ�λ��
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
        
        // �����������ӽ����ֵķ���
        int directions[4] = {0, 1, 2, 3};
        
        // ��������ֵ�Զ��������
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
    
    return 0;  // ׷��ʧ��
}

// ����������в����ֹ���ֻ�ó���������
float assess_opponent_threat(struct Player *player, int our_x, int our_y) {
    int opponent_x, opponent_y, opponent_score;
    
    // ��ȡ������Ϣ
    if (!get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score)) {
        return 0.0;  // �޶�����Ϣ������в
    }
    
    float total_threat = 0.0;
    
    // ������г����ǣ�����������в
    for (int k = 0; k < power_star_cache_size; k++) {
        int power_x = power_star_cache[k].x;
        int power_y = power_star_cache[k].y;
        
        int our_dist_to_power = abs(our_x - power_x) + abs(our_y - power_y);
        int opponent_dist_to_power = abs(opponent_x - power_x) + abs(opponent_y - power_y);
        
        // ���־��볬���Ǹ�����������в��
        if (opponent_dist_to_power < our_dist_to_power) {
            float threat_level = 0.0;
            
            // ��������Խ����вԽ��
            int distance_advantage = our_dist_to_power - opponent_dist_to_power;
            threat_level += distance_advantage * 50.0;  // ÿ������50����в
            
            // ���ַ���Խ�ߣ���вԽ�󣨲��ܱ����˶��棩
            if (opponent_score > player->your_score) {
                threat_level += 100.0;  // ��������ʱ��в+100
            } else if (opponent_score > player->your_score * 0.8) {
                threat_level += 50.0;   // ���ֽӽ�ʱ��в+50
            }
            
            // ���Ǵ���ǿ��״̬ʱ�����ֻ�ó����Ǹ�Σ��
            if (player->your_status > 0) {
                threat_level += 150.0;  // ǿ��״̬�¶�����в+150
                
                // ǿ����������ʱ��Σ��
                if (player->your_status <= POWER_TIME_MODERATE) {
                    threat_level += 100.0;  // ��������ʱ+100
                }
            }
            
            total_threat += threat_level;
        }
    }
    
    return total_threat;
}

//���Ż�����ͼ��С����Ӧ�İ�ȫ����������
int assess_danger_level(struct Player *player, int x, int y) {
    int min_ghost_dist = distance_to_ghost(player, x, y);
    int map_size = player->row_cnt * player->col_cnt;
    
    // ����Ԥ�����
    int predicted_min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int pred_x, pred_y;
        predict_ghost_movement(player, i, &pred_x, &pred_y);
        int pred_dist = abs(x - pred_x) + abs(y - pred_y);
        if (pred_dist < predicted_min_dist) predicted_min_dist = pred_dist;
    }
    
    // ʹ�ø��ϸ�İ�ȫ����
    int safe_distance, danger_distance_immediate, danger_distance_close;
    
    if (map_size <= SMALL_MAP_THRESHOLD) {
        // С��ͼ������Ҫ������ɣ�����Ҫ�ϸ�
        safe_distance = 2;  // ��1������2
        danger_distance_immediate = 1;
        danger_distance_close = 2;  // ��1������2
    } else if (map_size >= LARGE_MAP_THRESHOLD) {
        // ���ͼ������Ҫ����ϸ�
        safe_distance = 4;  // ��3������4
        danger_distance_immediate = 2;
        danger_distance_close = 3;
    } else {
        // �еȵ�ͼ����׼����Ҫ��
        safe_distance = 3;  // ��2������3
        danger_distance_immediate = 1;
        danger_distance_close = 2;
    }
    
    // ��������·������
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
    
    // �ۺϵ�ǰ��Ԥ���������Σ�յȼ�
    int overall_dist = (min_ghost_dist + predicted_min_dist) / 2;
    
    // ��������������·��Խ�٣�Σ�յȼ�Խ��
    int terrain_penalty = 0;
    if (escape_routes <= 1) {
        terrain_penalty = 1;
    } else if (escape_routes == 2) {
        terrain_penalty = 0;
    }
    
    // ǿ��״̬����
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
        // ��ǿ��״̬Σ������ - �����ϸ�
        int base_danger = 0;
        if (overall_dist <= danger_distance_immediate) {
            base_danger = 2;  // ��Σ��
        } else if (overall_dist <= danger_distance_close) {
            base_danger = 1;  // �е�Σ��
        } else if (overall_dist <= safe_distance) {
            base_danger = (escape_routes <= 1) ? 1 : 0;
        } else {
            base_danger = 0;  // ��ȫ
        }
        
        // Ӧ�õ�������
        int final_danger = base_danger + terrain_penalty;
        
        // С��ͼ���⴦�� - ��΢���ɵ���Ҫ����
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

// ���Ŀ���Ƿ�Ӧ�ñ����ȶ�
int should_keep_target(struct Player *player, int x, int y) {
    // û�е�ǰĿ��
    if (current_target_x == -1 || current_target_y == -1) {
        return 0;
    }
    
    // Ŀ�������Ѿ����Ե�
    if (!is_star(player, current_target_x, current_target_y)) {
        return 0;
    }
    
    // ���������Ȳ��ԣ������ǰĿ�겻�ǳ����ǣ�����Ƿ��и��ŵĳ�����
    char current_target_type = player->mat[current_target_x][current_target_y];
    if (current_target_type != 'O' && player->your_status <= 0) {
        // ����Ƿ��а�ȫ�ĳ�����ֵ���л�
        for (int k = 0; k < power_star_cache_size; k++) {
            int super_danger = assess_danger_level(player, power_star_cache[k].x, power_star_cache[k].y);
            int super_distance = abs(x - power_star_cache[k].x) + abs(y - power_star_cache[k].y);
            int current_distance = abs(x - current_target_x) + abs(y - current_target_y);
            
            // ��������ǰ�ȫ�Ҳ��ȵ�ǰĿ��Զ̫�࣬Ӧ���л�
            if (super_danger < 2 && super_distance <= current_distance + 3) {
                return 0;  // ������ǰĿ�꣬�л���������
            }
        }
    }
    
    // Ŀ���ȶ�ʱ��̫�̣���������
    if (target_stable_count < TARGET_STABILITY_MIN_COUNT) {
        return 1;
    }
    
    // ����Ŀ��̫Զ�����Կ��ǻ�Ŀ��
    int dist_to_target = abs(x - current_target_x) + abs(y - current_target_y);
    if (dist_to_target > TARGET_MAX_DISTANCE) {
        return 0;
    }
    
    // Ŀ��λ�ñ��Σ��
    int target_danger = assess_danger_level(player, current_target_x, current_target_y);
    if (target_danger >= 2 && player->your_status <= 0) {
        return 0;
    }
    
    return 1;  // ���ֵ�ǰĿ��
}

// ���Ż��������������ǻ��棬�����ظ�������ͼ��
void build_star_cache(struct Player *player) {
    int map_size = player->row_cnt * player->col_cnt;
    
    // ������������С��ͼ���״ι�����ʹ��ȫ�����¡�
    if (map_size <= SMALL_MAP_THRESHOLD || cache_last_update_step == 0) {
        star_cache_size = 0;
        power_star_cache_size = 0;
        
        for (int i = 0; i < player->row_cnt; i++) {
            for (int j = 0; j < player->col_cnt; j++) {
                if (is_star(player, i, j)) {
                    if (player->mat[i][j] == 'O') {
                        // ǿ������
                        power_star_cache[power_star_cache_size].x = i;
                        power_star_cache[power_star_cache_size].y = j;
                        power_star_cache[power_star_cache_size].type = 'O';
                        power_star_cache[power_star_cache_size].valid = 1;
                        power_star_cache_size++;
                    } else {
                        // ��ͨ����
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
    
    // �����������ڴ��ͼ��ʹ���������¡�
    // ��黺���е������Ƿ���Ȼ��Ч
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
    
    // �����������ڣ�ÿ10��������һ�η�Χ��飬Ѱ�������ǡ�
    if (step_count - cache_last_update_step >= 10) {
        int check_radius = 5;  // ��������Χ5����Χ�ڵ�������
        int px = player->your_posx;
        int py = player->your_posy;
        
        for (int i = px - check_radius; i <= px + check_radius; i++) {
            for (int j = py - check_radius; j <= py + check_radius; j++) {
                if (i >= 0 && i < player->row_cnt && j >= 0 && j < player->col_cnt) {
                    if (is_star(player, i, j)) {
                        // ����Ƿ��Ѿ��ڻ�����
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

// ����AI״̬��Ϣ
void update_ai_state(struct Player *player) {
    // ��¼�ƻ��߳����أ���һ������ʱ��¼�����⵽�ƻ��߻ص���ʼλ�ã�
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
        if (step_count > 5) spawn_recorded = 1;  // ǰ5����¼���
    }
    
    // ����ƻ����Ƿ�ص������أ����Ե���������
    for (int i = 0; i < 2; i++) {
        if (ghost_spawn_x[i] != -1 && ghost_spawn_y[i] != -1) {
            int current_x = player->ghost_posx[i];
            int current_y = player->ghost_posy[i];
            
            // ����ƻ��߻ص������أ������Ǹձ�����
            if (current_x == ghost_spawn_x[i] && current_y == ghost_spawn_y[i]) {
                // ������������������߼��������Ǹ��ƻ��߸������������ж����޵��ڣ�
            }
        }
    }
    
    // ���¹����ʷλ�ã���ӱ߽���
    for (int i = 0; i < 2; i++) {
        if (player->ghost_posx[i] >= 0 && player->ghost_posx[i] < player->row_cnt &&
            player->ghost_posy[i] >= 0 && player->ghost_posy[i] < player->col_cnt) {
            ghost_last_x[i] = player->ghost_posx[i];
            ghost_last_y[i] = player->ghost_posy[i];
        }
    }
    
    // ����Σ�յȼ�
    danger_level = assess_danger_level(player, player->your_posx, player->your_posy);
    
    // ����������ȫ�ƶ�����
    if (danger_level == 0) {
        consecutive_safe_moves++;
    } else {
        consecutive_safe_moves = 0;
    }
    
    // ����Ƿ�Ե�������
    if (player->mat[player->your_posx][player->your_posy] == 'O') {
        power_star_eaten_count++;
    }
    
    // ���·���
    last_score = player->your_score;
    
    // �������ǻ���
    build_star_cache(player);
}

// ��ͼ��С����Ӧ����������
int heuristic(struct Player *player, int x1, int y1, int x2, int y2) {
    int manhattan = abs(x1 - x2) + abs(y1 - y2);
    int map_size = player->row_cnt * player->col_cnt;
    
    // С��ͼ(<= 15x15)����ȷ����
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
    
    // ���ͼ(> 15x15)���򻯼��㣬���ⳬʱ
    if (manhattan <= 3) return manhattan;
    
    // ���ͼֻ���ǰ3��
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
    
    // ���ͼ��Ȧ���ȣ����Ŀ������Ȧ����΢��������ʽ�ɱ�
    if (map_size >= LARGE_MAP_THRESHOLD) {
        int center_x = player->row_cnt / 2;
        int center_y = player->col_cnt / 2;
        int target_dist_from_center = abs(x2 - center_x) + abs(y2 - center_y);
        int max_dist_from_center = (player->row_cnt + player->col_cnt) / 2;
        
        // ���Ŀ������Ȧ����������ʽ�ɱ���������Ȧ·����
        if (target_dist_from_center > max_dist_from_center * 0.7) {
            base_cost = (int)(base_cost * 0.9);  // ��ȦĿ�꽵��10%�ɱ�
        }
    }
    
    return base_cost;
}

// 10x10��ͼר�ã����λ���Ƿ�ȫ����׼��ǿ��״̬ʱ�����ԳԵ����--��ȫ
int is_position_safe(struct Player *player, int x, int y) {
    // ������Ч�Լ��
    if (!is_valid(player, x, y)) return 0;
    
    // ����С��ͼ��ʹ�÷ſ�İ�ȫ���
    if (player->row_cnt * player->col_cnt <= SMALL_MAP_THRESHOLD) {
        // ���Ż���ǿ��״̬ʱ�����ص��жϡ�
        if (player->your_status > 0) {
            // ǿ��״̬ʱ���������ص��������ԳԵ���꣩
            // ����Ҫȷ��ǿ��ʱ���㹻
    for (int i = 0; i < 2; i++) {
                // ǿ��״̬ʱ���������ص��������ԳԵ���꣩
                if (x == player->ghost_posx[i] && y == player->ghost_posy[i]) {
                    // ���ǿ��ʱ���Ƿ��㹻
                    if (player->your_status >= 1) {
                        return 1;  // ǿ��״̬�¿���ֱ���ص��Ե����
                    } else {
                        return 0;  // ǿ��ʱ�䲻�㣬�ص�Σ��
                    }
                }
            }
            return 1;  // ǿ��״̬������λ�ö���ȫ
        }
        
        // ��ǿ��״̬��ֻ�о���1���ڲž���Σ��
        for (int i = 0; i < 2; i++) {
            int ghost_x = player->ghost_posx[i];
            int ghost_y = player->ghost_posy[i];
            int dist = abs(x - ghost_x) + abs(y - ghost_y);
            
            // ֻ�н���λ�ò�����Σ��
            if (dist <= 1) {
                return 0;
            }
        }
        
        return 1;  // ����2�����϶���Ϊ��ȫ
    }
    
    return 1;  // ���ͼĬ�ϰ�ȫ
}

// ��ͼ��С����Ӧ����������
// ������������Χ����ͨ�ǣ������յ���꣩
float evaluate_power_star_surrounding_stars(struct Player *player, int sx, int sy, int power_star_x, int power_star_y) {
    float best_score = -1000.0f;
    int best_star_x = -1, best_star_y = -1;
    
    // ��鳬������Χ3x3�����ڵ���ͨ��
    for (int dx = -3; dx <= 3; dx++) {
        for (int dy = -3; dy <= 3; dy++) {
            int check_x = power_star_x + dx;
            int check_y = power_star_y + dy;
            
            if (is_valid(player, check_x, check_y) && 
                player->mat[check_x][check_y] == 'o' && 
                !star_eaten[check_x][check_y]) {
                
                float score = NORMAL_STAR_VALUE;
                int distance = heuristic(player, sx, sy, check_x, check_y);
                
                // ��������
                score -= distance * 8;
                
                // �յ����������볬����Խ������ͨ��Խ�м�ֵ
                int dist_to_power = abs(check_x - power_star_x) + abs(check_y - power_star_y);
                score += NORMAL_STAR_LURE_BONUS / (dist_to_power + 1);
                
                // ��ȫ�Լ��
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
    
    // ��������
    score -= distance * 8;
    
    // ���ͼ������Ȧ����
    if (map_size >= LARGE_MAP_THRESHOLD) {
        int margin = 2;
        if (star_x < margin || star_x >= player->row_cnt - margin || star_y < margin || star_y >= player->col_cnt - margin) {
            score += 60; // ��Ȧ����
        }
    }
    // С��ͼ���Ƚ�����
    else if (map_size <= SMALL_MAP_THRESHOLD) {
        score += 30 / (distance + 1);
    }
    
    // ��ȫ�Լ򵥳ͷ�
    int danger = assess_danger_level(player, star_x, star_y);
    if (danger >= 2) score -= 50;
    else if (danger == 1) score -= 15;
    
    return score;
}

// �Ż���BFS��С��ͼ�ʹ��ͼͨ�ã�
int bfs_backup(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int userdata[2] = {sx, sy};
    int map_size = player->row_cnt * player->col_cnt;
    int max_search = (map_size <= SMALL_MAP_THRESHOLD) ? map_size * 2 :
                     ((map_size > LARGE_MAP_THRESHOLD) ? MAX_BFS_SEARCHES_LARGE : 500);
    // ���ݵ���һ��
    int tx, ty;
    if (general_bfs(player, sx, sy, &tx, &ty, max_search, bfs_backup_goal, NULL, userdata)) {
        // ���ݵ�һ��
        int px = tx, py = ty;
        int prex[MAXN][MAXN], prey[MAXN][MAXN]; // ֻ���ڻ���
        // ������һ��BFS�Ի�ø��ڵ�
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

// bfs_backupĿ���ж�����ǰλ��������
static int bfs_backup_goal(struct Player *player, int x, int y, void *userdata) {
    int sx = ((int*)userdata)[0];
    int sy = ((int*)userdata)[1];
    if (is_star(player, x, y) && !(x == sx && y == sy)) return 1;
    return 0;
}

// ��С�Ѳ���
void heap_push(int node_idx) {
    if (open_size >= MAXN * MAXN) return;  // ��ֹ�����
    
    open_list[open_size++] = node_idx;
    int pos = open_size - 1;
    
    while (pos > 0) {
        int parent = (pos - 1) / 2;
        int curr_x = node_idx / MAXN, curr_y = node_idx % MAXN;
        int parent_x = open_list[parent] / MAXN, parent_y = open_list[parent] % MAXN;
        
        // �߽���
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
        
        // �߽���
        if (pos_x >= MAXN || pos_y >= MAXN || left_x >= MAXN || left_y >= MAXN) break;
        
        if (nodes[left_x][left_y].f_cost < nodes[pos_x][pos_y].f_cost) {
            smallest = left;
        }
        
        if (right < open_size) {
            int right_x = open_list[right] / MAXN, right_y = open_list[right] % MAXN;
            int smallest_x = open_list[smallest] / MAXN, smallest_y = open_list[smallest] % MAXN;
            
            // �߽���
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

// ���ͼ�Ż���A*������������Χ��
int astar_limited(struct Player *player, int sx, int sy, int tx, int ty, int *next_x, int *next_y) {
    int map_size = player->row_cnt * player->col_cnt;
    int max_distance = abs(sx - tx) + abs(sy - ty);
    
    // ���ͼ�Ҿ����Զʱ��ֱ����BFS
    if (map_size > LARGE_MAP_THRESHOLD && max_distance > (FORCE_EXPLORATION_DISTANCE + 5)) {
        return 0;  // �õ�����ʹ��BFS
    }
    
    // ��ʼ���ڵ㣨ֻ��ʼ����Ҫ����
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
    
    // ���
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
            // ����·���ҵ���һ��
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
        
        // ̽���ھ�
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (!is_valid(player, nx, ny) || nodes[nx][ny].in_closed) continue;
            
            // ȷ���ڵ��ڳ�ʼ����Χ��
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
    
    return 0;  // ��ʱ���Ҳ���·��
}

//���Ż�����ǿ�ĳ����ط�������������
float evaluate_spawn_point_approach(struct Player *player, int sx, int sy, int ghost_idx) {
    if (player->your_status <= 0) return -1000.0;  // ��ǿ��״̬�޷�׷��
    if (ghost_idx < 0 || ghost_idx >= 2) return -1000.0;  // ��Ч�������
    if (ghost_spawn_x[ghost_idx] == -1 || ghost_spawn_y[ghost_idx] == -1) return -1000.0;  // δ��¼������
    
    int spawn_x = ghost_spawn_x[ghost_idx];
    int spawn_y = ghost_spawn_y[ghost_idx];
    int ghost_x = player->ghost_posx[ghost_idx];
    int ghost_y = player->ghost_posy[ghost_idx];
    
    // ��������ڳ����ظ�����ֱ��׷������
    int ghost_spawn_dist = abs(ghost_x - spawn_x) + abs(ghost_y - spawn_y);
    if (ghost_spawn_dist <= 2) {
        return -500.0;  // ����ڳ����ظ�����ֱ��׷��
    }
    
    // �������ǵ������صľ���
    int our_spawn_dist = abs(sx - spawn_x) + abs(sy - spawn_y);
    
    // �����굽�����صľ���
    int ghost_to_spawn_dist = abs(ghost_x - spawn_x) + abs(ghost_y - spawn_y);
    
    // �������ԣ��������������ؽ�Զ�����ǿ�����������ؿ����ȴ�
    float score = 0.0;
    
    // ���Ż����������ĳ����ز��ԡ�
    if (player->your_status > 5) {  // ���Ż�����6����5��
        // ����������غ�Զ�������л����ڳ����صȴ�
        if (ghost_to_spawn_dist >= 4) {  // ���Ż�����5����4��
            score = GHOST_HUNT_BASE_SCORE * 0.8;  // ���Ż�����70%������80%��
            
            // �������ӣ�����Խ�ӽ������ؼ�ֵԽ��
            float distance_factor = 1.0 / (our_spawn_dist + 1);
            score *= distance_factor;
            
            // ���Ż��������ɵ�ʱ�����ӵ�����
            if (player->your_status >= 10) {  // ���Ż�����12����10��
                score *= 1.5;  // ���Ż�����1.4������1.5��ʱ��ǳ�����
            } else if (player->your_status >= 7) {  // ���Ż�����9����7��
                score *= 1.3;  // ���Ż�����1.2������1.3��ʱ�����
            } else {
                score *= 1.1;  // ���Ż�����1.0������1.1���е�ʱ��ҲҪ����
            }
            
            // ������������������ƶ������Ӽ�ֵ
            int pred_x, pred_y;
            predict_ghost_movement(player, ghost_idx, &pred_x, &pred_y);
            int pred_spawn_dist = abs(pred_x - spawn_x) + abs(pred_y - spawn_y);
            if (pred_spawn_dist < ghost_to_spawn_dist) {
                score *= 1.4;  // ���Ż�����1.3������1.4�����������������ƶ�
            }
            
            // �������������ظ����ķ���������
            if (our_spawn_dist <= 3) {
                score += 120.0;  // ���Ż�����100.0������120.0���ӽ������صķ�������
                
                // ����������������λ�ý�����
                if (our_spawn_dist <= 1) {
                    score += 180.0;  // ���Ż�����150.0������180.0���ڳ����ظ�����������������
                }
            }
            
            // ����������������������
            // ����������Χ�ĵ��Σ�����ͬ��ͨ���ڸ�����
            int spawn_exits = 0;
            for (int d = 0; d < 4; d++) {
                int check_x = spawn_x + dx[d];
                int check_y = spawn_y + dy[d];
                if (is_valid(player, check_x, check_y)) {
                    spawn_exits++;
                }
            }
            
            if (spawn_exits <= 2) {
                score += 100.0;  // ���Ż�����80.0������100.0��������������ͬ��ͨ���������׷���
            } else if (spawn_exits == 3) {
                score += 50.0;  // ���Ż�����40.0������50.0��������ѡ�����ޣ���һ������
            }
        }
    } else if (player->your_status > 3) {  // ���Ż���ʱ��Ҫ�󱣳ֲ��䡿
        // ���Ż���ʱ������ʱ�ķ������ԡ�
        if (our_spawn_dist <= 5 && ghost_to_spawn_dist >= 5) {  // ���Ż�����4/6������5/5��
            score = GHOST_HUNT_BASE_SCORE * 0.6;  // ���Ż�����50%������60%��
            score *= (1.0 / (our_spawn_dist + 1));
            
            // ��������ʱ������ʱ�ķ���������
            if (our_spawn_dist <= 2) {
                score += 80.0;  // ���Ż�����60.0������80.0��ʱ�����޵�λ�úõķ�������
            }
        }
    } else if (player->your_status > 1) {  // �����������ʱ�̵ķ������ԡ�
        // �����������ʱ�̵�ƴ��������
        if (our_spawn_dist <= 2 && ghost_to_spawn_dist >= 2) {
            score = GHOST_HUNT_BASE_SCORE * 0.1;  // ���ʱ�̵ķ������м�ֵ
            score *= (1.0 / (our_spawn_dist + 1));
            
            // ���ʱ�̵ķ�������
            if (our_spawn_dist <= 1) {
                score += 60.0;  // ���ʱ�̵���������
            }
        }
    }
    
    // ������������Эͬ����������
    // �����һ�����Ҳ����������ƶ������ӷ�����ֵ
    int other_ghost = (ghost_idx == 0) ? 1 : 0;
    int other_ghost_x = player->ghost_posx[other_ghost];
    int other_ghost_y = player->ghost_posy[other_ghost];
    int other_ghost_to_spawn = abs(other_ghost_x - spawn_x) + abs(other_ghost_y - spawn_y);
    
    if (other_ghost_to_spawn <= 6 && ghost_to_spawn_dist >= 4) {
        score += 60.0;  // ���Ż�����40.0������60.0�����ܵ�˫���������
    }
    
    // ������������ѹ���µķ������� - ��̬������ֵ��
    // ���ݵ�ͼ����Ϸ��չ��̬����������ֵ
    int target_score = 10000;  // Ŀ�����
    int low_score_threshold = target_score / 3;    // 3333�� - �������ز���
    int mid_score_threshold = target_score / 2;    // 5000�� - ��������
    int high_score_threshold = target_score * 2/3; // 6667�� - �����ӽ�Ŀ��
    
    if (player->your_score < low_score_threshold) {
        score += 80.0;  // �������ز���ʱ���������Լ�����Ҫ
    } else if (player->your_score < mid_score_threshold) {
        score += 60.0;  // ��������ʱ���������Ժ���Ҫ
    } else if (player->your_score < high_score_threshold) {
        score += 40.0;  // �����ӽ�Ŀ��ʱ���ʶ�����
    } else if (player->your_score < target_score) {
        score += 20.0;  // �ӽ�Ŀ��ʱ����΢����
    }
    
    return score;
}
    
// ѡ�����׷��Ŀ�꣨���������ز��ԣ�
int choose_best_ghost_target(struct Player *player, int sx, int sy) {
    if (player->your_status <= 0) return -1;  // û��ǿ��״̬
    
    float best_score = -1000.0;
    int best_ghost = -1;
    
    for (int i = 0; i < 2; i++) {
        // ����ֱ��׷���ļ�ֵ
        float direct_score = evaluate_ghost_hunt(player, sx, sy, i);
        
        // ���������ز��Եļ�ֵ
        float spawn_score = evaluate_spawn_point_approach(player, sx, sy, i);
        
        // ѡ�����Ų���
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

// ר�ŵĳ����ؽӽ�Ѱ·��ǿ��״̬ר�ã�
int bfs_spawn_approach(struct Player *player, int sx, int sy, int target_ghost, int *next_x, int *next_y) {
    if (target_ghost < 0 || target_ghost >= 2) return 0;
    if (ghost_spawn_x[target_ghost] == -1 || ghost_spawn_y[target_ghost] == -1) return 0;
    struct SpawnApproachGoalData data;
    data.spawn_x = ghost_spawn_x[target_ghost];
    data.spawn_y = ghost_spawn_y[target_ghost];
    int max_search = 150;
    int tx, ty;
    if (general_bfs(player, sx, sy, &tx, &ty, max_search, bfs_spawn_approach_goal, NULL, &data)) {
        // ���ݵ�һ��
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

//������������0 - ��ǿ��״̬����Զ����ԡ�
int handle_non_power_escape_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    if (player->your_status != 0) return 0;  // ֻ�����ǿ��״̬
    
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    // ��̬Σ����ֵ
    int map_size = player->row_cnt * player->col_cnt;
    int danger_threshold = (map_size <= SMALL_MAP_THRESHOLD) ? 3 : 
                          (map_size >= LARGE_MAP_THRESHOLD) ? 5 : 4;
    
    // ��������ƻ���̫������Ҫ����Զ��
    if (min_ghost_dist <= danger_threshold) {
        int current_danger = assess_danger_level(player, x, y);
        int emergency_mode = (current_danger >= 2 || min_ghost_dist <= 2);
        
        int best_escape_dir = -1;
        float max_escape_value = -1000.0f;
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny)) {
                float escape_value = 0.0f;
                
                // 1. �������밲ȫ��ֵ��Ȩ�أ�50%��
                int new_ghost_dist = distance_to_ghost(player, nx, ny);
                escape_value += new_ghost_dist * 0.5f;
                
                // 2. ����ƶ�Ԥ�⣨Ȩ�أ�30%��
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
                
                // 3. ��·��ȫ��������Ȩ�أ�15%��
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
                
                // 4. �����ռ���ȫ�ԣ�Ȩ�أ�5%��
                char cell = player->mat[nx][ny];
                if (cell == 'o' && (!emergency_mode || (safe_exit_count > 0 && new_ghost_dist >= 2))) {
                    escape_value += 0.05f;
                } else if (cell == 'O' && (!emergency_mode || (safe_exit_count > 0 && new_ghost_dist >= 2))) {
                    escape_value += 0.1f;
                }
                
                // 5. �����𵴣�Ȩ�أ�5%��
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

//������������1 - ǿ��״̬׷�����ԡ�
int handle_power_hunt_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    if (player->your_status <= 0) return 0;  // ֻ����ǿ��״̬
    
    int remaining_power_time = player->your_status;
    
    // ǿ��״̬������׷������Ĺ�꣬��Ҫ����ʱ��ɱ�
    int best_ghost = -1;
    int min_ghost_dist = INF;
    
    // ������Ĺ��
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_ghost_dist) {
            min_ghost_dist = dist;
            best_ghost = i;
        }
    }
    
    // ��������׷���ļ�ֵ
    int opponent_x, opponent_y, opponent_score;
    int should_hunt_opponent = 0;
    float opponent_hunt_value = 0;
    
    if (get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score) && opponent_score > 0) {
        int opponent_dist = abs(x - opponent_x) + abs(y - opponent_y);
        
        // ���Ż��������������Ȩ�����á�
        // ������ַ�ֵ��һ�� vs �ƻ��ߵ÷�(300��)�ıȽ�
        float opponent_half_score = opponent_score * 0.5f;
        float ghost_score = GHOST_HUNT_BASE_SCORE;  // ʹ��ͳһȨ�س���
        
        // ������ַ�ֵ��һ����ڹ��÷֣�����׷������
        if (opponent_half_score > ghost_score && remaining_power_time >= 4) {
            if (opponent_dist <= remaining_power_time - 1) {
                should_hunt_opponent = 1;
                opponent_hunt_value = opponent_half_score;
            }
        }
        // ��ʹ���ַ�ֵһ��С�ڹ��÷֣���������ַ����ܸ�Ҳֵ�ÿ���
        else if (opponent_score > 600 && remaining_power_time >= 7) {
            if (opponent_dist <= remaining_power_time - 2) {
                should_hunt_opponent = 1;
                opponent_hunt_value = opponent_half_score;
            }
        }
    }
    
    // ���Ż�����������������ȼ��жϡ�
    // ���ȼ��жϣ����ּ�ֵ vs �ƻ��߼�ֵ
    if (should_hunt_opponent && (opponent_hunt_value > GHOST_HUNT_BASE_SCORE || min_ghost_dist > 8)) {
        // ׷������
        if (bfs_opponent_hunt(player, x, y, opponent_x, opponent_y, next_x, next_y)) {
            return 1;  // �ɹ�����
        }
    }
    
    if (best_ghost >= 0) {
        // ��ǿ������������׷�����ߣ�����׷����Χ��
        int should_chase = 0;
        
        if (remaining_power_time >= 7) {
            should_chase = (min_ghost_dist <= 20);  // ��ǿ������12������20��ʱ�����ʱ�������׷����Χ
        } else if (remaining_power_time >= 4) {
            should_chase = (min_ghost_dist <= 15);  // ��ǿ������7������15��ʱ������ʱ�������׷����Χ
        } else if (remaining_power_time >= 2) {
            should_chase = (min_ghost_dist <= 10);  // ��ǿ������4������10��ʱ�����ʱ��Ҫ����׷����Χ
        } else {
            should_chase = (min_ghost_dist <= 6);   // ��ǿ������2������6�����ʱ��ҲҪ����׷��
            
            // ���3�غϵ����⴦��
            if (remaining_power_time <= 3 && remaining_power_time >= 1) {
                if (min_ghost_dist <= 2) {
                    should_chase = 1;
                }
                // ���ǹ��Ԥ��λ�õ�׷������
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
            // ����׷��·��
            if (bfs_ghost_hunt(player, x, y, best_ghost, next_x, next_y)) {
                return 1;  // �ɹ�����
            }
            
            // BFSʧ�ܣ�����ֱ�ӿ���
            int ghost_x = player->ghost_posx[best_ghost];
            int ghost_y = player->ghost_posy[best_ghost];
            int best_dir = -1;
            float best_move_value = -1000.0;
            
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (is_valid(player, nx, ny)) {
                    float move_value = 0;
                    
                    // ����׷����ֵ
                    int dist_to_ghost = abs(nx - ghost_x) + abs(ny - ghost_y);
                    int current_dist = abs(x - ghost_x) + abs(y - ghost_y);
                    if (dist_to_ghost < current_dist) {
                        move_value += 100.0;
                    }
                    
                    // ��������ǿ��״̬�½���·������Ȩ�أ�רע���׷����
                    char cell = player->mat[nx][ny];
                    if (cell == 'o') {
                        move_value += 20.0;  // ����������50.0���͵�20.0��������ͨ����Ȩ��
                    } else if (cell == 'O') {
                        move_value += 30.0;  // ����������120.0������͵�30.0��������ͳ�����Ȩ�أ�רע���׷��
                    }
                    
                    // ��ȫ�Կ���
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
                return 1;  // �ɹ�����
            }
        }
    }
    
    // ��������ǿ��״̬���ϸ����Ƴ������ռ������ڼ�������¿��ǡ�
    if (remaining_power_time <= 2) {  // ����������5������2��ֻ�����2�غ���û�й���׷ʱ�ſ��ǳ�����
        build_star_cache(player);
        
        // ��������ֻ����û�п�׷���Ĺ��ʱ�ſ��ǳ����ǡ�
        int any_ghost_reachable = 0;
        for (int i = 0; i < 2; i++) {
            int ghost_dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
            if (ghost_dist <= remaining_power_time + 1) {
                any_ghost_reachable = 1;
                break;
            }
        }
        
        // ֻ����û�й���׷��ʱ��Ѱ�ҳ�����
        if (!any_ghost_reachable) {
            int nearest_power_star_x = -1, nearest_power_star_y = -1;
            int min_power_dist = INF;
            
            for (int k = 0; k < power_star_cache_size; k++) {
                int dist = abs(x - power_star_cache[k].x) + abs(y - power_star_cache[k].y);
                if (dist < min_power_dist && dist <= remaining_power_time) {  // ���������Ƴ�+2�Ŀ���������
                    min_power_dist = dist;
                    nearest_power_star_x = power_star_cache[k].x;
                    nearest_power_star_y = power_star_cache[k].y;
                }
            }
            
            if (nearest_power_star_x != -1) {
                if (astar_limited(player, x, y, nearest_power_star_x, nearest_power_star_y, next_x, next_y)) {
                    return 1;  // �ɹ�����
                }
            }
        }
        
        // ���û�к��ʵĳ����ǣ��������ռ���ͨ����
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
                return 1;  // �ɹ�����
            }
        }
    }
    
    return 0;  // δ����
}

//������������2 - ���������ռ����ԡ�
int handle_star_collection_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    // ��ȫ��飺�����ǰ���ڸ�Σ��״̬�������������ռ�
    int current_danger = assess_danger_level(player, x, y);
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    if (current_danger >= 2 || min_ghost_dist <= CRITICAL_DISTANCE) {
        return 0; // �ø������ȼ��İ�ȫ���Դ���
    }
    
    build_star_cache(player);
    int best_star_x = -1, best_star_y = -1;
    float best_score = -1000.0;
    int map_size = player->row_cnt * player->col_cnt;
    
    // ��������ľ���
    int ghost_distance = distance_to_ghost(player, x, y);
    
    // ��ȡ������Ϣ
    int opponent_x = -1, opponent_y = -1, opponent_score = 0;
    get_opponent_info(player, &opponent_x, &opponent_y, &opponent_score);
    int opponent_distance = INF;
    if (opponent_x != -1 && opponent_y != -1) {
        opponent_distance = abs(x - opponent_x) + abs(y - opponent_y);
    }
    
    // ���ͼ�����յ�����
    if (map_size >= LARGE_MAP_THRESHOLD) {
        // ����������Զ������20������������������Χ����ͨ�����յ����
        // ��������־����������5���������ȳԳ�����
        if (ghost_distance > GHOST_DISTANCE_FAR && opponent_distance > 5) {
            float best_lure_score = -1000.0f;
            int best_lure_x = -1, best_lure_y = -1;
            
            // Ѱ�ҳ�������Χ����ͨ��
            for (int k = 0; k < power_star_cache_size; k++) {
                int power_x = power_star_cache[k].x, power_y = power_star_cache[k].y;
                
                // ��鳬������Χ3x3�����ڵ���ͨ��
                for (int dx = -3; dx <= 3; dx++) {
                    for (int dy = -3; dy <= 3; dy++) {
                        int check_x = power_x + dx;
                        int check_y = power_y + dy;
                        
                        if (is_valid(player, check_x, check_y) && 
                            player->mat[check_x][check_y] == 'o' && 
                            !star_eaten[check_x][check_y]) {
                            
                            float lure_score = NORMAL_STAR_VALUE;
                            int distance = heuristic(player, x, y, check_x, check_y);
                            
                            // ��������
                            lure_score -= distance * 8;
                            
                            // �յ����������볬����Խ������ͨ��Խ�м�ֵ
                            int dist_to_power = abs(check_x - power_x) + abs(check_y - power_y);
                            lure_score += NORMAL_STAR_LURE_BONUS / (dist_to_power + 1);
                            
                            // ��ȫ�Լ��
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
            
            // ����ҵ����ʵ��յ�Ŀ�꣬����ѡ��
            if (best_lure_x != -1 && best_lure_score > -500.0f) {
                if (astar_limited(player, x, y, best_lure_x, best_lure_y, next_x, next_y)) {
                    return 1;
                }
            }
        }
        
        // ��������������С��5��������־����������5�������ȳԵ�������
        if (ghost_distance < GHOST_DISTANCE_NEAR || opponent_distance <= 5) {
            for (int k = 0; k < power_star_cache_size; k++) {
                int star_x = power_star_cache[k].x, star_y = power_star_cache[k].y;
                float score = evaluate_star(player, x, y, star_x, star_y);
                
                // ������������ǽ���
                score += POWER_STAR_URGENT_BONUS;
                
                // ���ͼ������Ȧ
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
            
            // ����ҵ������ǣ�����ѡ��
            if (best_star_x != -1) {
                if (astar_limited(player, x, y, best_star_x, best_star_y, next_x, next_y)) {
                    return 1;
                }
            }
        }
    }
    
    // ���������ռ�����
    // ���ȿ��ǳ�����
    for (int k = 0; k < power_star_cache_size; k++) {
        int star_x = power_star_cache[k].x, star_y = power_star_cache[k].y;
        float score = evaluate_star(player, x, y, star_x, star_y);
        
        // ���ͼ������Ȧ
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
    
    // ��ͨ����
    for (int k = 0; k < star_cache_size; k++) {
        int star_x = star_cache[k].x, star_y = star_cache[k].y;
        float score = evaluate_star(player, x, y, star_x, star_y);
        
        // ���ͼ������Ȧ
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

//������������3 - ��ȫ�ƶ����ԡ�
int handle_safe_movement_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    // ��鵱ǰΣ�յȼ�
    int current_danger = assess_danger_level(player, x, y);
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    // �����ǰ���ڸ�Σ��״̬��ǿ��ʹ�ý�������
    if (current_danger >= 2 || min_ghost_dist <= EMERGENCY_DISTANCE) {
        // �������գ�ѡ���ȫ�ķ���
        int best_dir = -1;
        float max_safety = -1000.0f;
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (!is_valid(player, nx, ny)) continue;
            
            int new_ghost_dist = distance_to_ghost(player, nx, ny);
            float safety = new_ghost_dist * 10.0f;
            
            // �����Ԥ��λ��
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
            
            // ����ͬ�ͷ�
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
    
    // ������ȫ�ƶ������𵴼�飩
    return choose_safe_direction_with_oscillation_check(player, x, y, next_x, next_y);
}

//������������4 - ����̽�����ԡ�
int handle_intelligent_exploration_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_x = x, best_y = y;
    float max_score = -1000.0f;
    
    // �����ģʽ
    int oscillation_type = detect_path_oscillation(x, y);
    
    // �����⵽�𵴣����ȳ��Դ�����
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
    
    // �����ͼ����
    int center_x = player->row_cnt / 2;
    int center_y = player->col_cnt / 2;
    int map_size = player->row_cnt * player->col_cnt;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            float explore_score = 0.0f;
            
            // 1. ������ȫ��ֵ��Ȩ�أ�30%��
            int ghost_dist = distance_to_ghost(player, nx, ny);
            explore_score += ghost_dist * 0.3f;
            
            // 2. ��Ȧ���Ȳ��ԣ�Ȩ�أ�40%��
            int dist_from_center = abs(nx - center_x) + abs(ny - center_y);
            int max_dist_from_center = (player->row_cnt + player->col_cnt) / 2;
            
            // ��Ȧ��������������ԽԶ����Խ��
            float outer_ring_bonus = (float)dist_from_center / max_dist_from_center;
            explore_score += outer_ring_bonus * 0.4f;
            
            // 3. ��Ե̽��������Ȩ�أ�15%��
            int margin = (map_size >= LARGE_MAP_THRESHOLD) ? 3 : 2;
            if (nx < margin || nx >= player->row_cnt - margin || 
                ny < margin || ny >= player->col_cnt - margin) {
                explore_score += 0.15f;
            }
            
            // 4. ����̽��������Ȩ�أ�10%��
            if ((nx <= 1 || nx >= player->row_cnt - 2) && 
                (ny <= 1 || ny >= player->col_cnt - 2)) {
                explore_score += 0.1f;
            }
            
            // 5. �����𵴣�Ȩ�أ�5%��
            if (nx == last_x && ny == last_y) {
                explore_score -= 0.05f;
            }
            
            // 6. ��ʷλ�óͷ�
            for (int i = 0; i < path_history_count; i++) {
                if (path_history_x[i] == nx && path_history_y[i] == ny) {
                    int recency = path_history_count - i;
                    explore_score -= recency * 0.01f;
                }
            }
            
            // 7. �𵴴���
            if (oscillation_type >= 2) {
                // �����𵴣�ǿ��ѡ��ֱ��ˮƽ����
                int horizontal_moves = 0, vertical_moves = 0;
                for (int i = 1; i < path_history_count && i < 4; i++) {
                    int idx_curr = (path_history_head - i + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
                    int idx_prev = (path_history_head - i - 1 + PATH_HISTORY_SIZE) % PATH_HISTORY_SIZE;
                    
                    if (path_history_x[idx_curr] != path_history_x[idx_prev]) horizontal_moves++;
                    if (path_history_y[idx_curr] != path_history_y[idx_prev]) vertical_moves++;
                }
                
                if (horizontal_moves > vertical_moves) {
                    if (d == 0 || d == 2) explore_score += 0.3f;  // �����ƶ�
                } else {
                    if (d == 1 || d == 3) explore_score += 0.3f;  // �����ƶ�
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
    
    // ����ȷ������Ч�ƶ�
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
 * ���ľ��ߺ���������𵴡�ǿ��׷����·���Ż�����
 * ���ȼ���ǿ��״̬׷�� > �������ռ� > ��ͨ���ռ� > ����̽��
 */
 
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    update_ai_state(player);
    
    // ��ӵ�ǰλ�õ�·����ʷ��¼
    add_to_path_history(x, y);
    
    // ��ǵ�ǰλ�õ�����Ϊ�ѳ�
    char current_cell = player->mat[x][y];
    if ((current_cell == 'o' || current_cell == 'O') && !star_eaten[x][y]) {
        star_eaten[x][y] = 1;
    }
    
    int next_x, next_y;
    clock_t current_time;
    double elapsed_ms;
    
    // ����0��ԭ������Զ����ԣ�������ȼ���
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_non_power_escape_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; 
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����1��ǿ��״̬׷�����θ����ȼ���
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_power_hunt_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����2����ȫ�ƶ����������ȼ�����������������ͣ�
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_safe_movement_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����3�����������ռ����������ȼ���ȷ����ȫ��һ��
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_star_collection_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����4������̽��������ϣ�
    current_time = clock();
    elapsed_ms = ((double)(current_time - start_time)) * 1000 / CLOCKS_PER_SEC;
    if (elapsed_ms > 90) goto timeout_return;
    
    if (handle_intelligent_exploration_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
timeout_return:
    
    // ����fallback��ȷ������Ч�ƶ�
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            last_x = x; last_y = y;
            struct Point ret = {nx, ny};
            return ret;
        }
    }
    
    // ���ʵ��û����Ч�ƶ������ص�ǰλ��
    last_x = x; last_y = y;
    struct Point ret = {x, y};
    return ret;
}

// bfs_spawn_approachĿ���ж�����ǰλ��Ϊ��������丽��
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
        // ���ݵ�һ��
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

// ͨ��BFSĿ���ж��ص�������1��ʾ����Ŀ��
typedef int (*BFSGoalFunc)(struct Player *player, int x, int y, void *userdata);
// ͨ��BFS·����ֵ�ص�������float��Խ�����ȼ�Խ�ߣ���ΪNULL��
typedef float (*BFSValueFunc)(struct Player *player, int from_x, int from_y, int to_x, int to_y, void *userdata);

// ͨ��BFSģ��
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