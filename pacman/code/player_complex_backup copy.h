#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

// ��������
#define GHOST_HUNT_BASE_SCORE 200.0f    // ���׷����������
#define NORMAL_STAR_VALUE 10            // ��ͨ���Ǽ�ֵ
#define POWER_STAR_VALUE 20             // ǿ�����Ǽ�ֵ

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

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;  // ��¼��һ��λ��
static int step_count = 0;  // ����������

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

// ���������ֲ�·���滮��ؽṹ��
#define REGION_SIZE 8  // �����С��8x8��
#define MAX_REGIONS 64  // �����������

struct RegionInfo {
    int center_x, center_y;  // �������ĵ�
    int star_count;          // ��������������
    int power_star_count;    // �����ڳ���������
    int danger_level;        // ����Σ�յȼ�
};

static struct RegionInfo region_map[MAX_REGIONS];
static int region_count = 0;
static int region_map_initialized = 0;

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
    
    // ���������ֲ�·���滮��ʼ����
    region_map_initialized = 0;
    region_count = 0;
    memset(region_map, 0, sizeof(region_map));
    
    // ��������A*�㷨��س�ʼ����
    memset(nodes, 0, sizeof(nodes));
    memset(open_list, 0, sizeof(open_list));
    open_size = 0;
    
    // �����������ܼ�ر�����ʼ����
    // ��������ܼ�ر������������ʼ��
    
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
    
    // ������������Ķ���״̬��Ϣ��¼��
    // ��¼���ֵ�ǿ��״̬�����ں������Ե�����
    // opponent_status > 0 ��ʾ���ִ���ǿ��״̬
    // opponent_status == 0 ��ʾ���ִ�����ͨ״̬
    // opponent_status == -1 ��ʾ���������������������飩
    
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
            total_interference_value += blocking_value * 0.3;  // Ȩ��0.3���������Ӱ����Ҫ����
        }
    }
    
    // 2. ·���赲����
    float path_blocking_value = evaluate_path_blocking(player, our_x, our_y, move_x, move_y);
    total_interference_value += path_blocking_value * 0.2;  // Ȩ��0.2
    
    // 3. �����������
    float ghost_guidance_value = evaluate_ghost_guidance(player, our_x, our_y, move_x, move_y);
    total_interference_value += ghost_guidance_value * 0.25;  // Ȩ��0.25
    
    return total_interference_value;
}

// �Ż����ƻ����ƶ�Ԥ�⺯������ǿ������·��Ԥ�⡢ǽ�ڼ�⡢�ಽԤ�⡿
void predict_ghost_movement(struct Player *player, int ghost_idx, int *pred_x, int *pred_y) {
    // �߽��飺ȷ�����������Ч
    if (ghost_idx < 0 || ghost_idx >= 2) {
        *pred_x = player->your_posx;  // Ĭ�Ϸ������λ��
        *pred_y = player->your_posy;
        return;
    }
    
    int gx = player->ghost_posx[ghost_idx];
    int gy = player->ghost_posy[ghost_idx];
    int px = player->your_posx;
    int py = player->your_posy;
    
    // ���Ż�1����ǿ��Ŀ�굼��Ԥ�⣬��������׷���߼���
    if (player->your_status == 0) {  // ֻ�ڷ�ǿ��״̬Ԥ��׷����Ϊ
        int dist_to_player = abs(gx - px) + abs(gy - py);
        
        // ����������2-8������Ԥ���������ҷ����ƶ�
        if (dist_to_player >= 2 && dist_to_player <= 8) {
            // ������ѡ�ƶ���������
            struct {
                int dir;
                int next_x, next_y;
                float score;
            } candidates[4];
            int candidate_count = 0;
            
            // �����������ƶ�����
            for (int d = 0; d < 4; d++) {
                int next_x = gx + dx[d];
                int next_y = gy + dy[d];
                
                if (is_valid(player, next_x, next_y)) {
                    candidates[candidate_count].dir = d;
                    candidates[candidate_count].next_x = next_x;
                    candidates[candidate_count].next_y = next_y;
                    
                    // �����ƶ�����ҵľ���
                    int new_dist = abs(next_x - px) + abs(next_y - py);
                    float score = 1000.0f / (new_dist + 1);  // ����Խ������Խ��
                    
                    // ���������ಽ·����ͨ�Լ�顿
                    // ���÷�������������Ƿ���ǽ���赲
                    int wall_penalty = 0;
                    int continuous_path = 0;
                    for (int step = 1; step <= 4; step++) {
                        int future_x = next_x + dx[d] * step;
                        int future_y = next_y + dy[d] * step;
                        
                        if (future_x < 0 || future_x >= player->row_cnt || 
                            future_y < 0 || future_y >= player->col_cnt ||
                            player->mat[future_x][future_y] == '#') {
                            wall_penalty += (5 - step) * 15;  // Խ����ǽ�ڳͷ�Խ��
                            break;
                        }
                        continuous_path++;
                    }
                    
                    // ������ͨ·��
                    score += continuous_path * 10;
                    
                    // �������������ȶ��Խ�����
                    // ��������������ʷ�ƶ�����һ�£����轱��
                    if (ghost_last_x[ghost_idx] != -1 && ghost_last_y[ghost_idx] != -1) {
                        int hist_dx = gx - ghost_last_x[ghost_idx];
                        int hist_dy = gy - ghost_last_y[ghost_idx];
                        
                        if (dx[d] == hist_dx && dy[d] == hist_dy) {
                            score += 80.0f;  // ����һ���Խ�������
                        }
                        
                        // ������������仯�����Լ�顿
                        // ��������Ҫ�ı䷽������ѡ��90��ת�������180�ȵ�ͷ
                        if (dx[d] != hist_dx || dy[d] != hist_dy) {
                            // ����Ƿ��ǵ�ͷ��180��ת��
                            if (dx[d] == -hist_dx && dy[d] == -hist_dy) {
                                score -= 40.0f;  // �ͷ���ͷ
                            } else {
                                score += 20.0f;  // ����90��ת��
                            }
                        }
                    }
                    
                    // ����������������ƶ����Ƶ�����Ԥ�⡿
                    // �����������ƶ���Ԥ�������һ��λ�ò���Ӧ�������Ŀ��
                    if (last_x != -1 && last_y != -1) {
                        int player_dx = px - last_x;
                        int player_dy = py - last_y;
                        
                        // Ԥ�������һ��λ��
                        int pred_player_x = px + player_dx;
                        int pred_player_y = py + player_dy;
                        
                        // ���Ԥ��λ����Ч���������Ŀ��
                        if (is_valid(player, pred_player_x, pred_player_y)) {
                            int pred_dist = abs(next_x - pred_player_x) + abs(next_y - pred_player_y);
                            score += 500.0f / (pred_dist + 1);  // �������Ԥ��λ�õĶ��⽱��
                        }
                    }
                    
                    candidates[candidate_count].score = score - wall_penalty;
                    candidate_count++;
                }
            }
            
            // �ҵ���Ѻ�ѡ
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
        // ��������ǿ��״̬�µ��ӱ���ΪԤ�⡿
        // ǿ��״̬�£����᳢��Զ�����
        int escape_candidates[4];
        int escape_count = 0;
        
        for (int d = 0; d < 4; d++) {
            int next_x = gx + dx[d];
            int next_y = gy + dy[d];
            
            if (is_valid(player, next_x, next_y)) {
                int dist_to_player = abs(next_x - px) + abs(next_y - py);
                int current_dist = abs(gx - px) + abs(gy - py);
                
                // ������������Զ����ң������ѡ
                if (dist_to_player >= current_dist) {
                    escape_candidates[escape_count++] = d;
                }
            }
        }
        
        // ������ӱܷ���ѡ������ӱܷ���
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
    
    // ���Ż�2���Ľ�����ʷλ�÷�������������ת��Ԥ�⡿
    if (ghost_last_x[ghost_idx] != -1 && ghost_last_y[ghost_idx] != -1) {
        int move_dx = gx - ghost_last_x[ghost_idx];
        int move_dy = gy - ghost_last_y[ghost_idx];
        
        // Ԥ�������ǰ�����ƶ�
        int next_x = gx + move_dx;
        int next_y = gy + move_dy;
        
        // ����������ǿ����Ч�Լ�顿
        if (is_valid(player, next_x, next_y)) {
            // ���ǰ���Ƿ���ǽ���赲δ���ƶ�
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
            
            // ���ǰ��·�������ã�ʹ�����Ԥ��
            if (!future_blocked && path_quality >= 2) {
                *pred_x = next_x;
                *pred_y = next_y;
                return;
            }
        }
        
        // ����������ǰ����ǽ���赲ʱ������ת��Ԥ�⡿
        // ѡ����ӽ���ǰ�ƶ����ƵĿ��з���
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
                
                // �����ͷ
                if (dx[d] == -move_dx && dy[d] == -move_dy) {
                    score -= 100.0f;  // ����ͷ���ͷ
                } else {
                    // ���㷽����죨�����پ��룩
                    int dir_diff = abs(dx[d] - move_dx) + abs(dy[d] - move_dy);
                    score += (4 - dir_diff) * 20;  // �������ԽС����Խ��
                }
                
                // ���÷����·������
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
                score += path_steps * 15;  // ·����������
                
                turn_candidates[turn_count].dir = d;
                turn_candidates[turn_count].score = score;
                turn_count++;
            }
        }
        
        // ѡ�����ת��
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
    
    // ���Ż�3����ǿ�Ļ�����֪Ԥ�⡿
    // ���������ܵ��ƶ�����
    int movement_scores[4] = {0};
    int valid_moves = 0;
    
    for (int d = 0; d < 4; d++) {
        int next_x = gx + dx[d];
        int next_y = gy + dy[d];
        
        if (is_valid(player, next_x, next_y)) {
            valid_moves++;
            int score = 100;  // ��������
            
            // ���������������λ�õķ���ƫ�á�
            if (player->your_status == 0) {
                // ��ǿ��״̬�����������׷�����
                int dist_to_player = abs(next_x - px) + abs(next_y - py);
                score += (1000 / (dist_to_player + 1));
            } else {
                // ǿ��״̬�����������Զ�����
                int dist_to_player = abs(next_x - px) + abs(next_y - py);
                score += dist_to_player * 25;
            }
            
            // ����������������ͬ������
            int exit_routes = 0;
            for (int dd = 0; dd < 4; dd++) {
                int check_x = next_x + dx[dd];
                int check_y = next_y + dy[dd];
                if (is_valid(player, check_x, check_y)) {
                    exit_routes++;
                }
            }
            score += exit_routes * 20;  // ��·Խ��Խ��
            
            // ������������ƫ�� - ���ȿ�������
            if (exit_routes >= 3) {
                score += 50;  // ����������
            }
            
            movement_scores[d] = score;
        }
    }
    
    // ѡ������ƶ�����
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
    
    // ����fallback�����ص�ǰλ��
    *pred_x = gx;
    *pred_y = gy;
}

// �Ľ��İ�ȫ�ƶ�����ѡ�����������е�ͼ��С��
int choose_safe_direction(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_dir = -1;
    int max_safety = -1000;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        int safety_score = 0;
        
        // ���㵽������С���루Ȩ�ظ���ǿ��״̬������
        int min_ghost_dist = distance_to_ghost(player, nx, ny);
        
        // ǿ��״̬�µİ�ȫ����
        if (player->your_status > 0) {
            if (player->your_status > 8) {
                safety_score += min_ghost_dist * 2;  // ʱ����㣬Ȩ�ؽϵ�
            } else if (player->your_status > 3) {
                safety_score += min_ghost_dist * 6;  // ʱ�䲻�࣬��Ҫ����
            } else {
                safety_score += min_ghost_dist * 10;  // �������������뼫�Ƚ���
            }
        } else {
            safety_score += min_ghost_dist * 8;  // ��ǿ��״̬����Ȩ��
        }
        
        // Ԥ����λ�õİ�ȫ��
        int predicted_safety = 1000;  // Ĭ�ϰ�ȫ
        for (int i = 0; i < 2; i++) {
            int pred_x, pred_y;
            predict_ghost_movement(player, i, &pred_x, &pred_y);
            int pred_dist = abs(nx - pred_x) + abs(ny - pred_y);
            if (pred_dist < predicted_safety) {
                predicted_safety = pred_dist;
            }
        }
        safety_score += predicted_safety * 3;  // Ԥ�ⰲȫ�Խ���
        
        // �����ͷ�����Ǽ���Σ�գ�
        if (nx == last_x && ny == last_y) {
            int current_danger = assess_danger_level(player, x, y);
            if (current_danger >= 2) {
                safety_score -= 5;  // Σ��ʱ���ٻ�ͷ�ͷ�
            } else {
                safety_score -= 15;  // ��ȫʱ���ӻ�ͷ�ͷ�
            }
        }
        
        // ����Ƿ��ƶ�������ͬ����Ҫ��������ͬ�еĳ�����
        int exit_routes = 0;
        for (int dd = 0; dd < 4; dd++) {
            int check_x = nx + dx[dd];
            int check_y = ny + dy[dd];
            if (is_valid(player, check_x, check_y)) {
                exit_routes++;
            }
        }
        
        if (exit_routes <= 1) {
            int dead_end_penalty = 20;  // Ĭ������ͬ�ͷ�
            
            // �������ͬ�����Ƿ��г��������з�ɱ��ֵ
            if (player->your_status <= 0) {  // ��ǿ��״̬�ſ��ǳ�����
                // ��������������ļ������Ƿ��г�����
                for (int depth = 1; depth <= 4; depth++) {
                    int search_x = nx + dx[d] * (depth - 1);
                    int search_y = ny + dy[d] * (depth - 1);
                    
                    if (!is_valid(player, search_x, search_y)) break;
                    
                    if (player->mat[search_x][search_y] == 'O') {  // ���ֳ�����
                        float counter_value = evaluate_power_star_counter_attack_value(player, x, y, search_x, search_y);
                        
                        if (counter_value >= 300.0) {
                            dead_end_penalty = -30;  // �߼�ֵ��ɱ������ͬ��ɽ���
                        } else if (counter_value >= 150.0) {
                            dead_end_penalty = -10;  // �еȼ�ֵ��ɱ�����ٳͷ�
                        } else if (counter_value > 0) {
                            dead_end_penalty = 5;   // �ͼ�ֵ��ɱ��С�����ٳͷ�
                        }
                        break;  // �ҵ������Ǿ�ֹͣ����
                    }
                }
            }
            
            safety_score -= dead_end_penalty;
        }
        
        // Ѱ������İ�ȫ���Ƿ�����
        int closest_safe_star_dist = 1000;
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
                if (is_star(player, i, j)) {    
                    int star_danger = assess_danger_level(player, i, j);
                    if (star_danger < 2) {  // ֻ������԰�ȫ������
                        int star_dist_current = abs(nx - i) + abs(ny - j);
                        int star_dist_original = abs(x - i) + abs(y - j);
                        if (star_dist_current < star_dist_original && star_dist_current < closest_safe_star_dist) {
                            closest_safe_star_dist = star_dist_current;
                            safety_score += 8;  // ������ȫ���ǵķ���
                        }
                    }
                }
            }
        }
        
        // �߽簲ȫ�ԣ�����ǽ��ͨ������ȫ
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
        safety_score += wall_nearby * 2;  // ǽ�ڸ�����԰�ȫ
        
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
    if (player->your_status <= 0) return -1000.0;  // û��ǿ��״̬����׷��
    
    int ghost_x = player->ghost_posx[ghost_idx];
    int ghost_y = player->ghost_posy[ghost_idx];
    int distance = abs(sx - ghost_x) + abs(sy - ghost_y);
    
    float score = GHOST_HUNT_BASE_SCORE;  // ������ֵ200��
    
    // ��ǿ��������������׷������������ȷ�����ȼ����ڳ����ǡ�
    if (player->your_status > POWER_TIME_ABUNDANT) {
        score += 400.0;  // ��ǿ������150.0���������400.0��ʱ�����ʱ�޷�����������ֵ
    } else if (player->your_status > POWER_TIME_MODERATE) {
        score += 350.0;   // ��ǿ������80.0���������350.0��ʱ������ʱ�������
    } else {
        score += 200.0;   // ��ǿ������40.0���������200.0����ʹʱ�䲻��ҲҪ�����ȼ�׷��
    }
    
    // �������ӣ�����Խ����ֵԽ�ߣ�ǿ��״̬�¸�����
    float distance_factor = 1.0 / (distance + 1);
    score *= distance_factor * 5.0;  // ���Ż�����4.0������5.0��������
    
    // ���Ż��������ɵ�ǿ��ʱ�����
    if (player->your_status <= distance) {
        score -= 10.0;  // ���Ż�����-20.0��һ�����ٵ�-10.0��������
    } else if (player->your_status <= distance + 1) {
        score -= 5.0;   // ���Ż�����-10.0��һ�����ٵ�-5.0��������
    }
    
    // ��ǿ����ǿ��ʱ�伤�����ԣ�ȷ�����׷���������ȡ�
    if (player->your_status <= 3) {
        score += 100.0;  // ���Ż�����-10.0��Ϊ+50.0��ʱ�䲻��ʱҲҪ����׷��
        
        // �����������ʱ�̵�ƴ��������
        if (player->your_status <= 2) {
            score += 80.0;  // ���2�غϣ�ƴ��׷��
        }
        if (player->your_status <= 1) {
            score += 100.0; // ���1�غϣ�ȫ��׷��
        }
    } else if (player->your_status <= 5) {
        score += 400.0;  // ��ǿ������10.0���������400.0��ʱ�䲻��ʱ���׷�������ȼ�
    } else if (player->your_status <= 8) {
        score += 600.0;  // ��ǿ������30.0���������600.0��ʱ������ʱ���׷�������ȼ�
    } else {
        // ʱ�����ʱ�����������׷��
        score += 800.0;  // ��ǿ������80.0������100.0��ʱ�����ʱҲҪ���ֹ��׷������
    }
    
    // ��������·���ϵ���������������
    // ��·�����������ӵ�ǰλ�õ����λ�õ�ֱ��·���ϵ�����
    int path_star_bonus = 0;
    int steps_to_ghost = abs(ghost_x - sx) + abs(ghost_y - sy);
    
    if (steps_to_ghost <= 8) {  // ֻ�ԽϽ��Ĺ������·������
        // �򻯵�·�����Ǽ���
        int path_dx = (ghost_x > sx) ? 1 : ((ghost_x < sx) ? -1 : 0);
        int path_dy = (ghost_y > sy) ? 1 : ((ghost_y < sy) ? -1 : 0);
        
        int check_x = sx;
        int check_y = sy;
        
        for (int step = 0; step < steps_to_ghost && step < 8; step++) {
            if (is_valid(player, check_x, check_y)) {
                char cell = player->mat[check_x][check_y];
                if (cell == 'o') {
                    path_star_bonus += 15;  // ·���ϵ���ͨ����
                } else if (cell == 'O') {
                    path_star_bonus += 50;  // ·���ϵĳ�������
                }
            }
            
            // �ƶ�����һ��λ��
            if (abs(check_x - ghost_x) > 0) check_x += path_dx;
            if (abs(check_y - ghost_y) > 0) check_y += path_dy;
        }
    }
    
    score += path_star_bonus;
    
    // ���Ż��������ط����������ϡ�
    // �������������ؽ�Զ�����������㹻ʱ�䣬���ǳ����ط���
    if (ghost_spawn_x[ghost_idx] != -1 && ghost_spawn_y[ghost_idx] != -1) {
        int ghost_to_spawn_dist = abs(ghost_x - ghost_spawn_x[ghost_idx]) + abs(ghost_y - ghost_spawn_y[ghost_idx]);
        int our_to_spawn_dist = abs(sx - ghost_spawn_x[ghost_idx]) + abs(sy - ghost_spawn_y[ghost_idx]);
        
        // �������������ؽ�Զ�������Ǹ��ӽ������أ����Ƿ���
        if (ghost_to_spawn_dist >= 6 && our_to_spawn_dist < ghost_to_spawn_dist - 2 && player->your_status > 8) {
            score += 60.0;  // �����ط�������
        }
    }
    
    // ��鸽����ǿ�����ǣ����������ǿ�����Ǻܽ�������׷����ֵ
    for (int k = 0; k < power_star_cache_size; k++) {
        int power_star_dist_from_ghost = abs(ghost_x - power_star_cache[k].x) + abs(ghost_y - power_star_cache[k].y);
        
        // ���������ǿ�����Ǻܽ���׷����������
        if (power_star_dist_from_ghost <= 3) {
            score -= 15.0;  // ���Ż�����-25.0���ٵ�-15.0����꿿��ǿ�����ǣ����ֿ��ܻ��ǿ��
        }
    }
    
    // Ԥ�����ƶ�������׷���Ѷ�
    int pred_x, pred_y;
    predict_ghost_movement(player, ghost_idx, &pred_x, &pred_y);
    int pred_distance = abs(sx - pred_x) + abs(sy - pred_y);
    
    // ��������Զ�룬����׷����ֵ
    if (pred_distance > distance) {
        score -= 20.0;  // ���Ż�����-30.0���ٵ�-20.0�����������
    } else if (pred_distance < distance) {
        score += 30.0;  // ���Ż�����20.0������30.0������ڿ���
    }
    
    // �������ƣ����������ͬ�еĹ�������׷��
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int check_x = ghost_x + dx[d];
        int check_y = ghost_y + dy[d];
        if (is_valid(player, check_x, check_y)) {
            escape_routes++;
        }
    }
    
    if (escape_routes <= 2) {
        score += 60.0;  // ���Ż�����40.0������60.0�����������ͬ�����
    } else if (escape_routes == 3) {
        score += 30.0;  // ���Ż�����20.0������30.0�����ѡ������
    }
    
    // ˫����룺����������ܽ���׷����������
    if (ghost_idx == 0) {
        int other_ghost_dist = abs(ghost_x - player->ghost_posx[1]) + abs(ghost_y - player->ghost_posy[1]);
        if (other_ghost_dist <= 3) {
            score -= 15.0;  // ���Ż�����-25.0���ٵ�-15.0��˫��ۼ���׷�����մ�
        }
    } else {
        int other_ghost_dist = abs(ghost_x - player->ghost_posx[0]) + abs(ghost_y - player->ghost_posy[0]);
        if (other_ghost_dist <= 3) {
            score -= 15.0;  // ���Ż�����-25.0���ٵ�-15.0��
        }
    }
    
    // �����������������׷������ - ��̬������
    int target_score = 8000;  // Ŀ�����
    int low_threshold = target_score / 3;    // 2666��
    int mid_threshold = target_score / 2;    // 4000��
    int high_threshold = target_score * 2/3; // 5333��
    
    if (player->your_score < low_threshold) {
        score += 100.0;  // �������ز���ʱ��׷����ֵ��������
    } else if (player->your_score < mid_threshold) {
        score += 80.0;   // ��������ʱ��׷����ֵ�������
    } else if (player->your_score < high_threshold) {
        score += 60.0;   // �����ӽ�Ŀ��ʱ���ʶ�����
    } else if (player->your_score < target_score) {
        score += 40.0;   // �ӽ�Ŀ��ʱ����΢����
    }
    
    return score;
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
int bfs_ghost_hunt(struct Player *player, int sx, int sy, int target_ghost, int *next_x, int *next_y) {
    if (target_ghost < 0 || target_ghost >= 2) return 0;
    
    int ghost_x = player->ghost_posx[target_ghost];
    int ghost_y = player->ghost_posy[target_ghost];
    
    int vis[MAXN][MAXN] = {0};
    int prex[MAXN][MAXN], prey[MAXN][MAXN];
    int qx[MAXN*MAXN], qy[MAXN*MAXN], head = 0, tail = 0;
    
    // ��������·����ֵ�������顿
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
    
    // ׷��ģʽ����������ʵ����ƣ�������ͨBFS������
    int max_search = 250;  // ���Ż�����200������250��
    int searched = 0;
    
    while (head < tail && searched < max_search) {
        int x = qx[head], y = qy[head]; 
        head++;
        searched++;
        
        // ������λ��
        if (x == ghost_x && y == ghost_y) {
            // ���Ż�������ʱѡ���ֵ��ߵ�·����
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
        
        // Ԥ��������ƶ�����λ��Ҳ����ЧĿ��
        int pred_x, pred_y;
        predict_ghost_movement(player, target_ghost, &pred_x, &pred_y);
        if (x == pred_x && y == pred_y) {
            // ����Ԥ��λ��Ҳ��ɹ�
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
        
        // ���Ż�����ǿ�ķ������򣬿���·����ֵ��
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
                
                // �������ȼ���������Խ�� + ·�����Ǽ�ֵ
                int dist_to_ghost = abs(nx - ghost_x) + abs(ny - ghost_y);
                float priority = 1000.0f / (dist_to_ghost + 1);
                
                // ��������·����������������
                char cell = player->mat[nx][ny];
                if (cell == 'o') {
                    priority += 50.0f;  // ��ͨ��������
                } else if (cell == 'O') {
                    priority += 120.0f; // ������������
                }
                
                // ������������ǿ��ʱ���·����ֵ��
                if (player->your_status > 0) {
                    // ǿ��ʱ��Խ�٣�Խ��Ӧ����·�ռ�����
                    if (player->your_status <= 3) {
                        if (cell == 'o') priority += 20.0f;  // ʱ�����ʱ���Ǽ�ֵ����
                        if (cell == 'O') priority += 40.0f;  // �����������м�ֵ
                    } else if (player->your_status <= 6) {
                        if (cell == 'o') priority += 35.0f;  // �е�ʱ��ʱ�ʶȽ���
                        if (cell == 'O') priority += 80.0f;  // �����Ǹ߼�ֵ
                    } else {
                        if (cell == 'o') priority += 50.0f;  // ʱ�����ʱ��ֽ���
                        if (cell == 'O') priority += 120.0f; // �����Ǽ��߼�ֵ
                    }
                }
                
                // ��������·�������Խ�����
                if (path_value[x][y] > 0) {
                    priority += path_value[x][y] * 0.3f;  // �̳�ǰ��·���ļ�ֵ
                }
                
                directions[valid_directions].priority = priority;
                valid_directions++;
            }
        }
        
        // �����ȼ�����
        for (int i = 0; i < valid_directions; i++) {
            for (int j = i + 1; j < valid_directions; j++) {
                if (directions[j].priority > directions[i].priority) {
                    DirectionPriority temp = directions[i];
                    directions[i] = directions[j];
                    directions[j] = temp;
                }
            }
        }
        
        // �����ȼ�˳����ӵ�����
        for (int i = 0; i < valid_directions; i++) {
            int d = directions[i].dir;
            int nx = x + dx[d], ny = y + dy[d];
            
            vis[nx][ny] = 1;
            prex[nx][ny] = x; 
            prey[nx][ny] = y;
            path_value[nx][ny] = directions[i].priority;  // ��¼·����ֵ
            qx[tail] = nx; 
            qy[tail] = ny; 
            tail++;
        }
    }
    
    return 0;  // ׷��ʧ��
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
    int predicted_min_dist = INF;
    
    // ���ǹ��Ԥ��λ��
    for (int i = 0; i < 2; i++) {
        int pred_x, pred_y;
        predict_ghost_movement(player, i, &pred_x, &pred_y);
        int pred_dist = abs(x - pred_x) + abs(y - pred_y);
        if (pred_dist < predicted_min_dist) {
            predicted_min_dist = pred_dist;
        }
    }
    
    // �����������ݵ�ͼ��С��̬������ȫ���롿
    int map_size = player->row_cnt * player->col_cnt;
    int safe_distance, danger_distance_immediate, danger_distance_close;
    
    if (map_size <= SMALL_MAP_THRESHOLD) {
        // С��ͼ������Ҫ�������
        safe_distance = 1;
        danger_distance_immediate = 1;
        danger_distance_close = 1;
    } else if (map_size >= LARGE_MAP_THRESHOLD) {
        // ���ͼ������Ҫ����ϸ�
        safe_distance = 3;
        danger_distance_immediate = 2;
        danger_distance_close = 3;
    } else {
        // �еȵ�ͼ����׼����Ҫ��
        safe_distance = 2;
        danger_distance_immediate = 1;
        danger_distance_close = 2;
    }
    
    // ��������·�����������ڲ����ĵ��ĵ������أ�
    int escape_routes = 0;
    for (int d = 0; d < 4; d++) {
        int escape_x = x + dx[d];
        int escape_y = y + dy[d];
        if (is_valid(player, escape_x, escape_y)) {
            // ���������ܷ����Ƿ�Զ����
            int escape_ghost_dist = distance_to_ghost(player, escape_x, escape_y);
            if (escape_ghost_dist >= min_ghost_dist) {  // ������ӽ����
                escape_routes++;
            }
        }
    }
    
    // �ۺϵ�ǰ��Ԥ���������Σ�յȼ�
    int overall_dist = (min_ghost_dist + predicted_min_dist) / 2;
    
    // ��������������·��Խ�٣�Σ�յȼ�Խ��
    int terrain_penalty = 0;
    if (escape_routes <= 1) {
        terrain_penalty = 1;  // ����ͬ�򼸺���·����
    } else if (escape_routes == 2) {
        terrain_penalty = 0;  // ���ȣ��еȷ���
    }
    // escape_routes >= 3 ʱ��terrain_penalty = 0�������ش��ϰ�ȫ
    
    // ���Ż�����ͼ��С����Ӧ��ǿ��״̬������
    if (player->your_status > 0) {
        // ǿ��״̬ʱ����㣬������ȫ
        if (player->your_status > POWER_TIME_ABUNDANT) {
            return 0;  
        }
        // ǿ��״̬ʱ�䲻�࣬��Ҫ����
        else if (player->your_status > POWER_TIME_MODERATE) {
            if (overall_dist <= danger_distance_immediate) {
                return 1;  // �е�Σ�գ�����̫����
            } else {
                return 0;  // ��԰�ȫ
            }
        }
        // ǿ��״̬������������Σ��
        else {
            if (overall_dist <= danger_distance_immediate) {
                return 2;  // ��Σ�գ���������Զ��
            } else if (overall_dist <= danger_distance_close) {
                return 1;  // �е�Σ��
            } else {
                return 0;  // ��ȫ
            }
        }
    } else {
        // ���Ż�����ͼ��С����Ӧ�ķ�ǿ��״̬Σ��������
        int base_danger = 0;
        if (overall_dist <= danger_distance_immediate) {
            base_danger = 2;  // ��Σ��
        } else if (overall_dist <= danger_distance_close) {
            base_danger = 1;  // �е�Σ��
        } else if (overall_dist <= safe_distance) {
            // ���������ڰ�ȫ�����Եʱ����������Ӱ�����
            base_danger = (escape_routes <= 1) ? 1 : 0;
        } else {
            base_danger = 0;  // ��ȫ
        }
        
        // Ӧ�õ�������������ͬʱ����Σ�յȼ�
        int final_danger = base_danger + terrain_penalty;
        
        // ��������С��ͼ���⴦�� - ���ӿ��ɵİ�ȫ������
        if (map_size <= SMALL_MAP_THRESHOLD) {
            // С��ͼ�У�ֻ���ڷǳ��ӽ�ʱ����Ϊ��Σ��
            if (overall_dist <= 1 && escape_routes <= 1) {
                final_danger = 2;  // ��Σ��
            } else if (overall_dist <= 1) {
                final_danger = 1;  // �е�Σ��
            } else {
                final_danger = 0;  // ��ȫ
            }
        }
        
        return (final_danger > 2) ? 2 : final_danger;  // ���Σ�յȼ�Ϊ2
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
    
    return manhattan + wall_count / 2;  // ���ͼ����ǽ��ͷ�
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
float evaluate_star(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int star_value = (star_type == 'O') ? 20 : 10;
    
    int distance = heuristic(player, sx, sy, star_x, star_y);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    int map_size = player->row_cnt * player->col_cnt;
    
    // С��ͼר���Ż����� - �ȶ����Ż���
    if (map_size <= SMALL_MAP_THRESHOLD) {
        float score = star_value;
        
        // �������ӣ�����Խ������Խ��
        float distance_factor = 1.0 / (distance + 1);
        score *= distance_factor * 10.0;
        
        // Ŀ���ȶ��Խ���������ǵ�ǰĿ�꣬������⽱��
        if (star_x == current_target_x && star_y == current_target_y) {
            score += 30.0;  // �ȶ�Ŀ�꽱��
        }
        
            //������������ǿ��״̬�������������ȼ�����ǿ��״̬�³����Ǽ�ֵ�ߣ�ǿ��״̬�½���
            if (star_type == 'O') {
                if (player->your_status > 0) {
                    score += 150.0;  // ��������ǿ��״̬�»�����ֵ���͵�150.0������׷�����
                    
                    // ���Ż���ǿ��״̬�³����Ǽ�ֵ��̬������
                    if (player->your_status <= 5) {
                        score += 100.0;  // ����������350.0������͵�80.0����������ʱ���ͳ��������ȼ�
                        
                        // ��������ʱ�����ʱ�Ķ��⽱����
                        if (player->your_status <= 3) {
                            score += 250.0;  // ���Ż�����150.0������250.0�����3�غϣ������Ǽ�ֵ����
                        }
                        
                        // ��������ǿ��״̬�³����ǵ�������ֵ��
                        if (player->your_status <= 2) {
                            score += 350.0;  // ���2�غϣ���������Ψһ�����ֶ�
                        }
                    } else if (player->your_status <= 10) {
                        score += 60.0;  // ����������150.0���͵�60.0���е�ʱ��ʱ�ʶȽ���
                    } else {
                        score += 40.0;   // ����������80.0���͵�40.0��ʱ�����ʱ��΢����
                    }
                } else {
                    // ����ǿ��״̬�µĳ����Ǵ���
                    score += 750.0;  // �����֣���ǿ��״̬�»�����ֵ450.0�����ǿ��״̬�ǻ�ʤ�ؼ�
                    score += 500.0;  // ���Ż�����180.0������500.0�����⽱����ȷ�����ǿ��
                    
                    // ��ǿ��״̬��������ɱ��ֵ
                    float counter_attack_value = evaluate_power_star_counter_attack_value(player, sx, sy, star_x, star_y);
                    
                    if (counter_attack_value > 0) {
                        score += counter_attack_value;  // ���Ϸ�ɱ��ֵ
                        int current_danger = assess_danger_level(player, star_x, star_y);
                        
                        // ���Ż�����ɱ��ֵ�ܸ�ʱ�������������ȼ���
                        if (counter_attack_value >= 300.0) {
                            score += 400.0;  // ���Ż�����300.0������400.0��������
                            
                            // ���Ż����߼�ֵ��ɱʱ���������Σ�ճͷ���
                            if (current_danger >= 2) {
                                score += 300.0;  // ���Ż�����200.0������300.0�����󲹳���Σ�ճͷ�
                            } else if (current_danger == 1) {
                                score += 150.0;  // ���Ż�����100.0������150.0��
                            }
                        } else if (counter_attack_value >= 150.0) {
                            score += 250.0;  // ���Ż�����180.0������250.0��
                            
                            // ���Ż����еȼ�ֵ��ɱʱ����������Σ�ճͷ���
                            if (current_danger >= 2) {
                                score += 180.0;  // ���Ż�����120.0������180.0��
                            } else if (current_danger == 1) {
                                score += 90.0;   // ���Ż�����60.0������90.0���е�Σ�ղ���
                            }
                        } else if (counter_attack_value >= 50.0) {
                            score += 120.0;   // ���Ż�����80.0������120.0���ͼ�ֵ��ɱҲ���轱��
                            
                            // ���������ͼ�ֵ��ɱʱ����΢����Σ�ճͷ���
                            if (current_danger >= 2) {
                                score += 80.0;  // ���Ż�����50.0������80.0����Σ��ʱ���貹��
                            }
                        }
                    }
                    
                    // ��������ս���Գ������ռ����� - ��̬������
                    // �����Ƿ�������ʱ�������Ǽ�ֵ����
                    int target_score = 10000;  // Ŀ�����
                    int low_threshold = target_score / 3;    // 3333��
                    int mid_threshold = target_score / 2;    // 5000��
                    int high_threshold = target_score * 2/3; // 6667��
                    
                    if (player->your_score < low_threshold) {
                        score += 400.0;  // �������ز���ʱ�������Ǽ�ֵ��������
                    } else if (player->your_score < mid_threshold) {
                        score += 300.0;  // ��������ʱ�������Ǽ�ֵ�������
                    } else if (player->your_score < high_threshold) {
                        score += 200.0;  // �����ӽ�Ŀ��ʱ���ʶ�����
                    } else if (player->your_score < target_score) {
                        score += 100.0;  // �ӽ�Ŀ��ʱ����΢����
                    }
                
                // ���Ż�������ͬ���������⴦��������
                int escape_routes = 0;
                for (int d = 0; d < 4; d++) {
                    int check_x = star_x + dx[d];
                    int check_y = star_y + dy[d];
                    if (is_valid(player, check_x, check_y)) {
                        escape_routes++;
                    }
                }
                
                if (escape_routes <= 2 && distance <= 4) {
                    score += 150.0;  // ���Ż�����80.0������150.0������ͬ�����볬���ǣ��޴���ά��
                } else if (escape_routes <= 1 && distance <= 6) {
                    score += 200.0;  // ���Ż�����120.0������200.0������������ͬ��������
                } else if (escape_routes <= 2 && distance <= 8) {
                    score += 100.0;  // ���������еȾ�������ͬ������
                }
                
                // ����������������ʱ�ļ������� - ��̬������
                int target_score = 10000;  // Ŀ�����
                int very_low_threshold = target_score / 4;    // 2500��
                int low_threshold = target_score / 2;         // 5000��
                int high_threshold = target_score * 3/4;      // 7500��
                
                if (player->your_score < very_low_threshold) {
                    score += 300.0;  // �������Ȳ���ʱ�������Ǽ�ֵ����
                } else if (player->your_score < low_threshold) {
                    score += 200.0;  // �������ز���ʱ�������Ǽ�ֵ����
                } else if (player->your_score < high_threshold) {
                    score += 120.0;   // ��������ʱ���ʶ�����
                } else if (player->your_score < target_score) {
                    score += 60.0;   // �ӽ�Ŀ��ʱ����΢����
                }
            }
            
            // ���Ż����ѳԳ����������ĳͷ���һ�����͡�
            if (power_star_eaten_count >= 2) {
                score -= 15.0;   // ���Ż���������΢���͡�
            }
            // ��������ǰ�ڳ����ǽ�����
            if (power_star_eaten_count == 0) {
                score += 80.0;  // ��һ�������Ƕ��⽱��
            }
        }
        
        // ��̬�������������ڹ��Ԥ�⣩
        int current_danger = assess_danger_level(player, star_x, star_y);
        
        // �Ľ���ǿ��״̬�������������ȣ�
        if (player->your_status > 0) {
            // ǿ��״̬ʱ�����
            if (player->your_status > POWER_TIME_ABUNDANT) {
                score += 8.0;  // ���Ż�����5.0������8.0��������������
                
                // ��������ʱ�����ʱ��ֵ����
                if (star_type == 'O') {
                    score += 15.0;  // ���Ż�����-5.0��Ϊ+15.0��ʱ�����ʱ���������м�ֵ
                }
                
                // ֻ�а�ȫ�ҷǳ���������ֵ�ÿ���
                if (current_danger == 0 && distance <= 1) {
                    score += 25.0;  // ���Ż�����15.0������25.0����ȫ�����ֿɵõ�����
                }
            }
            // ǿ��״̬ʱ������
            else if (player->your_status > POWER_TIME_MODERATE) {
                score += 25.0;  // ���Ż�����15.0������25.0�����н���
                
                // ��ȫ����Ȼ��Ҫ�������缴������ʱ�ؼ�
                if (current_danger >= 2) {
                    score -= 20.0;  // ���Ż�����-30.0���ٵ�-20.0����Σ��ʱ���ּ���
                } else if (current_danger == 1) {
                    score -= 5.0;   // ���Ż�����-10.0���ٵ�-5.0���е�Σ��ʱ��΢����
                }
                
                // �����Ǵ���
                if (star_type == 'O') {
                    if (current_danger < 2) {  // ֻ����԰�ȫʱ�ſ���
                        score += 25.0;  // ���Ż�����10.0������25.0���е�ʱ��ʱ��������һ����ֵ
                    } else {
                        score -= 5.0;   // ���Ż�����-15.0���ٵ�-5.0��Σ��ʱ���ⳬ���ǳͷ�����
                    }
                }
                
                // ���������ǽ�������Ҫ���ǰ�ȫ�ԣ�
                if (distance <= 1 && current_danger < 2) {
                    score += 30.0;  // ���Ż�����20.0������30.0����ȫ�Ľ���������
                } else if (distance <= 2 && current_danger == 0) {
                    score += 15.0;  // ���Ż�����10.0������15.0����ȫ�ĽϽ�����
                }
            }
            // ǿ��״̬����������1-3�غϣ�
            else {
                // ��������ʱ��Ҫ���ǳ����ǵľ޴��ֵ
                if (current_danger >= 2) {
                    score -= 25.0;  // ���Ż�����-40.0���ٵ�-25.0����Σ��ʱ�Ը�����
                } else if (current_danger == 1) {
                    score -= 10.0;  // ���Ż�����-20.0���ٵ�-10.0���е�Σ��ʱ��΢����
                } else {
                    score += 120.0; // ���Ż�����80.0������120.0����ȫʱ������ӷ�
                }
                
                // �������ڼ�������ʱֻ�а�ȫʱ�ſ���
                if (star_type == 'O') {
                    if (current_danger == 0) {
                        score += 50.0;  // ���Ż�����30.0������50.0����ȫ�ĳ����Ƿǳ���Ҫ
                    } else if (current_danger == 1) {
                        score += 20.0;  // ���������е�Σ��ʱҲ���轱����
                    } else {
                        score -= 10.0;  // ���Ż�����-20.0���ٵ�-10.0������ȫ�ĳ�����Ҫ����
                    }
                }
                
                // ֻ�зǳ����Ұ�ȫ������ֵ��ð��
                if (distance <= 1 && current_danger == 0) {
                    score += 60.0;  // ���Ż�����40.0������60.0����ȫ�����ֿɵ�����
                } else if (distance <= 2 && current_danger == 0) {
                    score += 30.0;  // ���Ż�����20.0������30.0����ȫ�ĺܽ�����
                } else if (distance > 3) {
                    score -= 15.0;  // ���Ż�����-25.0���ٵ�-15.0��Զ�������Ƿ���̫��
                }
            }
        } else {
            // ��ǿ��״̬�ķ������� - �Գ����Ǽ��ȿ��ɣ����Ȼ��ǿ��
            if (star_type == 'O') {
                // ���������⴦��������ͷ��ճͷ�
                if (current_danger >= 2) {
                    score -= 5.0;   // ���Ż�����-10.0��һ�����͵�-5.0��������ֵ��ð��
                } else if (current_danger == 1) {
                    score += 5.0;   // ���Ż�����-2.0��Ϊ+5.0���е�Σ��ʱҲ���轱��
                }
                
                // ����ǰ�ȫ�ĳ����ǣ����⽱��
                if (current_danger == 0) {
                    score += 80.0;  // ���Ż�����50.0������80.0����ȫ�����Ǿ޶��
                }
                
                // �������������Ǿ��뽱����
                if (distance <= 2) {
                    score += 60.0;  // �����볬���Ƕ��⽱��
                } else if (distance <= 4) {
                    score += 30.0;  // �о��볬���ǽ���
                }
            } else {
                // ��ͨ���ǵķ�����������ԭ���߼�
                if (current_danger >= 2) {
                    score -= 30.0;  // ��Σ��ʱ����
                } else if (current_danger == 1) {
                    score -= 10.0;  // �е�Σ��ʱ��΢����
                }
            }
        }
        
        // ������ȫ�ƶ��������������ƶ��ܰ�ȫ��������΢ð��
        if (consecutive_safe_moves >= 5) {
            score += 10.0;  // ���ڰ�ȫ������ʵ�����
        }
        
        // ��Ϸ�׶���Ӧ����������ϡ��ʱ������
        int total_stars = 0;
        for (int i = 0; i < player->row_cnt; i++) {
            for (int j = 0; j < player->col_cnt; j++) {
                if (is_star(player, i, j)) total_stars++;
            }
        }
        
        if (total_stars <= 10) {
            score += 15.0;  // ��������ϡ�٣�ÿ�����Ƕ�����Ҫ
            if (star_type == 'O') {
                score += 25.0;  // ���ڳ����Ǽ�����Ҫ
            }
        }
        
        // 10000��Ŀ��ר�÷��������Ż�
        // ���ݵ�ǰ��������Ϸ���ȶ�̬����������
        float score_boost = 0.0;
        int current_score = player->your_score;
        
        // ��������ʱ���Ӽ�����׷��߼�ֵĿ��
        int target_score = 10000;  // Ŀ�����
        if (current_score < target_score / 4) {  // 2500��
            // �������ز��㣬�����Ǽ�ֵ����
            if (star_type == 'O') {
                score_boost += 150.0;
            } else {
                score_boost += 15.0;  // ��ͨ����Ҳ��Ҫ�����ռ�
            }
        } else if (current_score < target_score / 2) {  // 4000��
            // �������㣬�ʶ�������ֵ
            if (star_type == 'O') {
                score_boost += 100.0;
            } else {
                score_boost += 10.0;
            }
        } else if (current_score < target_score * 3/4) {  // 6000��
            // �ӽ�Ŀ�꣬���ּ���
            if (star_type == 'O') {
                score_boost += 50.0;
            } else {
                score_boost += 5.0;
            }
        } else if (current_score < target_score) {  // 8000��
            // ����̽׶�
            if (star_type == 'O') {
                score_boost += 25.0;
            }
        }
        
        // ��Ϸ���ڣ������϶ࣩʱ���Ӽ���
        if (step_count > 100) {
            score_boost += 20.0;
            if (star_type == 'O') {
                score_boost += 30.0;  // ���ڳ����Ǹ���Ҫ
            }
        }
        
        score += score_boost;
        
        // λ�ð�ȫ�Խ���
        int edge_bonus = 0;
        if (star_x == 0 || star_x == player->row_cnt-1 || 
            star_y == 0 || star_y == player->col_cnt-1) {
            edge_bonus = 8;
        }
        if ((star_x <= 1 || star_x >= player->row_cnt-2) && 
            (star_y <= 1 || star_y >= player->col_cnt-2)) {
            edge_bonus += 5;  // ����λ�ö��⽱��
        }
        score += edge_bonus;
        
        // ·���ɴ��Լ�飺����ѡ�񱻹����ȫ����������
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
            score -= 30.0;  // ��ȫ��������������
        }
        
        return score;
    }
    
    // ���ͼ�߼����ֲ��䣬��Ҳ����һЩ�ȶ��Կ���
    float distance_factor = 1.0;
    float danger_threshold = 1;
    float near_threshold = 2;
    float mid_threshold = 5;
    
    if (map_size > LARGE_MAP_THRESHOLD) {  // ���ͼ
        distance_factor = 1.5;  // ���ͼ����Ӱ�����
        danger_threshold = 2;   // ���ͼΣ����ֵ�ſ�
        near_threshold = 4;     // ���ͼ��������ֵ����
        mid_threshold = 10;     // ���ͼ�о�����ֵ����
    }
    
    // Ŀ���ȶ��Խ��������ͼҲ���ã�
    float stability_bonus = 0;
    if (star_x == current_target_x && star_y == current_target_y) {
        stability_bonus = 15.0;  // ���ͼ�ȶ�Ŀ�꽱��
    }
    
    // ��������
    float danger_penalty = 0;
    if (ghost_dist <= danger_threshold && player->your_status <= 0) {
        danger_penalty = 3.0;
    }
    
    // ǿ��״̬����
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
    
    // ���뽱�������ͼ������
    float distance_bonus = 0;
    if (distance <= near_threshold) {
        distance_bonus = 5.0;
    } else if (distance <= mid_threshold) {
        distance_bonus = 2.0;
    }
    
    // ���ͼ���⴦���������ڴ��ͼ�и���Ҫ
    if (map_size > LARGE_MAP_THRESHOLD && star_type == 'O') {
        power_bonus += 5.0;  // ���ͼ�����Ƕ��⽱��
    }
    
    // ��ͼ�Ż����ԣ�������Ȧ���ǣ���������ʽ�ռ�
    float region_bonus = 0;
    int current_region = get_region(player, sx, sy);
    int star_region = get_region(player, star_x, star_y);
    int target_region_stars = count_stars_in_region(player, star_region);
    
    // �����ռ���Ȧ���ǣ����⴩Խ����ǽ������
    if (star_region == 1) {  // ��Ȧ����
        region_bonus += 8.0;
        
        // �����ǰҲ����Ȧ����һ���������ȼ�
        if (current_region == 1) {
            region_bonus += 6.0;
        }
    } else {  // ��Ȧ����
        // ֻ����Ȧ���Ǻ���ʱ�ſ�����Ȧ
        if (target_region_stars >= 3) {
            region_bonus = -5.0;  // ������Ȧ���ȼ�
        } else {
            region_bonus = 2.0;   // ��Ȧ����ˣ����Կ�����Ȧ
        }
    }
    
    // ����ʽ·������������ѡ�����γ�����·��������
    int center_x = player->row_cnt / 2;
    int center_y = player->col_cnt / 2;
    
    // ����Ƕȣ�����˳ʱ���ƶ�
    if (star_region == 1) {  // ֻ����Ȧ����Ӧ����������
        int dx_current = sx - center_x;
        int dy_current = sy - center_y;
        int dx_star = star_x - center_x;
        int dy_star = star_y - center_y;
        
        // �򻯵ĽǶ��жϣ�����˳ʱ�뷽�������
        if ((dx_current >= 0 && dy_current >= 0 && dx_star >= dy_current && dy_star >= dx_current) ||  // ��������˳ʱ��
            (dx_current <= 0 && dy_current >= 0 && dx_star <= dy_current && dy_star >= -dx_current) || // ��������˳ʱ��
            (dx_current <= 0 && dy_current <= 0 && dx_star <= -dy_current && dy_star <= dx_current) || // ��������˳ʱ��
            (dx_current >= 0 && dy_current <= 0 && dx_star >= -dy_current && dy_star <= -dx_current)) { // ��������˳ʱ��
            region_bonus += 4.0;  // ����˳ʱ�뷽�������
        }
    }
    
    return (float)star_value * distance_factor / (distance + 1) + power_bonus + distance_bonus + region_bonus + stability_bonus - danger_penalty;
}

// �Ż���BFS��С��ͼ�ʹ��ͼͨ�ã�
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
    
    // С��ͼ������������ȣ����ͼ��������������
    int max_search = (map_size <= SMALL_MAP_THRESHOLD) ? map_size * 2 : 
                     ((map_size > LARGE_MAP_THRESHOLD) ? MAX_BFS_SEARCHES_LARGE : 500);
    int searched = 0;
    
    while (head < tail && searched < max_search) {
        int x = qx[head], y = qy[head]; head++;
        searched++;
        
        if (is_star(player, x, y)) {
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
    
    // �����ؽӽ�ģʽ����������ʵ�����
    int max_search = 150;  // 10x10��ͼ�㹻��
    int searched = 0;
    
    while (head < tail && searched < max_search) {
        int x = qx[head], y = qy[head]; 
        head++;
        searched++;
        
        // ���������λ��
        if (x == spawn_x && y == spawn_y) {
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
        
        // �ڳ����ظ���Ҳ��ɹ���׼���ȴ���
        int spawn_dist = abs(x - spawn_x) + abs(y - spawn_y);
        if (spawn_dist <= 2) {
            // ��������ظ��������Կ�ʼ�ȴ�
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
        
        // �����������ӽ������صķ���
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
    
    return 0;  // �����ؽӽ�ʧ��
}

// ���򻮷ֺ��������Բ�ε�ͼ�ľ������
int get_region(struct Player *player, int x, int y) {
    int center_x = player->row_cnt / 2;
    int center_y = player->col_cnt / 2;
    
    // ���㵽���ĵľ���
    int dist_to_center = abs(x - center_x) + abs(y - center_y);
    
    // ���ݾ����Ϊ��Ȧ(0)����Ȧ(1)
    int radius_threshold = (player->row_cnt + player->col_cnt) / 6;
    return (dist_to_center < radius_threshold) ? 0 : 1;
}

// ����ָ���������������
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

// ���������ֲ�·���滮 - �����ʼ����
void initialize_region_map(struct Player *player) {
    if (region_map_initialized) return;
    
    int map_size = player->row_cnt * player->col_cnt;
    if (map_size <= LARGE_MAP_THRESHOLD) {
        region_map_initialized = 1;
        return;  // С��ͼ����Ҫ���򻮷�
    }
    
    region_count = 0;
    int regions_per_row = (player->row_cnt + REGION_SIZE - 1) / REGION_SIZE;//ÿһ���ж��ٸ�����
    int regions_per_col = (player->col_cnt + REGION_SIZE - 1) / REGION_SIZE;//ÿһ���ж��ٸ�����
    
    for (int i = 0; i < regions_per_row && region_count < MAX_REGIONS; i++) {
        for (int j = 0; j < regions_per_col && region_count < MAX_REGIONS; j++) {
            region_map[region_count].center_x = i * REGION_SIZE + REGION_SIZE / 2;//��������x����
            region_map[region_count].center_y = j * REGION_SIZE + REGION_SIZE / 2;//��������y����
            
            // ȷ�����ĵ��ڵ�ͼ��Χ��
            if (region_map[region_count].center_x >= player->row_cnt) {
                region_map[region_count].center_x = player->row_cnt - 1;
                //�����������x���곬����ͼ��Χ��������Ϊ��ͼ���һ��
            }
            if (region_map[region_count].center_y >= player->col_cnt) {
                region_map[region_count].center_y = player->col_cnt - 1;
                //�����������y���곬����ͼ��Χ��������Ϊ��ͼ���һ��
            }
            
            region_map[region_count].star_count = 0;//������������
            region_map[region_count].power_star_count = 0;//������������
            region_map[region_count].danger_level = 0;//����Σ�յȼ�
            
            region_count++;
        }
    }
    
    region_map_initialized = 1;
}

// ����������ȡλ����������
int get_region_index(struct Player *player, int x, int y) {
    if (!region_map_initialized) {
        initialize_region_map(player);
    }
    
    int region_row = x / REGION_SIZE;//�����кţ�x
    int region_col = y / REGION_SIZE;//�����к�: y
    int regions_per_col = (player->col_cnt + REGION_SIZE - 1) / REGION_SIZE;//ÿһ���ж��ٸ�����
    
    int region_idx = region_row * regions_per_col + region_col;//���������������к�*ÿһ���ж��ٸ�����+�����к�
    return (region_idx < region_count) ? region_idx : -1;//�����������С�������������򷵻��������������򷵻�-1
}

// ������������������Ϣ��
void update_region_info(struct Player *player) {
    if (!region_map_initialized) return;
    
    // ����������Ϣ
    for (int i = 0; i < region_count; i++) {
        region_map[i].star_count = 0;
        region_map[i].power_star_count = 0;
        region_map[i].danger_level = 0;
    }
    
    // ����������Ϣ
    for (int i = 0; i < star_cache_size; i++) {
        int region_idx = get_region_index(player, star_cache[i].x, star_cache[i].y);
        if (region_idx >= 0) {
            region_map[region_idx].star_count++;
        }
    }
    
    // ����ǿ��������Ϣ
    for (int i = 0; i < power_star_cache_size; i++) {
        int region_idx = get_region_index(player, power_star_cache[i].x, power_star_cache[i].y);
        if (region_idx >= 0) {
            region_map[region_idx].power_star_count++;
        }
    }
    
    // ����Σ�յȼ�
    for (int i = 0; i < region_count; i++) {
        int region_x = region_map[i].center_x;
        int region_y = region_map[i].center_y;
        
        // �����굽�������ĵľ���
        int min_ghost_dist = INF;
        for (int j = 0; j < 2; j++) {
            int ghost_dist = abs(region_x - player->ghost_posx[j]) + abs(region_y - player->ghost_posy[j]);
            if (ghost_dist < min_ghost_dist) {
                min_ghost_dist = ghost_dist;
            }
        }
        
        if (min_ghost_dist <= REGION_SIZE) {
            region_map[i].danger_level = 2;  // ��Σ��
        } else if (min_ghost_dist <= REGION_SIZE * 2) {
            region_map[i].danger_level = 1;  // �е�Σ��
        } else {
            region_map[i].danger_level = 0;  // ��ȫ
        }
    }
}

// �����������򼶱��·���滮��
int find_best_target_region(struct Player *player, int current_x, int current_y) {
    if (!region_map_initialized) return -1;
    
    update_region_info(player);
    
    int best_region = -1;
    float best_score = -1000.0f;
    
    for (int i = 0; i < region_count; i++) {
        float score = 0.0f;
        
        // ���Ǽ�ֵ
        score += region_map[i].star_count * 15.0f;
        score += region_map[i].power_star_count * 200.0f;
        
        // ��������
        int dist_to_region = abs(current_x - region_map[i].center_x) + abs(current_y - region_map[i].center_y);
        score += 100.0f / (dist_to_region + 1);
        
        // Σ�ճͷ�
        if (player->your_status <= 0) {  // ��ǿ��״̬
            score -= region_map[i].danger_level * 30.0f;
                }
                
                if (score > best_score) {
                    best_score = score;
            best_region = i;
        }
    }
    
    return best_region;
}

// ���������ֲ�·���滮����������
int hierarchical_pathfinding(struct Player *player, int sx, int sy, int tx, int ty, int *next_x, int *next_y) {
    int map_size = player->row_cnt * player->col_cnt;
    
    // С��ͼֱ��ʹ��A*
    if (map_size <= LARGE_MAP_THRESHOLD) {
        return astar_limited(player, sx, sy, tx, ty, next_x, next_y);
    }
    
    // ���ͼʹ�÷ֲ�滮
    initialize_region_map(player);
    
    int source_region = get_region_index(player, sx, sy);
    int target_region = get_region_index(player, tx, ty);
    
    if (source_region == -1 || target_region == -1) {
        return 0;  // ��Ч����
    }
    
    // �����ͬһ����ֱ��ʹ��A*
    if (source_region == target_region) {
        return astar_limited(player, sx, sy, tx, ty, next_x, next_y);
    }
    
    // ��ͬ�����ȹ滮��Ŀ������ı߽�
    int target_center_x = region_map[target_region].center_x;
    int target_center_y = region_map[target_region].center_y;
    
    // �ҵ�Ŀ������ı߽�㣨����Դ��ı߽磩
    int boundary_x = target_center_x;
    int boundary_y = target_center_y;
    
    // �򻯱߽����㣺��Դ�㷽���ƶ�
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
    
    // ȷ���߽����Ч
    if (boundary_x < 0) boundary_x = 0;
    if (boundary_x >= player->row_cnt) boundary_x = player->row_cnt - 1;
    if (boundary_y < 0) boundary_y = 0;
    if (boundary_y >= player->col_cnt) boundary_y = player->col_cnt - 1;
    
    // ʹ��A*�滮���߽��
    return astar_limited(player, sx, sy, boundary_x, boundary_y, next_x, next_y);
}

//������������0 - ��ǿ��״̬����Զ����ԡ�
int handle_non_power_escape_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    if (player->your_status != 0) return 0;  // ֻ�����ǿ��״̬
    
    int min_ghost_dist = distance_to_ghost(player, x, y);
    
    // ��������ƻ���̫������Ҫ����Զ��
    if (min_ghost_dist <= 3) {
        // �����Է�AI����������ǵľ��루���Ǿ������أ�
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
        
        // ����Զ�룺ѡ���ȫ���ƶ�����
        int best_escape_dir = -1;
        int max_escape_value = -1000;
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny)) {
                int escape_value = 0;
                
                // ������ȫ��ֵ�������ƻ���ԽԶԽ��
                int new_ghost_dist = distance_to_ghost(player, nx, ny);
                escape_value += new_ghost_dist * 30;
                
                // ������·���������ƶ�ѡ��
                int exit_count = 0;
                for (int dd = 0; dd < 4; dd++) {
                    int check_x = nx + dx[dd], check_y = ny + dy[dd];
                    if (is_valid(player, check_x, check_y)) {
                        exit_count++;
                    }
                }
                escape_value += exit_count * 10;  // ��·Խ��Խ��ȫ
                
                // ������λ�������ǣ��ʶȽ���������ȫ���ȣ�
                char cell = player->mat[nx][ny];
                if (cell == 'o') {
                    escape_value += 15;  // ��ͨ����С����
                } else if (cell == 'O') {
                    escape_value += 25;  // �������еȽ���
                    
                    // ��Ҫ��������ֵľ�����������ָ������򽵵ͼ�ֵ
                    if (nearest_power_star_dist_to_opponent != INF) {
                        int our_dist_to_power = abs(nx - power_star_cache[0].x) + abs(ny - power_star_cache[0].y);  // �򻯼���
                        if (our_dist_to_power > nearest_power_star_dist_to_opponent + 2) {
                            escape_value -= 20;  // ���ָ������ƣ����ͼ�ֵ
                        }
                    }
                }
                
                // ������
                if (nx == last_x && ny == last_y) {
                    escape_value -= 50;  // �ͷ���ͷ
                }
                
                // ������ʱҲ���Ƕ��ָ��Ż���
                int our_safety = assess_danger_level(player, nx, ny);
                if (our_safety <= 1) {  // ��԰�ȫʱ
                    float interference_value = evaluate_opponent_interference(player, x, y, nx, ny);
                    if (interference_value > 0) {
                        escape_value += (int)(interference_value * 0.5);  // ����ʱ���͸���Ȩ��
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
            return 1;  // �ɹ�����
        }
    }
    
    return 0;  // δ����
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
        
        // ������ַ�ֵ��һ�� vs �ƻ��ߵ÷�(200��)�ıȽ�
        float opponent_half_score = opponent_score * 0.5f;
        float ghost_score = 200.0f;
        
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
    
    // ���ȼ��жϣ����ּ�ֵ vs �ƻ��߼�ֵ
    if (should_hunt_opponent && (opponent_hunt_value > 200.0f || min_ghost_dist > 8)) {
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
                if (hierarchical_pathfinding(player, x, y, nearest_power_star_x, nearest_power_star_y, next_x, next_y)) {
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
            if (hierarchical_pathfinding(player, x, y, nearest_star_x, nearest_star_y, next_x, next_y)) {
                return 1;  // �ɹ�����
            }
        }
    }
    
    return 0;  // δ����
}

//������������2 - ���������ռ����ԡ�
int handle_star_collection_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    build_star_cache(player);
    
    int best_star_x = -1, best_star_y = -1;
    float best_score = -1000.0;
    int danger_level = assess_danger_level(player, x, y);
    
    // ���ȿ��ǳ�����
    for (int k = 0; k < power_star_cache_size; k++) {
        int star_x = power_star_cache[k].x, star_y = power_star_cache[k].y;
        int star_danger = assess_danger_level(player, star_x, star_y);
        int star_dist = abs(x - star_x) + abs(y - star_y);
        
        // �������ж��߼�
        int should_consider_power_star = 0;
        
        // ��Դ�������Լ��
        float blocking_value = evaluate_resource_blocking(player, x, y, star_x, star_y);
        if (blocking_value > 50.0) {
            should_consider_power_star = 1;
        }
        
        // ������ȫ����
        if (star_danger < 2) {
            should_consider_power_star = 1;
        }
        // ����ͬ��ɱ����
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
    
    // ������ͨ���ǣ�ֻ����û�кõĳ�����ʱ��
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
    
    // ִ�������ռ��ƶ�
    if (best_star_x != -1) {
        if (hierarchical_pathfinding(player, x, y, best_star_x, best_star_y, next_x, next_y)) {
            return 1;  // �ɹ�����
        }
    }
    
    return 0;  // δ����
}

//������������3 - ��ȫ�ƶ����ԡ�
int handle_safe_movement_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    return choose_safe_direction_with_oscillation_check(player, x, y, next_x, next_y);
}

//������������4 - ����̽�����ԡ�
int handle_intelligent_exploration_strategy(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_x = x, best_y = y;
    int max_score = -1000;
    
    // �����ģʽ
    int oscillation_type = detect_path_oscillation(x, y);
    
    // �����⵽�𵴣����ȳ��Դ�����
    if (oscillation_type > 0) {
        int break_x, break_y;
        if (break_oscillation_pattern(player, x, y, &break_x, &break_y)) {
            // ��֤�����𵴵��ƶ��Ƿ�ȫ
            int ghost_dist = distance_to_ghost(player, break_x, break_y);
            int danger = assess_danger_level(player, break_x, break_y);
            
            if (danger < 2 || ghost_dist > 2) {
                *next_x = break_x;
                *next_y = break_y;
                return 1;  // �ɹ�����
            }
        }
    }
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            int ghost_dist = distance_to_ghost(player, nx, ny);
            int explore_score = ghost_dist * 3;
            
            // ��ǿ����ʷλ�óͷ�
            int history_penalty = 0;
            for (int i = 0; i < path_history_count; i++) {
                if (path_history_x[i] == nx && path_history_y[i] == ny) {
                    int recency = path_history_count - i;
                    history_penalty += recency * 20;
                }
            }
            explore_score -= history_penalty;
            
            // ǿ�����𵴣�����ͷ���ͷ
            if (nx == last_x && ny == last_y) {
                explore_score -= 150;
            }
            
            // ���ܷ���ѡ��
            int center_x = player->row_cnt / 2;
            int center_y = player->col_cnt / 2;
            int dist_from_center = abs(nx - center_x) + abs(ny - center_y);
            
            // ����������ѡ��ͬ����
            if (oscillation_type >= 2) {
                // �����𵴣�ǿ��ѡ��ֱ��ˮƽ����
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
                    if (d == 0 || d == 2) {  // �����ƶ�
                        explore_score += 300;
                    }
                } else {
                    if (d == 1 || d == 3) {  // �����ƶ�
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
            
            // ��Ե̽������
            if (nx < 2 || nx >= player->row_cnt - 2 || ny < 2 || ny >= player->col_cnt - 2) {
                explore_score += 15;
            }
            
            // ����̽�����⽱��
            if ((nx <= 1 || nx >= player->row_cnt - 2) && (ny <= 1 || ny >= player->col_cnt - 2)) {
                explore_score += 25;
            }
            
            // ���ָ��Ų�������
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
    
    // ����ȷ������Ч�ƶ�
    if (best_x == x && best_y == y) {
        // ������������ѡ��һ����Ч�ƶ����ǻ�ͷ��
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !(nx == last_x && ny == last_y)) {
                best_x = nx;
                best_y = ny;
                break;
            }
        }
        
        // ���ʵ��û�У�ѡ��������Ч�ƶ�
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
    return 1;  // ���ǳɹ�����
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
    
    // ����0����ǿ��״̬�µ�����Զ�����
    if (handle_non_power_escape_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; 
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����1��ǿ��״̬�µĻ���׷����������ȼ���
    if (handle_power_hunt_strategy(player, x, y, &next_x, &next_y)) {
    last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
    return ret;
}

    // ����2�����������ռ�
    if (handle_star_collection_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����3����ȫ�ƶ�����û�п��ռ�������ʱ��
    if (handle_safe_movement_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����4������̽����ȷ���������ƶ��������𵴣�
    if (handle_intelligent_exploration_strategy(player, x, y, &next_x, &next_y)) {
        last_x = x; last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
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