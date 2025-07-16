#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// **�޸���ˢ��ר�ó���** - �����صĲ�������
#define SUPER_STAR_VALUE 1200.0f     // ���˵��ȶ�ֵ
#define NORMAL_STAR_VALUE 100.0f     // ���˵��ȶ�ֵ
#define GHOST_HUNT_VALUE 600.0f      // ���˵��ȶ�ֵ

// **�޸���ˢ��ר�ó���**
#define AGGRESSIVE_MODE_THRESHOLD 8000  // ��߼���ģʽ�ż�
#define SAFETY_DISTANCE_REDUCTION 1     // ���ټ����̶�
#define BFS_SEARCH_DEPTH 150            // ���˵��ȶ����
#define STAR_CLUSTER_BONUS 200.0f       // ���ͼ�Ⱥ����

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;
static int aggressive_mode = 0;

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// **�޸����ʼ������**
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
    aggressive_mode = 0;
}

// **�޸����������** - ��ǿ�߽���
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

// **�޸������λ�ø���**
void update_opponent_quick(struct Player *player) {
    if (player == NULL) return;
    opponent_x = player->opponent_posx;
    opponent_y = player->opponent_posy;
    
    // **�޸�**: �����صļ���ģʽ
    aggressive_mode = (player->your_score >= AGGRESSIVE_MODE_THRESHOLD);
}

// **�޸���������** - ��ֹ���
int distance_to_ghost(struct Player *player, int x, int y) {
    if (player == NULL) return INF;
    
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_dist) {
            min_dist = dist;
        }
    }
    
    // **�޸�**: �Ƴ����ܵ�������Ĳ���
    return min_dist;
}

// **�޸���BFS�㷨** - ��ǿ�ȶ���
int simple_bfs(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    if (player == NULL || next_x == NULL || next_y == NULL) return 0;
    
    // �߽���
    if (sx < 0 || sx >= player->row_cnt || sy < 0 || sy >= player->col_cnt) return 0;
    if (target_x < 0 || target_x >= player->row_cnt || target_y < 0 || target_y >= player->col_cnt) return 0;
    if (sx == target_x && sy == target_y) return 0;
    
    int vis[MAXN][MAXN] = {0};
    int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    int qx[500], qy[500], head = 0, tail = 0;  // **�޸�**: ���˵��ȶ����д�С
    
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
            // ��ȫ���ݣ��ҵ�����㵽Ŀ��ĵ�һ��
            int px = x, py = y;
            int backtrack_count = 0;
            
            // ���ݵ���㣬�ҵ���һ��
            while ((parent_x[px][py] != sx || parent_y[px][py] != sy) && backtrack_count < 100) {
                backtrack_count++;
                int temp_x = parent_x[px][py];
                int temp_y = parent_y[px][py];
                if (temp_x == -1 || temp_y == -1) break;
                px = temp_x;
                py = temp_y;
            }
            
            if (backtrack_count >= 100) return 0;
            
            // ������ݳɹ������ص�һ��
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

// **�޸������Ǽ�Ⱥ��⺯��**
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

// **�޸���������������** - ��ֹ��ֵ���
float evaluate_star_enhanced(struct Player *player, int sx, int sy, int star_x, int star_y) {
    if (player == NULL) return -INF;
    
    char star_type = player->mat[star_x][star_y];
    int my_dist = abs(star_x - sx) + abs(star_y - sy);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    
    // **�޸�**: ������ֵ���˵��ȶ�ֵ
    float value = (star_type == 'O') ? SUPER_STAR_VALUE : NORMAL_STAR_VALUE;
    
    // **�޸�**: �����صķ����������
    if (player->your_score < 2000) {
        value *= 2.0f;  // ���ͱ���
    } else if (player->your_score < 4000) {
        value *= 1.5f;
    } else if (player->your_score < 8000) {
        value *= 1.2f;
    } else if (aggressive_mode) {
        value *= 1.1f;  // ���ͼ���ģʽ����
    }
    
    // **�޸�**: ����ͷ�
    value -= my_dist * 5.0f;
    
    // **�޸�**: ��ȫ����
    int safety_threshold = aggressive_mode ? 2 : 3;
    if (ghost_dist > safety_threshold) {
        value += 200.0f;
    } else if (ghost_dist <= 1 && player->your_status <= 0) {
        value -= 800.0f;
    }
    
    // **�޸�**: �����ǽ���
    if (star_type == 'O' && player->your_status <= 0) {
        value += 400.0f;
    }
    
    // **�޸�**: ��ͼ��������
    if (player->row_cnt <= 12 && player->col_cnt <= 12) {
        value += 150.0f;
    }
    
    // **�޸�**: ���Ǽ�Ⱥ����
    int nearby_stars = count_nearby_stars(player, star_x, star_y, 2);
    if (nearby_stars > 1) {
        value += STAR_CLUSTER_BONUS * (nearby_stars - 1);
    }
    
    // **�޸�**: ������ʶ
    if (opponent_x != -1 && opponent_y != -1) {
        int opp_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        if (my_dist < opp_dist && star_type == 'O') {
            value += 400.0f;
        } else if (my_dist - opp_dist > 6) {
            value *= 0.8f;
        }
    }
    
    // **�޸�**: ǿ��״̬����
    if (player->your_status > 0) {
        value += 200.0f;  // ���ͽ���
    }
    
    return value;
}

// **�޸�����������**
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

// **�޸�����׷��**
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
    
    // **�޸�**: �����ص�׷������
    int hunt_threshold = aggressive_mode ? 10 : 8;
    if (player->your_status >= 3 || min_dist <= hunt_threshold) {
        return simple_bfs(player, sx, sy, 
                         player->ghost_posx[target_ghost], 
                         player->ghost_posy[target_ghost], 
                         next_x, next_y);
    }
    
    return 0;
}

// **�޸��氲ȫ�ƶ�**
int safe_move(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (player == NULL || next_x == NULL || next_y == NULL) return 0;
    
    int best_dir = -1;
    int max_safety = -1;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        int safety = distance_to_ghost(player, nx, ny);
        
        // **�޸�**: ���Ǽӷ�
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

// **�޸��������ߺ���** - ��ǿ�ȶ���
struct Point walk(struct Player *player) {
    if (player == NULL) {
        struct Point ret = {0, 0};
        return ret;
    }
    
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // ״̬����
    update_opponent_quick(player);
    
    // ����״̬����
    if (is_star(player, x, y) && x >= 0 && x < MAXN && y >= 0 && y < MAXN) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // **����1**: ���׷��
    if (player->your_status > 0) {
        if (hunt_ghost(player, x, y, &next_x, &next_y)) {
            last_x = x;
            last_y = y;
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // **����2**: �����ռ�
    int target_x, target_y;
    if (find_best_star(player, x, y, &target_x, &target_y)) {
        if (simple_bfs(player, x, y, target_x, target_y, &next_x, &next_y)) {
            // **�޸�**: �����صİ�ȫ����
            int required_dist = aggressive_mode ? 2 : 3;
            if (distance_to_ghost(player, next_x, next_y) >= required_dist || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // **����3**: ��ȫ�ƶ�
    if (safe_move(player, x, y, &next_x, &next_y)) {
        last_x = x;
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����ֶ�
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            last_x = x;
            last_y = y;
            struct Point ret = {nx, ny};
            return ret;
        }
    }
    
    // ��������
    last_x = x;
    last_y = y;
    struct Point ret = {x, y};
    return ret;
}

// **�޸���ˢ��ר���Ż�** - ��ǿ�ȶ���
// ��Ҫ�޸���
// 1. ��ǿ�߽���Ϳ�ָ����
// 2. ��ֹ��ֵ���
// 3. ���˵��ȶ��Ĳ�������
// 4. �����صĲ�������
// 5. ��ǿ������
// �����Ԥ��ͨ�����в�������
// ʱ�䣺2025-07-16 10:30:00
// ר��Ϊ�ȶ����Ż�