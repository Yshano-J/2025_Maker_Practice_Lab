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

// A* �ڵ�ṹ
struct AStarNode {
    int x, y;          // �ڵ��ڵ�ͼ�е�����
    int g_cost;        // ����㵽��ǰ�ڵ��ʵ�ʾ���
    int h_cost;        // ��������ֵ����Ŀ��Ĺ��ƾ��룩
    int f_cost;        // g_cost + h_cost���������ɱ���
    int parent_x, parent_y;  // ���ڵ����꣨����·�����ݣ�
    int in_open;       // �Ƿ��ڿ����б��У��ѷ��֣���̽����
    int in_closed;     // �Ƿ��ڹر��б��У���̽�������ٿ��ǣ�
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

// �Ľ�����������������ǽ��Ӱ��
int heuristic(struct Player *player, int x1, int y1, int x2, int y2) {
    int manhattan = abs(x1 - x2) + abs(y1 - y2);
    
    // �̾���ʱ�����پ����׼ȷ
    if (manhattan <= 3) {
        return manhattan;
    }
    
    // ���ֱ��·���ϵ�ǽ���ܶ�
    int wall_penalty = 0;
    int dx = (x2 > x1) ? 1 : ((x2 < x1) ? -1 : 0);
    int dy = (y2 > y1) ? 1 : ((y2 < y1) ? -1 : 0);
    
    int steps = (abs(x2 - x1) > abs(y2 - y1)) ? abs(x2 - x1) : abs(y2 - y1);
    int wall_count = 0;
    
    for (int i = 1; i <= steps && i <= 10; i++) {  // ֻ���ǰ10����������ȼ���
        int check_x = x1 + dx * i;
        int check_y = y1 + dy * i;
        
        if (check_x >= 0 && check_x < player->row_cnt && 
            check_y >= 0 && check_y < player->col_cnt) {
            if (player->mat[check_x][check_y] == '#') {
                wall_count++;
            }
        }
    }
    
    // ����ǽ�����������������
    wall_penalty = wall_count * 2;  // ÿ��ǽ����2�ĳͷ�
    
    return manhattan + wall_penalty;
}

// ���㵽�ƻ��ߵ���̾��루���ڷ���������
int distance_to_ghost(struct Player *player, int x, int y) {
    int min_dist = INF;
    for (int i = 0; i < 2; i++) {
        int dist = heuristic(player, x, y, player->ghost_posx[i], player->ghost_posy[i]);
        if (dist < min_dist) min_dist = dist;
    }
    return min_dist;
}

// �������ǵļ�ֵ��̰������������
float evaluate_star(struct Player *player, int sx, int sy, int star_x, int star_y) {
    char star_type = player->mat[star_x][star_y];
    int star_value = (star_type == 'O') ? 15 : 10;  // �����Ǹ��м�ֵ
    
    int distance = heuristic(player, sx, sy, star_x, star_y);
    int ghost_dist = distance_to_ghost(player, star_x, star_y);
    int opponent_dist = heuristic(player, star_x, star_y, player->opponent_posx, player->opponent_posy);
    
    // ���ͷ��ճͷ���������ȱ���
    float danger_penalty = 0;
    if (ghost_dist <= 2 && player->your_status <= 0) {
        danger_penalty = 2.0 / (ghost_dist + 1);  // ���ͳͷ�
    }
    
    // ��������
    float competition_penalty = 0;
    if (opponent_dist < distance && player->opponent_status != -1) {
        competition_penalty = 1.0;  // ���;����ͷ�
    }
    
    // ǿ��״̬����
    float power_bonus = 0;
    if (player->your_status > 0) {
        power_bonus = 3.0;  // ǿ��״̬�¸�����
        if (star_type == 'O') {
            // ǿ��״̬�³����ǵĴ������
            if (player->your_status <= 3) {
                // ǿ���������������������ȼ�����
                power_bonus += 8.0;
            } else if (player->your_status <= 8) {
                // ǿ��״̬���ڣ����������ȼ���
                power_bonus += 5.0;
            } else {
                // ǿ��״̬���㣬���������ȼ�����
                power_bonus += 2.0;
            }
        }
    } else {
        // ��ͨ״̬�£����������ȼ�����
        if (star_type == 'O') {
            power_bonus += 4.0;
        }
    }
    
    // ���뽱�������Ƚ�����Ŀ��
    float distance_bonus = 0;
    if (distance <= 3) distance_bonus = 2.0;
    
    // �ۺ����֣���ֵ�ܶ� - ���� - ���� + ����
    return (float)star_value / (distance + 1) - danger_penalty - competition_penalty + power_bonus + distance_bonus;
}

// BFS����Ѱ·���򵥵��ɿ���
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
            // �ҵ����ǣ����ݵ�һ��
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

// ��С�Ѳ���
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

// A* Ѱ·�㷨
int Astar(struct Player *player, int sx, int sy, int tx, int ty, int *next_x, int *next_y) {
    // ��ʼ���ڵ�
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
    
    // ���
    nodes[sx][sy].g_cost = 0;
    nodes[sx][sy].f_cost = nodes[sx][sy].h_cost;
    nodes[sx][sy].in_open = 1;
    
    open_size = 0;
    heap_push(sx * MAXN + sy); // ���ֱ�Ӵ洢����ԣ��Ѳ�����ܸ��ӣ�������һ��һά�������洢
    
    while (open_size > 0) {
        int current_idx = heap_pop();
        int x = current_idx / MAXN, y = current_idx % MAXN;
        
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
    
    return 0;  // �Ҳ���·��
}

// �ж��Ƿ�Ӧ�ò��ƶ����ȴ����ԣ�
int should_stay(struct Player *player, int x, int y) {
    // 1. Σ�չ�ܣ���Χ�����ƻ��ߣ����ƶ�����ȫ
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
    
    // ֻ���ڼ���Σ�յ�����²Ų��ƶ�
    if (player->your_status <= 0 && current_ghost_dist <= 1 && safe_moves == 0 && total_moves > 0) {
        return 1;  // ���ƶ�����ȫ
    }
    
    // 2. �ȴ����֣��������ϸ�ֻ���ڷǳ����Ե�����²ŵȴ�
    int opponent_closer_count = 0;
    int total_stars = 0;
    
    for (int i = 0; i < player->row_cnt; i++) {
        for (int j = 0; j < player->col_cnt; j++) {
            if (is_star(player, i, j)) {
                total_stars++;
                int my_dist = heuristic(player, x, y, i, j);
                int opponent_dist = heuristic(player, i, j, player->opponent_posx, player->opponent_posy);
                
                // �������Ը���������
                if (opponent_dist < my_dist - 3 && my_dist <= 4) {
                    opponent_closer_count++;
                }
            }
        }
    }
    
    // ֻ�е��������Ը����󲿷�����ʱ�ŵȴ�
    if (total_stars > 0 && opponent_closer_count >= total_stars / 2 && 
        player->opponent_status != -1 && total_stars <= 3) {
        return 1;  // �ȴ�������ȥ
    }
    
    // 3. ǿ��״̬����ֻ���ڻغ��������Ұ�ȫ������²ŵȴ�
    if (player->your_status >= 10) {  // ��ߵ�10�غ�����
        // ����Ƿ��г������ڸ������Ҷ��־����Զ
        for (int i = x - 2; i <= x + 2; i++) {
            for (int j = y - 2; j <= y + 2; j++) {
                if (i >= 0 && i < player->row_cnt && j >= 0 && j < player->col_cnt) {
                    if (player->mat[i][j] == 'O' && !star_eaten[i][j]) {
                        int my_dist = heuristic(player, x, y, i, j);
                        int opponent_dist = heuristic(player, i, j, player->opponent_posx, player->opponent_posy);
                        
                        // �������ϸ񣺾���ܽ������ֺ�Զ��ǿ��״̬�ܳ���
                        if (my_dist <= 1 && opponent_dist > my_dist + 5 && player->your_status >= 15) {
                            return 1;  // �ȴ�ǿ����������ȥ�ó�����
                        }
                    }
                }
            }
        }
    }
    
    return 0;  // �����ƶ�
}

struct Point walk(struct Player *player) {
    int x = player->your_posx, y = player->your_posy;
    
    // ��ǵ�ǰλ�õ�����Ϊ�ѳ�
    if (is_star(player, x, y)) {
        star_eaten[x][y] = 1;
    }
    
    // ����Ƿ�Ӧ�ò��ƶ�
    if (should_stay(player, x, y)) {
        struct Point ret = {x, y};  // ���ƶ������ص�ǰλ��
        return ret;
    }
    
    // Ѱ������Ŀ������
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
    
    // ����ҵ�Ŀ�����ǣ�������A*Ѱ·
    if (best_star_x != -1) {
        int next_x, next_y;
        if (Astar(player, x, y, best_star_x, best_star_y, &next_x, &next_y)) {
            // �ٴμ���ƶ����λ���Ƿ�ȫ
            int move_ghost_dist = distance_to_ghost(player, next_x, next_y);
            int current_ghost_dist = distance_to_ghost(player, x, y);
            
            // ����ƶ��������Ǹ��ӽ��ƻ�����û��ǿ��״̬�����ǲ��ƶ�
            if (player->your_status <= 0 && move_ghost_dist < current_ghost_dist && 
                move_ghost_dist <= 2 && best_score < 5.0) {
                struct Point ret = {x, y};  // ���ƶ�
                return ret;
            }
            
            struct Point ret = {next_x, next_y};
            return ret;
        }
    }
    
    // A*ʧ�ܣ�����BFS����������
    int next_x, next_y;
    if (bfs_backup(player, x, y, &next_x, &next_y)) {
        struct Point ret = {next_x, next_y};
        return ret;
    }
    
    // û�����ǿɴ����̽������
    int explore_x = x, explore_y = y;
    int max_score = -1000;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (is_valid(player, nx, ny)) {
            int ghost_dist = distance_to_ghost(player, nx, ny);
            int explore_score = ghost_dist;  // ����Զ���ƻ���
            
            // ���������ͼ�����ƶ�
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
    
    // ���̽���÷ֺܵͣ�ѡ���ƶ�
    if (max_score < 0) {
        struct Point ret = {x, y};  // ���ƶ�
        return ret;
    }
    
    struct Point ret = {explore_x, explore_y};
    return ret;
}
