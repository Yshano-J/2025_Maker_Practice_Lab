#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/playerbase.h"

#define MAXN 70
#define INF 1000000

// �Ż���ĳ��� - ƽ�����ܺ��ȶ���
#define SUPER_STAR_VALUE 1000.0f     // �����Ǽ�ֵ
#define NORMAL_STAR_VALUE 100.0f     // ��ͨ�Ǽ�ֵ
#define GHOST_HUNT_VALUE 600.0f      // ���׷����ֵ

static int star_eaten[MAXN][MAXN];
static int last_x = -1, last_y = -1;
static int step_count = 0;
static int opponent_x = -1, opponent_y = -1;

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

// ��ʼ������
void init(struct Player *player) {
    memset(star_eaten, 0, sizeof(star_eaten));
    last_x = -1;
    last_y = -1;
    step_count = 0;
    opponent_x = -1;
    opponent_y = -1;
}

// ������֤����
int is_valid(struct Player *player, int x, int y) {
    return x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt && player->mat[x][y] != '#';
}

int is_star(struct Player *player, int x, int y) {
    char c = player->mat[x][y];
    return (c == 'o' || c == 'O') && !star_eaten[x][y];
}

// �޸��Ķ���λ�ø��� - ����ȫ�ɿ�
void update_opponent_quick(struct Player *player) {
    opponent_x = -1;
    opponent_y = -1;
    
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            char c = player->mat[i][j];
            // ����Ƿ�Ϊ����λ�ã��ų��Լ��͹�꣩
            if (c != 'O' && c != 'o' && c != '#' && c != '.') {
                if (!(i == player->your_posx && j == player->your_posy)) {
                    // �ų����λ��
                    int is_ghost = 0;
                    for (int k = 0; k < 2; k++) {
                        if (i == player->ghost_posx[k] && j == player->ghost_posy[k]) {
                            is_ghost = 1;
                            break;
                        }
                    }
                    if (!is_ghost) {
                        opponent_x = i;
                        opponent_y = j;
                        return;
                    }
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

// �޸���BFS�㷨 - ��ǿ�߽���Ͱ�ȫ��
int simple_bfs(struct Player *player, int sx, int sy, int target_x, int target_y, int *next_x, int *next_y) {
    // �߽���
    if (sx < 0 || sx >= player->row_cnt || sy < 0 || sy >= player->col_cnt) return 0;
    if (target_x < 0 || target_x >= player->row_cnt || target_y < 0 || target_y >= player->col_cnt) return 0;
    if (sx == target_x && sy == target_y) return 0;
    
    int vis[MAXN][MAXN] = {0};
    int parent_x[MAXN][MAXN], parent_y[MAXN][MAXN];
    int qx[500], qy[500], head = 0, tail = 0;
    
    // ��ʼ��
    qx[tail] = sx;
    qy[tail] = sy;
    tail++;
    vis[sx][sy] = 1;
    parent_x[sx][sy] = -1;
    parent_y[sx][sy] = -1;
    
    int max_iterations = 120; // ���ٵ��������������
    int iterations = 0;
    
    while (head < tail && iterations < max_iterations) {
        int x = qx[head], y = qy[head];
        head++;
        iterations++;
        
        if (x == target_x && y == target_y) {
            // ��ȫ����
            int px = x, py = y;
            int backtrack_count = 0;
            while ((parent_x[px][py] != sx || parent_y[px][py] != sy) && backtrack_count < 100) {
                backtrack_count++;
                int temp_x = parent_x[px][py];
                int temp_y = parent_y[px][py];
                if (temp_x == -1 || temp_y == -1) break; // ��ֹ��Ч���ڵ�
                px = temp_x;
                py = temp_y;
            }
            if (backtrack_count >= 100) return 0; // ��ֹ��ѭ��
            
            *next_x = px;
            *next_y = py;
            return 1;
        }
        
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (is_valid(player, nx, ny) && !vis[nx][ny] && tail < 499) {
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

// �򻯵������������� - ���⸴�Ӽ��㵼�³�ʱ
float evaluate_star_simple(struct Player *player, int sx, int sy, int star_x, int star_y) {
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
        value -= 600.0f;
    }
    
    // �����Ƕ��⽱��
    if (star_type == 'O' && player->your_status <= 0) {
        value += 400.0f;
    }
    
    // С��ͼ����
    if (player->row_cnt <= 12 && player->col_cnt <= 12) {
        value += 100.0f;
    }
    
    // ������ʶ
    if (opponent_x != -1) {
        int opp_dist = abs(star_x - opponent_x) + abs(star_y - opponent_y);
        if (my_dist < opp_dist && star_type == 'O') {
            value += 300.0f;
        }
    }
    
    return value;
}

// ��������
int find_best_star(struct Player *player, int sx, int sy, int *best_x, int *best_y) {
    float best_value = -INF;
    int found = 0;
    
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                float value = evaluate_star_simple(player, sx, sy, i, j);
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

// ���׷��
int hunt_ghost(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
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
    
    // ׷������
    if (player->your_status >= 3 || min_dist <= 8) {
        return simple_bfs(player, sx, sy, 
            player->ghost_posx[target_ghost], 
            player->ghost_posy[target_ghost], 
            next_x, next_y);
    }
    
    return 0;
}

// ��ȫ�ƶ�
int safe_move(struct Player *player, int sx, int sy, int *next_x, int *next_y) {
    int best_dir = -1;
    float max_score = -1;
    
    for (int d = 0; d < 4; d++) {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (!is_valid(player, nx, ny)) continue;
        
        float score = distance_to_ghost(player, nx, ny);
        
        // ���Ǽӷ�
        if (is_star(player, nx, ny)) {
            score += (player->mat[nx][ny] == 'O') ? 15 : 8;
        }
        
        // �ƶ�������
        int valid_exits = 0;
        for (int dd = 0; dd < 4; dd++) {
            if (is_valid(player, nx + dx[dd], ny + dy[dd])) {
                valid_exits++;
            }
        }
        if (valid_exits == 1) {
            score -= 5.0f;  // �ͷ���·
        } else if (valid_exits >= 3) {
            score += 1.0f;  // �������ƶ���
        }
        
        if (score > max_score) {
            max_score = score;
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

// �����ߺ���
struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    step_count++;
    
    // ���¶���λ��
    update_opponent_quick(player);
    
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
    
    // ����2: �ռ�����
    int target_x, target_y;
    if (find_best_star(player, x, y, &target_x, &target_y)) {
        if (simple_bfs(player, x, y, target_x, target_y, &next_x, &next_y)) {
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
    
    // ��������
    last_x = x;
    last_y = y;
    struct Point ret = {x, y};
    return ret;
} 