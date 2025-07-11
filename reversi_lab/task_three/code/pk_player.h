#include <string.h>
#include "../include/playerbase.h"
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>

#define DEPTH 3
#define MAX_SIZE 12
#define ENDGAME_SOLVE_N 10

#define TIME_LIMIT_MS 80
static clock_t start_time;
int time_exceeded() {
    return ((clock() - start_time) * 1000 / CLOCKS_PER_SEC) > TIME_LIMIT_MS;
}

// 权重表
static const int weight_map_8[8][8] = {
    {120, -25, 5, 2, 2, 5, -25, 120},
    {-25, -50, 1, 1, 1, 1, -50, -25},
    {5, 1, 1, 1, 1, 1, 1, 5},
    {2, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 2},
    {5, 1, 1, 1, 1, 1, 1, 5},
    {-25, -50, 1, 1, 1, 1, -50, -25},
    {120, -25, 5, 2, 2, 5, -25, 120}
};

static const int weight_map_10[10][10] = {
    {120, -25, 5, 2, 2, 2, 2, 5, -25, 120},
    {-25, -50, 1, 1, 1, 1, 1, 1, -50, -25},
    {5, 1, 1, 1, 1, 1, 1, 1, 1, 5},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {5, 1, 1, 1, 1, 1, 1, 1, 1, 5},
    {-25, -50, 1, 1, 1, 1, 1, 1, -50, -25},
    {120, -25, 5, 2, 2, 2, 2, 5, -25, 120}
};

static const int weight_map_12[12][12] = {
    {120, -25, 5, 2, 2, 2, 2, 2, 2, 5, -25, 120},
    {-25, -50, 1, 1, 1, 1, 1, 1, 1, 1, -50, -25},
    {5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    {5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
    {-25, -50, 1, 1, 1, 1, 1, 1, 1, 1, -50, -25},
    {120, -25, 5, 2, 2, 2, 2, 2, 2, 5, -25, 120}
};

// 新增的三个二维数组
// 1. 位置稳定性评估表 - 重新设计，强化角落和边缘策略
static const int stability_map_8[8][8] = {
    {200, -50, 20, 5, 5, 20, -50, 200},
    {-50, -100, -20, -20, -20, -20, -100, -50},
    {20, -20, 15, 3, 3, 15, -20, 20},
    {5, -20, 3, 3, 3, 3, -20, 5},
    {5, -20, 3, 3, 3, 3, -20, 5},
    {20, -20, 15, 3, 3, 15, -20, 20},
    {-50, -100, -20, -20, -20, -20, -100, -50},
    {200, -50, 20, 5, 5, 20, -50, 200}
};

static const int stability_map_10[10][10] = {
    {200, -50, 20, 5, 5, 5, 5, 20, -50, 200},
    {-50, -100, -20, -20, -20, -20, -20, -20, -100, -50},
    {20, -20, 15, 3, 3, 3, 3, 15, -20, 20},
    {5, -20, 3, 3, 3, 3, 3, 3, -20, 5},
    {5, -20, 3, 3, 3, 3, 3, 3, -20, 5},
    {5, -20, 3, 3, 3, 3, 3, 3, -20, 5},
    {5, -20, 3, 3, 3, 3, 3, 3, -20, 5},
    {20, -20, 15, 3, 3, 3, 3, 15, -20, 20},
    {-50, -100, -20, -20, -20, -20, -20, -20, -100, -50},
    {200, -50, 20, 5, 5, 5, 5, 20, -50, 200}
};

// 12x12专用稳定性权重表（角落400，边线60）
static const int stability_map_12[12][12] = {
    {400, -80, 30, 10, 10, 10, 10, 10, 10, 30, -80, 400},
    {-80, -150, -30, -30, -30, -30, -30, -30, -30, -30, -150, -80},
    {30, -30, 20, 5, 5, 5, 5, 5, 5, 20, -30, 30},
    {10, -30, 5, 5, 5, 5, 5, 5, 5, 5, -30, 10},
    {10, -30, 5, 5, 5, 5, 5, 5, 5, 5, -30, 10},
    {10, -30, 5, 5, 5, 5, 5, 5, 5, 5, -30, 10},
    {10, -30, 5, 5, 5, 5, 5, 5, 5, 5, -30, 10},
    {10, -30, 5, 5, 5, 5, 5, 5, 5, 5, -30, 10},
    {10, -30, 5, 5, 5, 5, 5, 5, 5, 5, -30, 10},
    {30, -30, 20, 5, 5, 5, 5, 5, 5, 20, -30, 30},
    {-80, -150, -30, -30, -30, -30, -30, -30, -30, -30, -150, -80},
    {400, -80, 30, 10, 10, 10, 10, 10, 10, 30, -80, 400}
};

// 2. 移动潜力评估表 - 重新设计，强化角落和边缘策略
static const int mobility_map_8[8][8] = {
    {50, 0, 0, 0, 0, 0, 0, 50},
    {0, -30, -10, -10, -10, -10, -30, 0},
    {0, -10, 5, 5, 5, 5, -10, 0},
    {0, -10, 5, 8, 8, 5, -10, 0},
    {0, -10, 5, 8, 8, 5, -10, 0},
    {0, -10, 5, 5, 5, 5, -10, 0},
    {0, -30, -10, -10, -10, -10, -30, 0},
    {50, 0, 0, 0, 0, 0, 0, 50}
};

static const int mobility_map_10[10][10] = {
    {50, 0, 0, 0, 0, 0, 0, 0, 0, 50},
    {0, -30, -10, -10, -10, -10, -10, -10, -30, 0},
    {0, -10, 5, 5, 5, 5, 5, 5, -10, 0},
    {0, -10, 5, 8, 8, 8, 8, 5, -10, 0},
    {0, -10, 5, 8, 10, 10, 8, 5, -10, 0},
    {0, -10, 5, 8, 10, 10, 8, 5, -10, 0},
    {0, -10, 5, 8, 8, 8, 8, 5, -10, 0},
    {0, -10, 5, 5, 5, 5, 5, 5, -10, 0},
    {0, -30, -10, -10, -10, -10, -10, -10, -30, 0},
    {50, 0, 0, 0, 0, 0, 0, 0, 0, 50}
};

static const int mobility_map_12[12][12] = {
    {50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50},
    {0, -30, -10, -10, -10, -10, -10, -10, -10, -10, -30, 0},
    {0, -10, 5, 5, 5, 5, 5, 5, 5, 5, -10, 0},
    {0, -10, 5, 8, 8, 8, 8, 8, 8, 5, -10, 0},
    {0, -10, 5, 8, 10, 10, 10, 10, 8, 5, -10, 0},
    {0, -10, 5, 8, 10, 12, 12, 10, 8, 5, -10, 0},
    {0, -10, 5, 8, 10, 12, 12, 10, 8, 5, -10, 0},
    {0, -10, 5, 8, 10, 10, 10, 10, 8, 5, -10, 0},
    {0, -10, 5, 8, 8, 8, 8, 8, 8, 5, -10, 0},
    {0, -10, 5, 5, 5, 5, 5, 5, 5, 5, -10, 0},
    {0, -30, -10, -10, -10, -10, -10, -10, -10, -10, -30, 0},
    {50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50}
};

// 3. 角落控制评估表 - 重新设计，强化角落策略
static const int corner_map_8[8][8] = {
    {100, -25, 20, 5, 5, 20, -25, 100},
    {-25, -50, -15, -15, -15, -15, -50, -25},
    {20, -15, 10, 3, 3, 10, -15, 20},
    {5, -15, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, -15, 5},
    {20, -15, 10, 3, 3, 10, -15, 20},
    {-25, -50, -15, -15, -15, -15, -50, -25},
    {100, -25, 20, 5, 5, 20, -25, 100}
};

static const int corner_map_10[10][10] = {
    {100, -25, 20, 5, 5, 5, 5, 20, -25, 100},
    {-25, -50, -15, -15, -15, -15, -15, -15, -50, -25},
    {20, -15, 10, 3, 3, 3, 3, 10, -15, 20},
    {5, -15, 3, 3, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, 3, 3, -15, 5},
    {20, -15, 10, 3, 3, 3, 3, 10, -15, 20},
    {-25, -50, -15, -15, -15, -15, -15, -15, -50, -25},
    {100, -25, 20, 5, 5, 5, 5, 20, -25, 100}
};

static const int corner_map_12[12][12] = {
    {100, -25, 20, 5, 5, 5, 5, 5, 5, 20, -25, 100},
    {-25, -50, -15, -15, -15, -15, -15, -15, -15, -15, -50, -25},
    {20, -15, 10, 3, 3, 3, 3, 3, 3, 10, -15, 20},
    {5, -15, 3, 3, 3, 3, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, 3, 3, 3, 3, -15, 5},
    {5, -15, 3, 3, 3, 3, 3, 3, 3, 3, -15, 5},
    {20, -15, 10, 3, 3, 3, 3, 3, 3, 10, -15, 20},
    {-25, -50, -15, -15, -15, -15, -15, -15, -15, -15, -50, -25},
    {100, -25, 20, 5, 5, 5, 5, 5, 5, 20, -25, 100}
};

// 方向数组
static const int dirs[8][2] = {{0,1}, {0,-1}, {1,1}, {1,0}, {1,-1}, {-1,0}, {-1,1}, {-1,-1}};

// --- 声明提前 ---
// static clock_t start_time; // 移除全局变量声明
// static clock_t start_time; // 移除全局变量声明
int will_give_corner(char **board, int rows, int cols, int i, int j, char my, char opp);
int alphabeta(char **board, int rows, int cols, int depth, int alpha, int beta, int is_max, char my, char opp);

void init(struct Player *player) {
    srand(time(0));
}

// 判断位置是否合法
int is_valid(struct Player *player, int posx, int posy) {
    if (posx < 0 || posx >= player->row_cnt || posy < 0 || posy >= player->col_cnt)
        return 0;
    char c = player->mat[posx][posy];
    // 只要不是己方或对方棋子，都允许落子
    if (c == 'O' || c == 'o')
        return 0;
    for (int d = 0; d < 8; d++) {
        int x = posx + dirs[d][0];
        int y = posy + dirs[d][1];
        int found_opponent = 0;
        while (x >= 0 && x < player->row_cnt && y >= 0 && y < player->col_cnt) {
            if (player->mat[x][y] == 'o') {
                found_opponent = 1;
            } else if (player->mat[x][y] == 'O') {
                if (found_opponent) return 1;
                else break;
            } else {
                break;
            }
            x += dirs[d][0];
            y += dirs[d][1];
        }
    }
    return 0;
}

// 创建棋盘副本：自己模拟棋盘而不改变原棋盘
char** copy_board(struct Player *player) {
    int rows = player->row_cnt;
    int cols = player->col_cnt;
    char** new_board = (char**)malloc(rows * sizeof(char*));
    for (int i = 0; i < rows; i++) {
        new_board[i] = (char*)malloc(cols * sizeof(char));
        memcpy(new_board[i], player->mat[i], cols);
    }
    return new_board;
}

// 释放棋盘副本
void free_board(char** board, int rows) {
    for (int i = 0; i < rows; i++) {
        free(board[i]);
    }
    free(board);
}

// 在自定义棋盘上进行有效性检查
int is_valid_custom(char** board, int rows, int cols, int x, int y, char player_piece, char opponent_piece) {
    if (x < 0 || x >= rows || y < 0 || y >= cols) return false;
    if (board[x][y] == player_piece || board[x][y] == opponent_piece) return false;
    
    for (int d = 0; d < 8; d++) {
        int dx = x + dirs[d][0];
        int dy = y + dirs[d][1];
        if (dx < 0 || dx >= rows || dy < 0 || dy >= cols) continue;
        if (board[dx][dy] != opponent_piece) continue;
        
        while (true) {
            dx += dirs[d][0];
            dy += dirs[d][1];
            if (dx < 0 || dx >= rows || dy < 0 || dy >= cols || 
                (board[dx][dy] >= '1' && board[dx][dy] <= '9')) break;
            if (board[dx][dy] == player_piece) return true;
        }
    }
    return false;
}

// 恢复原有 do_move
void do_move(char **board, int rows, int cols, int x, int y) {
    board[x][y] = 'O';
    for (int d = 0; d < 8; d++) {
        int nx = x + dirs[d][0];
        int ny = y + dirs[d][1];
        int cnt = 0;
        while (nx >= 0 && nx < rows && ny >= 0 && ny < cols && board[nx][ny] == 'o') {
            nx += dirs[d][0];
            ny += dirs[d][1];
            cnt++;
        }
        if (cnt > 0 && nx >= 0 && nx < rows && ny >= 0 && ny < cols && board[nx][ny] == 'O') {
            int tx = x + dirs[d][0], ty = y + dirs[d][1];
            for (int k = 0; k < cnt; k++) {
                board[tx][ty] = 'O';
                tx += dirs[d][0];
                ty += dirs[d][1];
            }
        }
    }
}

// 1. 全局缓冲区定义
#define MAX_DEPTH 8
#define MAX_SIZE 12
static char board_buf[MAX_DEPTH][MAX_SIZE][MAX_SIZE];

// 2. 修改do_move支持二维数组
void do_move2(char board[MAX_SIZE][MAX_SIZE], int rows, int cols, int x, int y) {
    board[x][y] = 'O';
    for (int d = 0; d < 8; d++) {
        int nx = x + dirs[d][0];
        int ny = y + dirs[d][1];
        int cnt = 0;
        while (nx >= 0 && nx < rows && ny >= 0 && ny < cols && board[nx][ny] == 'o') {
            nx += dirs[d][0];
            ny += dirs[d][1];
            cnt++;
        }
        if (cnt > 0 && nx >= 0 && nx < rows && ny >= 0 && ny < cols && board[nx][ny] == 'O') {
            int tx = x + dirs[d][0], ty = y + dirs[d][1];
            for (int k = 0; k < cnt; k++) {
                board[tx][ty] = 'O';
                tx += dirs[d][0];
                ty += dirs[d][1];
            }
        }
    }
}

// 统计稳定子（仅角及连通）
int count_stable(char **board, int rows, int cols, char me) {
    int stable = 0;
    int dx[4] = {0, 0, rows-1, rows-1};
    int dy[4] = {0, cols-1, 0, cols-1};
    for (int d = 0; d < 4; d++) {
        int x = dx[d], y = dy[d];
        if (board[x][y] != me) continue;
        int ix = x, iy = y;
        // 横向
        while (ix >= 0 && ix < rows && board[ix][y] == me) ix += (x==0?1:-1);
        // 纵向
        while (iy >= 0 && iy < cols && board[x][iy] == me) iy += (y==0?1:-1);
        int r1 = (x==0?0:ix+1), r2 = (x==0?ix-1:rows-1);
        int c1 = (y==0?0:iy+1), c2 = (y==0?iy-1:cols-1);
        for (int a = r1; a <= r2; a++)
            for (int b = c1; b <= c2; b++)
                if (board[a][b] == me) stable++;
    }
    return stable;
}

// 统计有效移动数量
int count_moves(char **board, int rows, int cols, char player, char opponent) {
    int moves = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (is_valid_custom(board, rows, cols, i, j, player, opponent)) {
                moves++;
            }
        }
    }
    return moves;
}

// 计算角落控制分数
int calculate_corner_score(char **board, int rows, int cols, char my, char opp) {
    int corners[4][2] = {{0,0}, {0,cols-1}, {rows-1,0}, {rows-1,cols-1}};
    int my_corners = 0, opp_corners = 0;
    
    for (int i = 0; i < 4; i++) {
        int x = corners[i][0], y = corners[i][1];
        if (board[x][y] == my) my_corners++;
        else if (board[x][y] == opp) opp_corners++;
    }
    
    return (my_corners - opp_corners) * 100; // 增加角落权重
}

// 检查是否有角落可占
int has_corner_move(char **board, int rows, int cols, char player, char opponent) {
    int corners[4][2] = {{0,0}, {0,cols-1}, {rows-1,0}, {rows-1,cols-1}};
    
    for (int i = 0; i < 4; i++) {
        int x = corners[i][0], y = corners[i][1];
        if (board[x][y] == ' ' && is_valid_custom(board, rows, cols, x, y, player, opponent)) {
            return 1;
        }
    }
    return 0;
}

// 检查是否有边缘可占
int has_edge_move(char **board, int rows, int cols, char player, char opponent) {
    // 检查所有边缘位置
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if ((i == 0 || i == rows-1 || j == 0 || j == cols-1) && 
                board[i][j] == ' ' && 
                is_valid_custom(board, rows, cols, i, j, player, opponent)) {
                return 1;
            }
        }
    }
    return 0;
}

// 判断(i, j)这个边缘空位是否能安全吃掉对方边子（且不会送角）
int can_eat_edge_safe(char **board, int rows, int cols, int i, int j, char my, char opp) {
    if (!(i == 0 || i == rows-1 || j == 0 || j == cols-1)) return 0;
    if (board[i][j] != ' ') return 0;
    for (int d = 0; d < 8; d++) {
        int x = i + dirs[d][0];
        int y = j + dirs[d][1];
        if (x >= 0 && x < rows && y >= 0 && y < cols && board[x][y] == opp) {
            int xx = x + dirs[d][0];
            int yy = y + dirs[d][1];
            if (xx >= 0 && xx < rows && yy >= 0 && yy < cols && board[xx][yy] == my) {
                // 模拟落子，判断是否送角
                char **new_board = (char**)malloc(rows * sizeof(char*));
                for (int r = 0; r < rows; r++) {
                    new_board[r] = (char*)malloc(cols * sizeof(char));
                    memcpy(new_board[r], board[r], cols);
                }
                do_move(new_board, rows, cols, i, j);
                int give_corner = has_corner_move(new_board, rows, cols, opp, my);
                for (int r = 0; r < rows; r++) free(new_board[r]);
                free(new_board);
                if (!give_corner) return 1;
            }
        }
    }
    return 0;
}

// 获取权重表指针
const int* get_weight_map(int rows, int cols) {
    if (rows == 8 && cols == 8) return (const int*)weight_map_8;
    if (rows == 10 && cols == 10) return (const int*)weight_map_10;
    if (rows == 12 && cols == 12) return (const int*)weight_map_12;
    return NULL;
}

// 获取稳定性表指针
const int* get_stability_map(int rows, int cols) {
    if (rows == 8 && cols == 8) return (const int*)stability_map_8;
    if (rows == 10 && cols == 10) return (const int*)stability_map_10;
    if (rows == 12 && cols == 12) return (const int*)stability_map_12;
    return NULL;
}

// 获取移动潜力表指针
const int* get_mobility_map(int rows, int cols) {
    if (rows == 8 && cols == 8) return (const int*)mobility_map_8;
    if (rows == 10 && cols == 10) return (const int*)mobility_map_10;
    if (rows == 12 && cols == 12) return (const int*)mobility_map_12;
    return NULL;
}

// 获取角落控制表指针
const int* get_corner_map(int rows, int cols) {
    if (rows == 8 && cols == 8) return (const int*)corner_map_8;
    if (rows == 10 && cols == 10) return (const int*)corner_map_10;
    if (rows == 12 && cols == 12) return (const int*)corner_map_12;
    return NULL;
}

// 动态DEPTH函数，12x12后期DEPTH=4，其它情况DEPTH=3
int get_dynamic_depth(int rows, int cols, int empty) {
    int total = rows * cols;
    if (total >= 100) { // 10x10及以上
        if (empty > 20) return 1;
        else if (empty > 8) return 2;
        else if (empty > 4) return 3;
        else return 4;
    }
    if (rows == 8 && cols == 8) {
        if (empty > 16) return 1;
        else if (empty > 6) return 2;
        else if (empty > 3) return 3;
        else return 4;
    }
    return 1;
}

// 动态位置权重（进攻与防守兼备）- 强化角落策略和动态边线权重
int dynamic_weight(int x, int y, int rows, int cols, double empty_ratio) {
    // 角落 - 最高优先级
    if ((x == 0 && y == 0) || (x == 0 && y == cols - 1) ||
        (x == rows - 1 && y == 0) || (x == rows - 1 && y == cols - 1))
        return (rows == 12 && cols == 12) ? 400 : 200;
    // X点（角落斜对角一格）- 避免
    if ((x == 1 && y == 1) || (x == 1 && y == cols-2) ||
        (x == rows-2 && y == 1) || (x == rows-2 && y == cols-2))
        return -80;
    // C点（角落正邻一格）- 避免
    if ((x == 0 && y == 1) || (x == 1 && y == 0) ||
        (x == 0 && y == cols-2) || (x == 1 && y == cols-1) ||
        (x == rows-1 && y == 1) || (x == rows-2 && y == 0) ||
        (x == rows-1 && y == cols-2) || (x == rows-2 && y == cols-1))
        return -60;
    // 边缘 - 动态权重
    if (x == 0 || x == rows-1 || y == 0 || y == cols-1) {
        int edge_weight;
        if (rows == 12 && cols == 12) {
            edge_weight = 60; // 12x12边线权重提升
        } else {
            if (empty_ratio > 2.0/3.0) edge_weight = 20;      // 前期适中
            else if (empty_ratio > 1.0/3.0) edge_weight = 10; // 中期略降
            else edge_weight = 30;                            // 后期提升
        }
        // 避免角落附近的边缘位置
        if ((x == 0 && (y == 1 || y == cols-2)) ||
            (x == rows-1 && (y == 1 || y == cols-2)) ||
            (y == 0 && (x == 1 || x == rows-2)) ||
            (y == cols-1 && (x == 1 || x == rows-2)))
            return -30;
        return edge_weight;
    }
    // 其它
    return 1;
}

// 检查(i, j)是否为危险夹心空点（除非两端有我方棋子）
int is_sandwich_trap(char **board, int rows, int cols, int i, int j, char my, char opp) {
    if (board[i][j] != ' ') return 0;
    // 四个方向：水平、垂直、两条对角线
    int dirs[4][2][2] = {
        {{0, -1}, {0, 1}},    // 左右
        {{-1, 0}, {1, 0}},    // 上下
        {{-1, -1}, {1, 1}},   // 左上右下
        {{-1, 1}, {1, -1}}    // 右上左下
    };
    for (int d = 0; d < 4; d++) {
        int x1 = i + dirs[d][0][0], y1 = j + dirs[d][0][1];
        int x2 = i + dirs[d][1][0], y2 = j + dirs[d][1][1];
        int safe1 = 0, safe2 = 0;
        if (x1 >= 0 && x1 < rows && y1 >= 0 && y1 < cols) {
            if (board[x1][y1] == my) safe1 = 1;
        }
        if (x2 >= 0 && x2 < rows && y2 >= 0 && y2 < cols) {
            if (board[x2][y2] == my) safe2 = 1;
        }
        // 原有“对方-空-对方”型
        if (!safe1 || !safe2) {
            if (x1 >= 0 && x1 < rows && y1 >= 0 && y1 < cols &&
                x2 >= 0 && x2 < rows && y2 >= 0 && y2 < cols &&
                board[x1][y1] == opp && board[x2][y2] == opp) {
                return 1; // 是危险夹心空
            }
        }
        // 新增“我方-我方-空格-对方”型
        int xx1 = i - dirs[d][0][0], yy1 = j - dirs[d][0][1];
        int xx2 = i + 2*dirs[d][1][0], yy2 = j + 2*dirs[d][1][1];
        if (x1 >= 0 && x1 < rows && y1 >= 0 && y1 < cols &&
            xx1 >= 0 && xx1 < rows && yy1 >= 0 && yy1 < cols &&
            x2 >= 0 && x2 < rows && y2 >= 0 && y2 < cols &&
            xx2 >= 0 && xx2 < rows && yy2 >= 0 && yy2 < cols) {
            // 检查“我方-我方-空格-对方”
            if (board[xx1][yy1] == my && board[x1][y1] == my && board[i][j] == ' ' && board[x2][y2] == opp) {
                return 1;
            }
            // 检查“对方-空格-我方-我方”
            if (board[x1][y1] == opp && board[i][j] == ' ' && board[x2][y2] == my && board[xx2][yy2] == my) {
                return 1;
            }
        }
    }
    // 检查上边连续空点陷阱
    if (i == 0) { // 上边
        int left = j, right = j;
        // 向左扩展
        while (left > 0 && board[0][left-1] == ' ') left--;
        // 向右扩展
        while (right < cols-1 && board[0][right+1] == ' ') right++;
        // 检查区间[left, right]
        if (right - left + 1 >= 2) {
            int left_end = (left > 0) ? board[0][left-1] : 0;
            int right_end = (right < cols-1) ? board[0][right+1] : 0;
            // 仅一端是我方棋子，另一端不是
            if ((left_end == my && right_end != my) || (right_end == my && left_end != my)) {
                return 1;
            }
        }
    }
    // 检查下边连续空点陷阱
    if (i == rows-1) { // 下边
        int left = j, right = j;
        // 向左扩展
        while (left > 0 && board[rows-1][left-1] == ' ') left--;
        // 向右扩展
        while (right < cols-1 && board[rows-1][right+1] == ' ') right++;
        // 检查区间[left, right]
        if (right - left + 1 >= 2) {
            int left_end = (left > 0) ? board[rows-1][left-1] : 0;
            int right_end = (right < cols-1) ? board[rows-1][right+1] : 0;
            // 仅一端是我方棋子，另一端不是
            if ((left_end == my && right_end != my) || (right_end == my && left_end != my)) {
                return 1;
            }
        }
    }
    // 检查左边连续空点陷阱
    if (j == 0) { // 左边
        int up = i, down = i;
        // 向上扩展
        while (up > 0 && board[up-1][0] == ' ') up--;
        // 向下扩展
        while (down < rows-1 && board[down+1][0] == ' ') down++;
        // 检查区间[up, down]
        if (down - up + 1 >= 2) {
            int up_end = (up > 0) ? board[up-1][0] : 0;
            int down_end = (down < rows-1) ? board[down+1][0] : 0;
            // 仅一端是我方棋子，另一端不是
            if ((up_end == my && down_end != my) || (down_end == my && up_end != my)) {
                return 1;
            }
        }
    }
    // 检查右边连续空点陷阱
    if (j == cols-1) { // 右边
        int up = i, down = i;
        // 向上扩展
        while (up > 0 && board[up-1][cols-1] == ' ') up--;
        // 向下扩展
        while (down < rows-1 && board[down+1][cols-1] == ' ') down++;
        // 检查区间[up, down]
        if (down - up + 1 >= 2) {
            int up_end = (up > 0) ? board[up-1][cols-1] : 0;
            int down_end = (down < rows-1) ? board[down+1][cols-1] : 0;
            // 仅一端是我方棋子，另一端不是
            if ((up_end == my && down_end != my) || (down_end == my && up_end != my)) {
                return 1;
            }
        }
    }
    return 0;
}

// 评估函数中，若能给对方设置夹心陷阱则加分
int evaluate(char **board, int rows, int cols, char my, char opp) {
    int score = 0, my_moves = 0, opp_moves = 0;
    int empty = 0, my_cnt = 0, opp_cnt = 0;
    int total_cells = rows * cols;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (board[i][j] == ' ') empty++;
        }
    }
    double empty_ratio = (double)empty / total_cells;
    // 获取各种评估表
    const int* weight_map = get_weight_map(rows, cols);
    const int* stability_map = get_stability_map(rows, cols);
    const int* mobility_map = get_mobility_map(rows, cols);
    const int* corner_map = get_corner_map(rows, cols);
    // 计算移动数量（避免重复计算）
    my_moves = count_moves(board, rows, cols, my, opp);
    opp_moves = count_moves(board, rows, cols, opp, my);
    // 检查是否有角落可占 - 给予极高优先级
    if (has_corner_move(board, rows, cols, my, opp)) {
        score += 1000; // 角落优先
    }
    if (has_corner_move(board, rows, cols, opp, my)) {
        score -= 1000; // 阻止对手占角落
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // 基础权重评估
            int w = dynamic_weight(i, j, rows, cols, empty_ratio);
            if (board[i][j] == my) {
                score += w;
                my_cnt++;
                // 使用新的评估表
                if (weight_map) score += weight_map[i * cols + j];
                if (stability_map) score += stability_map[i * cols + j];
                if (corner_map) score += corner_map[i * cols + j];
            } else if (board[i][j] == opp) {
                score -= w;
                opp_cnt++;
                // 对手的负分
                if (weight_map) score -= weight_map[i * cols + j];
                if (stability_map) score -= stability_map[i * cols + j];
                if (corner_map) score -= corner_map[i * cols + j];
            } else {
                // 空位的移动潜力评估
                if (mobility_map) {
                    int mobility_score = mobility_map[i * cols + j];
                    if (is_valid_custom(board, rows, cols, i, j, my, opp)) {
                        score += mobility_score;
                    }
                    if (is_valid_custom(board, rows, cols, i, j, opp, my)) {
                        score -= mobility_score;
                    }
                }
                // 如果我方下在(i,j)能让对方下在(i,j)时被夹心，给我方加分
                if (is_sandwich_trap(board, rows, cols, i, j, opp, my)) {
                    score += 15; // 你可以根据实际调整分值
                }
            }
        }
    }
    // 计算稳定子
    int my_stable = count_stable(board, rows, cols, my);
    int opp_stable = count_stable(board, rows, cols, opp);
    // 计算角落控制分数
    int corner_score = calculate_corner_score(board, rows, cols, my, opp);
    // 终局优先
    if (empty < 10) score += 100 * (my_cnt - opp_cnt);
    // 动态调整稳定子权重
    int stable_weight;
    if (empty_ratio > 2.0/3.0) {         // 前期
        stable_weight = 2;
    } else if (empty_ratio > 1.0/3.0) {  // 中期
        stable_weight = 10;
    } else {                             // 后期
        stable_weight = 40;
    }
    score += stable_weight * (my_stable - opp_stable);
    // 综合加权
    score += 5 * (my_moves - opp_moves);
    score += corner_score;
    return score;
}

// 3. alphabeta递归，类型统一，防止越界，memcpy范围严谨
struct Move { int x, y, score; };

int cmp_move(const void *a, const void *b) {
    return ((struct Move*)b)->score - ((struct Move*)a)->score;
}

int alphabeta(char board[MAX_SIZE][MAX_SIZE], int rows, int cols, int depth, int alpha, int beta, int is_max, char my, char opp, int buf_id) {
    if (depth == 0) return evaluate((char**)board, rows, cols, my, opp);
    if (buf_id+1 >= MAX_DEPTH) return evaluate((char**)board, rows, cols, my, opp); // 防止越界
    int found = 0;
    struct Move moves[MAX_SIZE*MAX_SIZE];
    int move_cnt = 0;
    if (is_max) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (!is_valid_custom((char**)board, rows, cols, i, j, my, opp)) continue;
                moves[move_cnt].x = i;
                moves[move_cnt].y = j;
                moves[move_cnt].score = dynamic_weight(i, j, rows, cols, 0.5);
                move_cnt++;
            }
        }
        qsort(moves, move_cnt, sizeof(struct Move), cmp_move);
        int max_branch = 6;
        for (int k = 0; k < move_cnt && k < max_branch; k++) {
            int i = moves[k].x, j = moves[k].y;
                found = 1;
            memcpy(board_buf[buf_id+1], board, sizeof(char)*rows*cols);
            do_move2(board_buf[buf_id+1], rows, cols, i, j);
            int empty = 0;
            for (int x = 0; x < rows; x++) {
                for (int y = 0; y < cols; y++) {
                    if (board_buf[buf_id+1][x][y] == ' ') empty++;
                }
            }
            int next_depth = get_dynamic_depth(rows, cols, empty) - 1;
            int val = alphabeta(board_buf[buf_id+1], rows, cols, next_depth, alpha, beta, 0, my, opp, buf_id+1);
            if (val > alpha) alpha = val;
            if (beta <= alpha) return alpha;
        }
        if (!found) return evaluate((char**)board, rows, cols, my, opp);
        return alpha;
    } else {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (!is_valid_custom((char**)board, rows, cols, i, j, opp, my)) continue;
                moves[move_cnt].x = i;
                moves[move_cnt].y = j;
                moves[move_cnt].score = dynamic_weight(i, j, rows, cols, 0.5);
                move_cnt++;
            }
        }
        qsort(moves, move_cnt, sizeof(struct Move), cmp_move);
        int max_branch = 6;
        for (int k = 0; k < move_cnt && k < max_branch; k++) {
            int i = moves[k].x, j = moves[k].y;
                found = 1;
            memcpy(board_buf[buf_id+1], board, sizeof(char)*rows*cols);
                // 对手落子模拟
            board_buf[buf_id+1][i][j] = opp;
                for (int d = 0; d < 8; d++) {
                    int nx = i + dirs[d][0];
                    int ny = j + dirs[d][1];
                    int cnt = 0;
                while (nx >= 0 && nx < rows && ny >= 0 && ny < cols && board_buf[buf_id+1][nx][ny] == my) {
                        nx += dirs[d][0];
                        ny += dirs[d][1];
                        cnt++;
                    }
                if (cnt > 0 && nx >= 0 && nx < rows && ny >= 0 && ny < cols && board_buf[buf_id+1][nx][ny] == opp) {
                        int tx = i + dirs[d][0], ty = j + dirs[d][1];
                    for (int m = 0; m < cnt; m++) {
                        board_buf[buf_id+1][tx][ty] = opp;
                            tx += dirs[d][0];
                            ty += dirs[d][1];
                        }
                    }
                }
            int empty = 0;
            for (int x = 0; x < rows; x++) {
                for (int y = 0; y < cols; y++) {
                    if (board_buf[buf_id+1][x][y] == ' ') empty++;
                }
            }
            int next_depth = get_dynamic_depth(rows, cols, empty) - 1;
            int val = alphabeta(board_buf[buf_id+1], rows, cols, next_depth, alpha, beta, 1, my, opp, buf_id+1);
            if (val < beta) beta = val;
            if (beta <= alpha) return beta;
        }
        if (!found) return evaluate((char**)board, rows, cols, my, opp);
        return beta;
    }
}

// 4. will_give_corner适配
int will_give_corner(char **board, int rows, int cols, int i, int j, char my, char opp) {
    // 用静态缓冲区，memcpy范围严谨
    memcpy(board_buf[0], board, sizeof(char)*rows*cols);
    do_move2(board_buf[0], rows, cols, i, j);
    int result = has_corner_move((char**)board_buf[0], rows, cols, opp, my);
    return result;
}

// 5. place函数：主循环计时，递归分支不检测时间
struct Point place(struct Player *player) {
    start_time = clock();
    int rows = player->row_cnt, cols = player->col_cnt;
    int best_score = INT_MIN;
    struct Point best = initPoint(-1, -1);
    int empty = 0;
    
    // 计算空位数量
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            if (player->mat[i][j] == ' ')
                empty++;
    
    int max_search_depth = get_dynamic_depth(rows, cols, empty);
    
    // 提前退出：如果没有有效移动
    int valid_move_exists = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (is_valid(player, i, j)) {
                valid_move_exists = 1;
                best = initPoint(i, j);  // 设置默认移动
                break;
            }
        }
        if (valid_move_exists) break;
    }
    if (!valid_move_exists) return best;  // 无有效移动时直接返回
    
    // 主循环计时：每步都检测时间
    for (int cur_depth = 1; cur_depth <= max_search_depth; cur_depth++) {
        int local_best_score = INT_MIN;
        struct Point local_best = initPoint(-1, -1);
        int updated = 0;
        
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                // 主循环时间检查
                if (time_exceeded()) {
                    if (local_best.X != -1) return local_best;
                    if (best.X != -1) return best;
                    return initPoint(i, j);
                }
                
                if (!is_valid(player, i, j)) continue;
                
                memcpy(board_buf[0], player->mat, sizeof(char)*rows*cols);
                do_move2(board_buf[0], rows, cols, i, j);
                
                // 递归分支不检测时间
                int score = alphabeta(board_buf[0], rows, cols, cur_depth - 1, 
                                     INT_MIN, INT_MAX, 0, 'O', 'o', 0);
                
                if (score > local_best_score) {
                    local_best_score = score;
                    local_best = initPoint(i, j);
                    updated = 1;
                }
            }
        }
        
        // 更新全局最佳结果
        if (updated && local_best_score > best_score) {
            best_score = local_best_score;
            best = local_best;
        }
        
        // 深度完成后检查时间
        if (time_exceeded()) break;
    }
    
    // 确保始终返回有效移动
    if (best.X == -1) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (is_valid(player, i, j)) {
                    return initPoint(i, j);
                }
            }
        }
    }
    
    return best;
}