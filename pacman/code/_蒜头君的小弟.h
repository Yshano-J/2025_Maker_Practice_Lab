#ifndef PLAYER_H
#define PLAYER_H

#include <string.h>
#include <stdlib.h>
#include "../include/playerbase.h"

#define MAXN 200
#define MAXM 200
#define MAXQ (MAXN*MAXM)
#define SAFE_DIST_GHOST 1   // 普通状态与破坏者的安全距离
#define SAFE_DIST_OPP 1     // 普通状态与强化对手的安全距离

// 四方向：上、下、左、右
static const int dirx[4] = {-1, 1, 0, 0};
static const int diry[4] = {0, 0, -1, 1};

// 全局 BFS 辅助数组（新增next方向记录）
static int qx[MAXQ], qy[MAXQ]; // 队列
static int dist_map[MAXN][MAXM], prex[MAXN][MAXM], prey[MAXN][MAXM]; 
// dist_map数组记录距离；prex和prey数组记录前驱节点
static int nextx[MAXN][MAXM], nexty[MAXN][MAXM];  // 记录起点到（i，j）的下一步应该走的位置
static int ghost_dist[MAXN][MAXM]; // 距离破坏者

// 计算任意格子到最近破坏者的最短距离（以破坏者为中心，增强破坏者位置有效性判断）
static void compute_ghost_dist(struct Player *player) {
    int n = player->row_cnt, m = player->col_cnt;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            ghost_dist[i][j] = -1;

    int qh = 0, qt = 0;
    for (int g = 0; g < 2; ++g) {
        int gx = player->ghost_posx[g], gy = player->ghost_posy[g];
        // 新增：检查破坏者位置是否是墙（根据规则初始不在墙，但移动后可能）
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


// 在吃豆人游戏中，我们需要找到从当前位置到目标位置（如超级星、对手、破坏者等）的最短路径，但通常只需要知道第一步应该往哪个方向走，而不需要完整路径。
// 解决方案：
// 使用nextx和nexty数组记录每个位置到起点的下一步位置：
// nextx[i][j]：从起点到位置(i,j)路径的第一步x坐标
// nexty[i][j]：从起点到位置(i,j)路径的第一步y坐标
// 获取next数组，记录起点到（i，j）的下一步应该走的位置
// 通用 BFS 填充（新增next方向记录，优化路径回溯）
// 普通状态安全判断（使用参数化安全距离）
// 移动之后，我与破坏者距离小于SAFE_DIST_GHOST，或者我与对手距离小于SAFE_DIST_OPP，就不能移动
static void bfs_fill(struct Player *player, int sx, int sy, int reset_pre) {
    int n = player->row_cnt, m = player->col_cnt;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            dist_map[i][j] = -1;
            if (reset_pre) {
                prex[i][j] = prey[i][j] = -1;
                nextx[i][j] = nexty[i][j] = -1;  // 初始化下一步方向
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
                    // 修正：起点的邻居直接记录下一步，其他节点继承父节点的下一步
                    if (x == sx && y == sy) {  // 父节点是起点，当前节点是起点的邻居
                        nextx[nx][ny] = nx;     // 下一步就是当前节点（nx, ny）
                        nexty[nx][ny] = ny;
                    } else {                     // 父节点不是起点，继承父节点的下一步
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

// 选择最佳超级星（使用next方向直接获取下一步）
// 选择超级星考虑：
// 1. 距离超级星越近越好，d越小越好
// 2. 距离破坏者越远越好，safety越大越好
static struct Point choose_superstar(struct Player *player, int use_cached_bfs) {
    int sx = player->your_posx, sy = player->your_posy;
    if (!use_cached_bfs) {
        bfs_fill(player, sx, sy, 1); // 非缓存模式重新计算BFS
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
        return initPoint(sx, sy);  // 目标是起点或无目标时返回原位
    }
    // 直接使用next方向获取下一步（替代原回溯循环）
    return initPoint(nextx[best_x][best_y], nexty[best_x][best_y]);
}

// 选择最近普通星或敌方蒜头（支持缓存BFS，使用next方向）
static struct Point choose_nearest_char(struct Player *player, char target, int use_cached_bfs) {
    int sx = player->your_posx, sy = player->your_posy;
    if (!use_cached_bfs) {
        bfs_fill(player, sx, sy, 1);  // 非缓存模式重新计算BFS
    }
    int best_x = -1, best_y = -1, best_d = MAXQ;
    for (int i = 0; i < player->row_cnt; ++i) {
        for (int j = 0; j < player->col_cnt; ++j) {
            if (dist_map[i][j] >= 0) {
                int ok = 0;
                if (target == 'E') {
                    // 新增：检查对手存活且为普通状态
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
    // 直接使用next方向获取下一步
    return initPoint(nextx[best_x][best_y], nexty[best_x][best_y]);
}

// 选择最近目标破坏者（支持缓存BFS，使用next方向）
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
    // 直接使用next方向获取下一步
    return initPoint(nextx[best_x][best_y], nexty[best_x][best_y]);
}

// 移动核心逻辑（统一BFS缓存，优化调用链）
struct Point walk(struct Player *player) {
    compute_ghost_dist(player);
    int sx = player->your_posx, sy = player->your_posy;

    // 普通状态：优先安全抢超级星（强制重新计算BFS）
    if (player->your_status == 0) {
        struct Point p = choose_superstar(player, 0);  // 0表示不使用缓存，强制重新计算
        if (p.X != sx || p.Y != sy) return p;
        // 调用choose_nearest_char时传递0（不使用缓存）
        return choose_nearest_char(player, 'o', 0);
    }

    // 强化状态：统一使用一次BFS缓存
    if (player->your_status > 0) {
        bfs_fill(player, sx, sy, 1);  // 首次计算BFS并记录next方向

        // 1. 优先追杀普通状态的对手（使用缓存BFS）
        struct Point opponent = choose_nearest_char(player, 'E', 1);
        if (opponent.X != sx || opponent.Y != sy) return opponent;

        // 2. 其次追杀破坏者（使用缓存BFS）
        struct Point ghost = choose_nearest_ghost(player, 1);
        if (ghost.X != sx || ghost.Y != sy) {
            // 检查当前BFS中是否有更近的超级星（使用缓存BFS）
            struct Point superstar = choose_superstar(player, 1);
            if (superstar.X != sx && superstar.Y != sy) {
                if (dist_map[superstar.X][superstar.Y] < dist_map[ghost.X][ghost.Y]) {
                    return superstar;
                }
            }
            return ghost;
        }

        // 3. 最后补充超级星（使用缓存BFS）
        struct Point superstar = choose_superstar(player, 1);
        if (superstar.X != sx || superstar.Y != sy) return superstar;

        return initPoint(sx, sy);
    }

    return initPoint(sx, sy);
}

// 初始化函数（未修改）
void init(struct Player *player) {
    (void)player;
}

#endif // PLAYER_H