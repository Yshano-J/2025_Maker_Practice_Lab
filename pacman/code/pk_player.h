#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// �򻯵ĳ�������
#define POWER_TIME_LOW 3      // ǿ��ʱ�䲻����ֵ
#define MAX_BFS_ITER 50       // BFS�����������������������������

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// �򻯵ĳ�ʼ��
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
}

// ��������
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// ���㵽������̾���
int distance_to_ghost(struct Player *player, int x, int y) {
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = abs(x - player->ghost_posx[i]) + abs(y - player->ghost_posy[i]);
        if (dist < min_dist) min_dist = dist;
    }
    return min_dist;
}

// �򻯵İ�ȫ����
int is_safe(struct Player *player, int x, int y) {
    int ghost_dist = distance_to_ghost(player, x, y);
    
    if (player->your_status > 0) {
        // ǿ��״̬�����Խӽ����
        return 1;
    } else {
        // ��ͨ״̬����Ҫ���ְ�ȫ����
        return ghost_dist >= 2;
    }
}

// ��Ч����������
float evaluate_star_simple(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int distance = abs(sx - star_x) + abs(sy - star_y);
    int is_power_star = (star_type == 'O');
    
    float score = is_power_star ? 100.0f : 10.0f;
    
    // ����Խ��Խ��
    score = score * 20.0f / (distance + 1);
    
    // �������ڷ�ǿ��״̬�¼������ȼ�
    if (is_power_star && player->your_status <= 0) {
        score += 500.0f;
    }
    
    // ��ȫ�Լ��
    if (!is_safe(player, star_x, star_y)) {
        score -= 200.0f;
    }
    
    return score;
}

// �򻯵�BFSѰ���������
int bfs_to_best_star(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_star_x = -1, best_star_y = -1;
    float best_score = -1000.0f;
    
    // ���ٱ������������
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                float score = evaluate_star_simple(player, sx, sy, i, j);
                if (score > best_score) {
                    best_score = score;
                    best_star_x = i;
                    best_star_y = j;
                }
            }
        }
    }
    
    if (best_star_x == -1) return 0;  // û������
    
    // ���Ŀ�����Ǿ�������λ�ã�ֱ���ƶ�
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (nx == best_star_x && ny == best_star_y && is_valid(player, nx, ny)) {
            *next_x = nx;
            *next_y = ny;
            return 1;
        }
    }
    
    // �򻯵ķ������ƶ�
    int best_dir = -1;
    int min_dist = INF;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (is_valid(player, nx, ny)) {
            int dist = abs(nx - best_star_x) + abs(ny - best_star_y);
            if (dist < min_dist) {
                min_dist = dist;
                best_dir = d;
            }
        }
    }
    
    if (best_dir != -1) {
        *next_x = sx + dx[best_dir];
        *next_y = sy + dy[best_dir];
        return 1;
    }
    
    return 0;
}

// �򻯵Ĺ��׷��
int hunt_nearest_ghost(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    if (player->your_status <= 0) return 0;  // û��ǿ��״̬
    
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
    
    int ghost_x = player->ghost_posx[target_ghost];
    int ghost_y = player->ghost_posy[target_ghost];
    
    // ��׷����ѡ����ӽ����ķ���
    int best_dir = -1;
    int min_new_dist = INF;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (is_valid(player, nx, ny)) {
            int new_dist = abs(nx - ghost_x) + abs(ny - ghost_y);
            if (new_dist < min_new_dist) {
                min_new_dist = new_dist;
                best_dir = d;
            }
        }
    }
    
    if (best_dir != -1) {
        *next_x = sx + dx[best_dir];
        *next_y = sy + dy[best_dir];
        return 1;
    }
    
    return 0;
}

// ��ȫ�ƶ�ѡ��
int choose_safe_move(struct Player *player, int x, int y, int *next_x, int *next_y) {
    int best_dir = -1;
    int max_safety = -1;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        int safety = distance_to_ghost(player, nx, ny);
        
        // �����ͷ
        if (nx == last_x && ny == last_y) {
            safety -= 10;
        }
        
        // ���λ���������ǣ�����������
        if (is_star(player, nx, ny)) {
            char c = player->mat[nx][ny];
            safety += (c == 'O') ? 50 : 20;
        }
        
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
    
    return 0;
}

// �����ߺ���
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // ��ǵ�ǰ����Ϊ�ѳ�
    char current_cell = player->mat[x][y];
    if ((current_cell == 'o' || current_cell == 'O') && !star_eaten[x][y]) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // ����1: ǿ��״̬��׷�����
    if (player->your_status > 0) {
        int remaining_time = player->your_status;
        int nearest_ghost_dist = distance_to_ghost(player, x, y);
        
        // ���ʱ������ҹ�겻̫Զ������׷��
        if (remaining_time >= 5 || nearest_ghost_dist <= 6) {
            if (hunt_nearest_ghost(player, x, y, &next_x, &next_y)) {
                last_x = x; 
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
        
        // ʱ�䲻��ʱ�ռ���������
        if (remaining_time <= POWER_TIME_LOW) {
            if (bfs_to_best_star(player, x, y, &next_x, &next_y)) {
                last_x = x; 
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // ����2: �ռ��������
    if (bfs_to_best_star(player, x, y, &next_x, &next_y)) {
        last_x = x; 
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����3: ��ȫ�ƶ�
    if (choose_safe_move(player, x, y, &next_x, &next_y)) {
        last_x = x; 
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����ֶΣ����ѡ����Ч����
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            last_x = x; 
            last_y = y;
            struct Point ret = {nx, ny};
            return ret;
        }
    }
    
    // �޷��ƶ�������ԭλ��
    last_x = x; 
    last_y = y;
    struct Point ret = {x, y};
    return ret;
}