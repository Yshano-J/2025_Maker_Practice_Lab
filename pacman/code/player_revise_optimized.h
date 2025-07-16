#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// ΢����ĸ�Ч����
#define SUPER_STAR_VALUE 1000.0f     // ���������Ǽ�ֵ
#define NORMAL_STAR_VALUE 120.0f     // ΢����ͨ�Ǽ�ֵ
#define GHOST_HUNT_VALUE 700.0f      // ���׷����ֵ

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;  // ��Ӷ���׷��

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// ��ʼ��
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
}

// ��������
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// ����������λ�ø���
void update_opponent(struct Player *player) {
    for (int i = 0; i < player->row_cnt; i++) {
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

// �Ľ���BFSѰ· - ���ּ򵥵����Ƚ�
int smart_bfs(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    if (sx == target_x && sy == target_y) return 0;
    
    int vis[MAXN][MAXN] = {0};
    int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    int qx[400], qy[400], head = 0, tail = 0;
    
    qx[tail] = sx;
    qy[tail] = sy;
    tail++;
    vis[sx][sy] = 1;
    parent_x[sx][sy] = -1;
    parent_y[sx][sy] = -1;
    
    while (head < tail && head < 120) {  // ���е��������
        int x = qx[head], y = qy[head];
        head++;
        
        if (x == target_x && y == target_y) {
            // ��ȫ�����ҵ�һ��
            int px = x, py = y;
            int backtrack = 0;
            while (backtrack < 100 && parent_x[px][py] != -1) {
                int temp_x = parent_x[px][py];
                int temp_y = parent_y[px][py];
                if (temp_x == sx && temp_y == sy) {
                    *next_x = px;
                    *next_y = py;
                    return 1;
                }
                px = temp_x;
                py = temp_y;
                backtrack++;
            }
            return 0;
        }
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny] && tail < 399) {
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

// �Ľ����������� - ���Ӿ�����ʶ
float evaluate_star_improved(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int my_dist = abs(star_x - sx) + abs(star_y - sy);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    
    // ������ֵ
    float value = (star_type == 'O') ? SUPER_STAR_VALUE : NORMAL_STAR_VALUE;
    
    // ����ͷ�
    value -= my_dist * 5.0f;
    
    // ��ȫ����
    if (ghost_dist > 3) {
        value += 200.0f;
    } else if (ghost_dist <= 1 && player->your_status <= 0) {
        value -= 800.0f;  // Σ�ճͷ�
    }
    
    // �����Ƕ��⽱��
    if (star_type == 'O' && player->your_status <= 0) {
        value += 500.0f;  // ������������Ҫ��
    }
    
    // С��ͼ��������
    if (player->row_cnt <= 12 && player->col_cnt <= 12) {
        value += 200.0f;
    }
    
    // �������������
    if (opponent_x != -1) {
        int opp_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        if (my_dist < opp_dist && star_type == 'O') {
            value += 300.0f;  // ���ᳬ���ǽ���
        }
    }
    
    // �����������
    if (player->your_score < 3000) {
        value *= 1.5f;  // �ͷ�ʱ������
    }
    
    return value;
}

// Ѱ��������� - ��΢�Ż�
int find_best_star(struct Player *player, int sx, int sy, int *best_x, int *best_y) {
    float best_value = -INF;
    int found = 0;
    
    // ����������������
    int search_radius = 10;
    for (int i = max(0, sx - search_radius); i <= min(player->row_cnt - 1, sx + search_radius); i++) {
        for (int j = max(0, sy - search_radius); j <= min(player->col_cnt - 1, sy + search_radius); j++) {
            if (is_star(player, i, j)) {
                float value = evaluate_star_improved(player, sx, sy, i, j);
                if (value > best_value) {
                    best_value = value;
                    *best_x = i;
                    *best_y = j;
                    found = 1;
                }
            }
        }
    }
    
    // �������û�ҵ���������ȫͼ
    if (!found) {
        for (int i = 0; i < player->row_cnt; i++) {
            for (int j = 0; j < player->col_cnt; j++) {
                if (is_star(player, i, j)) {
                    float value = evaluate_star_improved(player, sx, sy, i, j);
                    if (value > best_value) {
                        best_value = value;
                        *best_x = i;
                        *best_y = j;
                        found = 1;
                    }
                }
            }
        }
    }
    
    return found;
}

// ���׷�� - ΢��
int hunt_ghost(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
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
    
    // �����ܵ�׷���ж�
    if (player->your_status >= 2 || min_dist <= 10) {
        return smart_bfs(player, sx, sy, 
                        player->ghost_posx[target_ghost], 
                        player->ghost_posy[target_ghost], 
                        next_x, next_y);
    }
    
    return 0;
}

// ��ȫ�ƶ� - ΢��
int safe_move(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_dir = -1;
    float max_safety = -INF;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        float safety = distance_to_ghost(player, nx, ny);
        
        // �����ǵ�λ�üӷ�
        if (is_star(player, nx, ny)) {
            safety += (player->mat[nx][ny] == 'O') ? 20.0f : 10.0f;
        }
        
        // �����ظ�λ��
        if (nx == last_x && ny == last_y) {
            safety -= 5.0f;
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

// �����ߺ��� - ���ּ�൫������
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // ������״̬����
    update_opponent(player);
    
    // ��������״̬
    if (is_star(player, x, y)) {
        star_eaten[x][y] = 1;
    }
    
    int next_x = x, next_y = y;
    
    // ����1: ǿ��״̬׷�����
    if (player->your_status > 0) {
        if (hunt_ghost(player, x, y, &next_x, &next_y)) {
            last_x = x;
            last_y = y;
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // ����2: �����ռ�����
    int target_x, target_y;
    if (find_best_star(player, x, y, &target_x, &target_y)) {
        if (smart_bfs(player, x, y, target_x, target_y, &next_x, &next_y)) {
            // ��̬��ȫ���
            int required_dist = (player->your_score < 3000) ? 1 : 2;
            if (distance_to_ghost(player, next_x, next_y) >= required_dist || player->your_status > 0) {
                last_x = x;
                last_y = y;
                struct Point ret = {next_x, next_y};
                return ret;
            }
        }
    }
    
    // ����3: ��ȫ�ƶ�
    if (safe_move(player, x, y, &next_x, &next_y)) {
        last_x = x;
        last_y = y;
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // ����ֶΣ�������Ч�ƶ�
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            last_x = x;
            last_y = y;
            struct Point ret = {nx, ny};
            return ret;
        }
    }
    
    // �޷��ƶ�
    last_x = x;
    last_y = y;
    struct Point ret = {x, y};
    return ret;
}

// ���ڼ򻯰�ľ�׼�Ż� - ���ָ�Ч���ĺ������� 
// �����ʧ��