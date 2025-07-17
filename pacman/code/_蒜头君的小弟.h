#ifndef PLAYER_H
#define PLAYER_H

#include <string.h>
#include <stdlib.h>
#include "../include/playerbase.h"

#define MAXN 200
#define MAXM 200
#define MAXQ (MAXN*MAXM)
#define SAFE_DIST_GHOST 1   // ��ͨ״̬���ƻ��ߵİ�ȫ����
#define SAFE_DIST_OPP 1     // ��ͨ״̬��ǿ�����ֵİ�ȫ����

// �ķ����ϡ��¡�����
static const int dirx[4] = {-1, 1, 0, 0};
static const int diry[4] = {0, 0, -1, 1};

// ȫ�� BFS �������飨����next�����¼��
static int qx[MAXQ], qy[MAXQ]; // ����
static int dist_map[MAXN][MAXM], prex[MAXN][MAXM], prey[MAXN][MAXM]; 
// dist_map�����¼���룻prex��prey�����¼ǰ���ڵ�
static int nextx[MAXN][MAXM], nexty[MAXN][MAXM];  // ��¼��㵽��i��j������һ��Ӧ���ߵ�λ��
static int ghost_dist[MAXN][MAXM]; // �����ƻ���

// ����������ӵ�����ƻ��ߵ���̾��루���ƻ���Ϊ���ģ���ǿ�ƻ���λ����Ч���жϣ�
static void compute_ghost_dist(struct Player *player) {
    int n = player->row_cnt, m = player->col_cnt;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            ghost_dist[i][j] = -1;

    int qh = 0, qt = 0;
    for (int g = 0; g < 2; ++g) {
        int gx = player->ghost_posx[g], gy = player->ghost_posy[g];
        // ����������ƻ���λ���Ƿ���ǽ�����ݹ����ʼ����ǽ�����ƶ�����ܣ�
        if (gx >= 0 && gx < n && gy >= 0 && gy < m && player->mat[gx][gy] != '#') {
            ghost_dist[gx][gy] = 0;
            qx[qt] = gx; qy[qt] = gy; ++qt;
        }
    }
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; ++qh;
        for (int d = 0; d < 4; ++d) {
            int nx = x + dirx[d], ny = y + diry[d];
            if (nx < 0 || nx >= n || ny < 0 || ny >= m) continue;
            if (player->mat[nx][ny] == '#') continue;
            if (ghost_dist[nx][ny] == -1) {
                ghost_dist[nx][ny] = ghost_dist[x][y] + 1;
                qx[qt] = nx; qy[qt] = ny; ++qt;
            }
        }
    }
}


// �ڳԶ�����Ϸ�У�������Ҫ�ҵ��ӵ�ǰλ�õ�Ŀ��λ�ã��糬���ǡ����֡��ƻ��ߵȣ������·������ͨ��ֻ��Ҫ֪����һ��Ӧ�����ĸ������ߣ�������Ҫ����·����
// ���������
// ʹ��nextx��nexty�����¼ÿ��λ�õ�������һ��λ�ã�
// nextx[i][j]������㵽λ��(i,j)·���ĵ�һ��x����
// nexty[i][j]������㵽λ��(i,j)·���ĵ�һ��y����
// ��ȡnext���飬��¼��㵽��i��j������һ��Ӧ���ߵ�λ��
// ͨ�� BFS ��䣨����next�����¼���Ż�·�����ݣ�
// ��ͨ״̬��ȫ�жϣ�ʹ�ò�������ȫ���룩
// �ƶ�֮�������ƻ��߾���С��SAFE_DIST_GHOST������������־���С��SAFE_DIST_OPP���Ͳ����ƶ�
static void bfs_fill(struct Player *player, int sx, int sy, int reset_pre) {
    int n = player->row_cnt, m = player->col_cnt;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            dist_map[i][j] = -1;
            if (reset_pre) {
                prex[i][j] = prey[i][j] = -1;
                nextx[i][j] = nexty[i][j] = -1;  // ��ʼ����һ������
            }
        }
    }
    int qh = 0, qt = 0;
    dist_map[sx][sy] = 0;
    qx[qt] = sx; qy[qt] = sy; ++qt;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; ++qh;
        for (int d = 0; d < 4; ++d) {
            int nx = x + dirx[d], ny = y + diry[d];
            if (nx < 0 || nx >= n || ny < 0 || ny >= m) continue;
            if (player->mat[nx][ny] == '#') continue;
            
            if (player->your_status == 0) {
                if (ghost_dist[nx][ny] >= 0 && ghost_dist[nx][ny] <= SAFE_DIST_GHOST) continue;
                if (player->opponent_status > 0) {
                    int dx = abs(nx - player->opponent_posx);
                    int dy = abs(ny - player->opponent_posy);
                    if (dx + dy <= SAFE_DIST_OPP) continue;
                }
            }
            if (dist_map[nx][ny] == -1) {
                dist_map[nx][ny] = dist_map[x][y] + 1;
                if (reset_pre) {
                    prex[nx][ny] = x;
                    prey[nx][ny] = y;
                    // �����������ھ�ֱ�Ӽ�¼��һ���������ڵ�̳и��ڵ����һ��
                    if (x == sx && y == sy) {  // ���ڵ�����㣬��ǰ�ڵ��������ھ�
                        nextx[nx][ny] = nx;     // ��һ�����ǵ�ǰ�ڵ㣨nx, ny��
                        nexty[nx][ny] = ny;
                    } else {                     // ���ڵ㲻����㣬�̳и��ڵ����һ��
                        nextx[nx][ny] = nextx[x][y];
                        nexty[nx][ny] = nexty[x][y];
                    }
                }
                qx[qt] = nx;
                qy[qt] = ny;
                ++qt;
            }
        }
    }
}

// ѡ����ѳ����ǣ�ʹ��next����ֱ�ӻ�ȡ��һ����
// ѡ�񳬼��ǿ��ǣ�
// 1. ���볬����Խ��Խ�ã�dԽСԽ��
// 2. �����ƻ���ԽԶԽ�ã�safetyԽ��Խ��
static struct Point choose_superstar(struct Player *player, int use_cached_bfs) {
    int sx = player->your_posx, sy = player->your_posy;
    if (!use_cached_bfs) {
        bfs_fill(player, sx, sy, 1); // �ǻ���ģʽ���¼���BFS
    }
    int best_x = -1, best_y = -1;
    int best_score = -1000000;
    for (int i = 0; i < player->row_cnt; ++i) {
        for (int j = 0; j < player->col_cnt; ++j) {
            if (player->mat[i][j] == 'O' && dist_map[i][j] >= 0) {
                int safety = ghost_dist[i][j];
                int d = dist_map[i][j];
                int score = safety * 100 - d * 10;
                if (score > best_score) {
                    best_score = score;
                    best_x = i; best_y = j;
                }
            }
        }
    }
    if (best_x < 0 || (best_x == sx && best_y == sy)) {
        return initPoint(sx, sy);  // Ŀ����������Ŀ��ʱ����ԭλ
    }
    // ֱ��ʹ��next�����ȡ��һ�������ԭ����ѭ����
    return initPoint(nextx[best_x][best_y], nexty[best_x][best_y]);
}

// ѡ�������ͨ�ǻ�з���ͷ��֧�ֻ���BFS��ʹ��next����
static struct Point choose_nearest_char(struct Player *player, char target, int use_cached_bfs) {
    int sx = player->your_posx, sy = player->your_posy;
    if (!use_cached_bfs) {
        bfs_fill(player, sx, sy, 1);  // �ǻ���ģʽ���¼���BFS
    }
    int best_x = -1, best_y = -1, best_d = MAXQ;
    for (int i = 0; i < player->row_cnt; ++i) {
        for (int j = 0; j < player->col_cnt; ++j) {
            if (dist_map[i][j] >= 0) {
                int ok = 0;
                if (target == 'E') {
                    // �����������ִ����Ϊ��ͨ״̬
                    if (player->opponent_status == 0 && player->opponent_status != -1 && 
                        i == player->opponent_posx && j == player->opponent_posy) {
                        ok = 1;
                    }
                } else if (player->mat[i][j] == target) ok = 1;

                if (ok && dist_map[i][j] < best_d) {
                    best_d = dist_map[i][j]; 
                    best_x = i; best_y = j;
                }
            }
        }
    }
    if (best_x < 0) return initPoint(sx, sy);
    // ֱ��ʹ��next�����ȡ��һ��
    return initPoint(nextx[best_x][best_y], nexty[best_x][best_y]);
}

// ѡ�����Ŀ���ƻ��ߣ�֧�ֻ���BFS��ʹ��next����
static struct Point choose_nearest_ghost(struct Player *player, int use_cached_bfs) {
    int sx = player->your_posx, sy = player->your_posy;
    if (!use_cached_bfs) {
        bfs_fill(player, sx, sy, 1);
    }
    int best_x = -1, best_y = -1, best_d = MAXQ;
    int rem = player->your_status;
    for (int g = 0; g < 2; ++g) {
        int gx = player->ghost_posx[g], gy = player->ghost_posy[g];
        if (dist_map[gx][gy] >= 0 && dist_map[gx][gy] <= rem) {
            if (dist_map[gx][gy] < best_d) {
                best_d = dist_map[gx][gy];
                best_x = gx; best_y = gy;
            }
        }
    }
    if (best_x < 0) return initPoint(sx, sy);
    // ֱ��ʹ��next�����ȡ��һ��
    return initPoint(nextx[best_x][best_y], nexty[best_x][best_y]);
}

// �ƶ������߼���ͳһBFS���棬�Ż���������
struct Point walk(struct Player *player) {
    compute_ghost_dist(player);
    int sx = player->your_posx, sy = player->your_posy;

    // ��ͨ״̬�����Ȱ�ȫ�������ǣ�ǿ�����¼���BFS��
    if (player->your_status == 0) {
        struct Point p = choose_superstar(player, 0);  // 0��ʾ��ʹ�û��棬ǿ�����¼���
        if (p.X != sx || p.Y != sy) return p;
        // ����choose_nearest_charʱ����0����ʹ�û��棩
        return choose_nearest_char(player, 'o', 0);
    }

    // ǿ��״̬��ͳһʹ��һ��BFS����
    if (player->your_status > 0) {
        bfs_fill(player, sx, sy, 1);  // �״μ���BFS����¼next����

        // 1. ����׷ɱ��ͨ״̬�Ķ��֣�ʹ�û���BFS��
        struct Point opponent = choose_nearest_char(player, 'E', 1);
        if (opponent.X != sx || opponent.Y != sy) return opponent;

        // 2. ���׷ɱ�ƻ��ߣ�ʹ�û���BFS��
        struct Point ghost = choose_nearest_ghost(player, 1);
        if (ghost.X != sx || ghost.Y != sy) {
            // ��鵱ǰBFS���Ƿ��и����ĳ����ǣ�ʹ�û���BFS��
            struct Point superstar = choose_superstar(player, 1);
            if (superstar.X != sx && superstar.Y != sy) {
                if (dist_map[superstar.X][superstar.Y] < dist_map[ghost.X][ghost.Y]) {
                    return superstar;
                }
            }
            return ghost;
        }

        // 3. ��󲹳䳬���ǣ�ʹ�û���BFS��
        struct Point superstar = choose_superstar(player, 1);
        if (superstar.X != sx || superstar.Y != sy) return superstar;

        return initPoint(sx, sy);
    }

    return initPoint(sx, sy);
}

// ��ʼ��������δ�޸ģ�
void init(struct Player *player) {
    (void)player;
}

#endif // PLAYER_H