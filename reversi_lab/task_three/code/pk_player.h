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

// Ȩ�ر�
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

// ������������ά����
// 1. λ���ȶ��������� - ������ƣ�ǿ������ͱ�Ե����
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

// 12x12ר���ȶ���Ȩ�ر�����400������60��
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

// 2. �ƶ�Ǳ�������� - ������ƣ�ǿ������ͱ�Ե����
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

// 3. ������������� - ������ƣ�ǿ���������
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

// ��������
static const int dirs[8][2] = {{0,1}, {0,-1}, {1,1}, {1,0}, {1,-1}, {-1,0}, {-1,1}, {-1,-1}};

// --- ������ǰ ---
// static clock_t start_time; // �Ƴ�ȫ�ֱ�������
// static clock_t start_time; // �Ƴ�ȫ�ֱ�������
int will_give_corner(char **board, int rows, int cols, int i, int j, char my, char opp);
int alphabeta(char **board, int rows, int cols, int depth, int alpha, int beta, int is_max, char my, char opp);

void init(struct Player *player) {
    srand(time(0));
}

// �ж�λ���Ƿ�Ϸ�
int is_valid(struct Player *player, int posx, int posy) {
    if (posx < 0 || posx >= player->row_cnt || posy < 0 || posy >= player->col_cnt)
        return 0;
    char c = player->mat[posx][posy];
    // ֻҪ���Ǽ�����Է����ӣ�����������
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

// �������̸������Լ�ģ�����̶����ı�ԭ����
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

// �ͷ����̸���
void free_board(char** board, int rows) {
    for (int i = 0; i < rows; i++) {
        free(board[i]);
    }
    free(board);
}

// ���Զ��������Ͻ�����Ч�Լ��
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

// �ָ�ԭ�� do_move
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

// 1. ȫ�ֻ���������
#define MAX_DEPTH 8
#define MAX_SIZE 12
static char board_buf[MAX_DEPTH][MAX_SIZE][MAX_SIZE];

// 2. �޸�do_move֧�ֶ�ά����
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

// ͳ���ȶ��ӣ����Ǽ���ͨ��
int count_stable(char **board, int rows, int cols, char me) {
    int stable = 0;
    int dx[4] = {0, 0, rows-1, rows-1};
    int dy[4] = {0, cols-1, 0, cols-1};
    for (int d = 0; d < 4; d++) {
        int x = dx[d], y = dy[d];
        if (board[x][y] != me) continue;
        int ix = x, iy = y;
        // ����
        while (ix >= 0 && ix < rows && board[ix][y] == me) ix += (x==0?1:-1);
        // ����
        while (iy >= 0 && iy < cols && board[x][iy] == me) iy += (y==0?1:-1);
        int r1 = (x==0?0:ix+1), r2 = (x==0?ix-1:rows-1);
        int c1 = (y==0?0:iy+1), c2 = (y==0?iy-1:cols-1);
        for (int a = r1; a <= r2; a++)
            for (int b = c1; b <= c2; b++)
                if (board[a][b] == me) stable++;
    }
    return stable;
}

// ͳ����Ч�ƶ�����
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

// ���������Ʒ���
int calculate_corner_score(char **board, int rows, int cols, char my, char opp) {
    int corners[4][2] = {{0,0}, {0,cols-1}, {rows-1,0}, {rows-1,cols-1}};
    int my_corners = 0, opp_corners = 0;
    
    for (int i = 0; i < 4; i++) {
        int x = corners[i][0], y = corners[i][1];
        if (board[x][y] == my) my_corners++;
        else if (board[x][y] == opp) opp_corners++;
    }
    
    return (my_corners - opp_corners) * 100; // ���ӽ���Ȩ��
}

// ����Ƿ��н����ռ
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

// ����Ƿ��б�Ե��ռ
int has_edge_move(char **board, int rows, int cols, char player, char opponent) {
    // ������б�Եλ��
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

// �ж�(i, j)�����Ե��λ�Ƿ��ܰ�ȫ�Ե��Է����ӣ��Ҳ����ͽǣ�
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
                // ģ�����ӣ��ж��Ƿ��ͽ�
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

// ��ȡȨ�ر�ָ��
const int* get_weight_map(int rows, int cols) {
    if (rows == 8 && cols == 8) return (const int*)weight_map_8;
    if (rows == 10 && cols == 10) return (const int*)weight_map_10;
    if (rows == 12 && cols == 12) return (const int*)weight_map_12;
    return NULL;
}

// ��ȡ�ȶ��Ա�ָ��
const int* get_stability_map(int rows, int cols) {
    if (rows == 8 && cols == 8) return (const int*)stability_map_8;
    if (rows == 10 && cols == 10) return (const int*)stability_map_10;
    if (rows == 12 && cols == 12) return (const int*)stability_map_12;
    return NULL;
}

// ��ȡ�ƶ�Ǳ����ָ��
const int* get_mobility_map(int rows, int cols) {
    if (rows == 8 && cols == 8) return (const int*)mobility_map_8;
    if (rows == 10 && cols == 10) return (const int*)mobility_map_10;
    if (rows == 12 && cols == 12) return (const int*)mobility_map_12;
    return NULL;
}

// ��ȡ������Ʊ�ָ��
const int* get_corner_map(int rows, int cols) {
    if (rows == 8 && cols == 8) return (const int*)corner_map_8;
    if (rows == 10 && cols == 10) return (const int*)corner_map_10;
    if (rows == 12 && cols == 12) return (const int*)corner_map_12;
    return NULL;
}

// ��̬DEPTH������12x12����DEPTH=4���������DEPTH=3
int get_dynamic_depth(int rows, int cols, int empty) {
    int total = rows * cols;
    if (total >= 100) { // 10x10������
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

// ��̬λ��Ȩ�أ���������ؼ汸��- ǿ��������ԺͶ�̬����Ȩ��
int dynamic_weight(int x, int y, int rows, int cols, double empty_ratio) {
    // ���� - ������ȼ�
    if ((x == 0 && y == 0) || (x == 0 && y == cols - 1) ||
        (x == rows - 1 && y == 0) || (x == rows - 1 && y == cols - 1))
        return (rows == 12 && cols == 12) ? 400 : 200;
    // X�㣨����б�Խ�һ��- ����
    if ((x == 1 && y == 1) || (x == 1 && y == cols-2) ||
        (x == rows-2 && y == 1) || (x == rows-2 && y == cols-2))
        return -80;
    // C�㣨��������һ��- ����
    if ((x == 0 && y == 1) || (x == 1 && y == 0) ||
        (x == 0 && y == cols-2) || (x == 1 && y == cols-1) ||
        (x == rows-1 && y == 1) || (x == rows-2 && y == 0) ||
        (x == rows-1 && y == cols-2) || (x == rows-2 && y == cols-1))
        return -60;
    // ��Ե - ��̬Ȩ��
    if (x == 0 || x == rows-1 || y == 0 || y == cols-1) {
        int edge_weight;
        if (rows == 12 && cols == 12) {
            edge_weight = 60; // 12x12����Ȩ������
        } else {
            if (empty_ratio > 2.0/3.0) edge_weight = 20;      // ǰ������
            else if (empty_ratio > 1.0/3.0) edge_weight = 10; // �����Խ�
            else edge_weight = 30;                            // ��������
        }
        // ������丽���ı�Եλ��
        if ((x == 0 && (y == 1 || y == cols-2)) ||
            (x == rows-1 && (y == 1 || y == cols-2)) ||
            (y == 0 && (x == 1 || x == rows-2)) ||
            (y == cols-1 && (x == 1 || x == rows-2)))
            return -30;
        return edge_weight;
    }
    // ����
    return 1;
}

// ���(i, j)�Ƿ�ΪΣ�ռ��Ŀյ㣨�����������ҷ����ӣ�
int is_sandwich_trap(char **board, int rows, int cols, int i, int j, char my, char opp) {
    if (board[i][j] != ' ') return 0;
    // �ĸ�����ˮƽ����ֱ�������Խ���
    int dirs[4][2][2] = {
        {{0, -1}, {0, 1}},    // ����
        {{-1, 0}, {1, 0}},    // ����
        {{-1, -1}, {1, 1}},   // ��������
        {{-1, 1}, {1, -1}}    // ��������
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
        // ԭ�С��Է�-��-�Է�����
        if (!safe1 || !safe2) {
            if (x1 >= 0 && x1 < rows && y1 >= 0 && y1 < cols &&
                x2 >= 0 && x2 < rows && y2 >= 0 && y2 < cols &&
                board[x1][y1] == opp && board[x2][y2] == opp) {
                return 1; // ��Σ�ռ��Ŀ�
            }
        }
        // �������ҷ�-�ҷ�-�ո�-�Է�����
        int xx1 = i - dirs[d][0][0], yy1 = j - dirs[d][0][1];
        int xx2 = i + 2*dirs[d][1][0], yy2 = j + 2*dirs[d][1][1];
        if (x1 >= 0 && x1 < rows && y1 >= 0 && y1 < cols &&
            xx1 >= 0 && xx1 < rows && yy1 >= 0 && yy1 < cols &&
            x2 >= 0 && x2 < rows && y2 >= 0 && y2 < cols &&
            xx2 >= 0 && xx2 < rows && yy2 >= 0 && yy2 < cols) {
            // ��顰�ҷ�-�ҷ�-�ո�-�Է���
            if (board[xx1][yy1] == my && board[x1][y1] == my && board[i][j] == ' ' && board[x2][y2] == opp) {
                return 1;
            }
            // ��顰�Է�-�ո�-�ҷ�-�ҷ���
            if (board[x1][y1] == opp && board[i][j] == ' ' && board[x2][y2] == my && board[xx2][yy2] == my) {
                return 1;
            }
        }
    }
    // ����ϱ������յ�����
    if (i == 0) { // �ϱ�
        int left = j, right = j;
        // ������չ
        while (left > 0 && board[0][left-1] == ' ') left--;
        // ������չ
        while (right < cols-1 && board[0][right+1] == ' ') right++;
        // �������[left, right]
        if (right - left + 1 >= 2) {
            int left_end = (left > 0) ? board[0][left-1] : 0;
            int right_end = (right < cols-1) ? board[0][right+1] : 0;
            // ��һ�����ҷ����ӣ���һ�˲���
            if ((left_end == my && right_end != my) || (right_end == my && left_end != my)) {
                return 1;
            }
        }
    }
    // ����±������յ�����
    if (i == rows-1) { // �±�
        int left = j, right = j;
        // ������չ
        while (left > 0 && board[rows-1][left-1] == ' ') left--;
        // ������չ
        while (right < cols-1 && board[rows-1][right+1] == ' ') right++;
        // �������[left, right]
        if (right - left + 1 >= 2) {
            int left_end = (left > 0) ? board[rows-1][left-1] : 0;
            int right_end = (right < cols-1) ? board[rows-1][right+1] : 0;
            // ��һ�����ҷ����ӣ���һ�˲���
            if ((left_end == my && right_end != my) || (right_end == my && left_end != my)) {
                return 1;
            }
        }
    }
    // �����������յ�����
    if (j == 0) { // ���
        int up = i, down = i;
        // ������չ
        while (up > 0 && board[up-1][0] == ' ') up--;
        // ������չ
        while (down < rows-1 && board[down+1][0] == ' ') down++;
        // �������[up, down]
        if (down - up + 1 >= 2) {
            int up_end = (up > 0) ? board[up-1][0] : 0;
            int down_end = (down < rows-1) ? board[down+1][0] : 0;
            // ��һ�����ҷ����ӣ���һ�˲���
            if ((up_end == my && down_end != my) || (down_end == my && up_end != my)) {
                return 1;
            }
        }
    }
    // ����ұ������յ�����
    if (j == cols-1) { // �ұ�
        int up = i, down = i;
        // ������չ
        while (up > 0 && board[up-1][cols-1] == ' ') up--;
        // ������չ
        while (down < rows-1 && board[down+1][cols-1] == ' ') down++;
        // �������[up, down]
        if (down - up + 1 >= 2) {
            int up_end = (up > 0) ? board[up-1][cols-1] : 0;
            int down_end = (down < rows-1) ? board[down+1][cols-1] : 0;
            // ��һ�����ҷ����ӣ���һ�˲���
            if ((up_end == my && down_end != my) || (down_end == my && up_end != my)) {
                return 1;
            }
        }
    }
    return 0;
}

// ���������У����ܸ��Է����ü���������ӷ�
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
    // ��ȡ����������
    const int* weight_map = get_weight_map(rows, cols);
    const int* stability_map = get_stability_map(rows, cols);
    const int* mobility_map = get_mobility_map(rows, cols);
    const int* corner_map = get_corner_map(rows, cols);
    // �����ƶ������������ظ����㣩
    my_moves = count_moves(board, rows, cols, my, opp);
    opp_moves = count_moves(board, rows, cols, opp, my);
    // ����Ƿ��н����ռ - ���輫�����ȼ�
    if (has_corner_move(board, rows, cols, my, opp)) {
        score += 1000; // ��������
    }
    if (has_corner_move(board, rows, cols, opp, my)) {
        score -= 1000; // ��ֹ����ռ����
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // ����Ȩ������
            int w = dynamic_weight(i, j, rows, cols, empty_ratio);
            if (board[i][j] == my) {
                score += w;
                my_cnt++;
                // ʹ���µ�������
                if (weight_map) score += weight_map[i * cols + j];
                if (stability_map) score += stability_map[i * cols + j];
                if (corner_map) score += corner_map[i * cols + j];
            } else if (board[i][j] == opp) {
                score -= w;
                opp_cnt++;
                // ���ֵĸ���
                if (weight_map) score -= weight_map[i * cols + j];
                if (stability_map) score -= stability_map[i * cols + j];
                if (corner_map) score -= corner_map[i * cols + j];
            } else {
                // ��λ���ƶ�Ǳ������
                if (mobility_map) {
                    int mobility_score = mobility_map[i * cols + j];
                    if (is_valid_custom(board, rows, cols, i, j, my, opp)) {
                        score += mobility_score;
                    }
                    if (is_valid_custom(board, rows, cols, i, j, opp, my)) {
                        score -= mobility_score;
                    }
                }
                // ����ҷ�����(i,j)���öԷ�����(i,j)ʱ�����ģ����ҷ��ӷ�
                if (is_sandwich_trap(board, rows, cols, i, j, opp, my)) {
                    score += 15; // ����Ը���ʵ�ʵ�����ֵ
                }
            }
        }
    }
    // �����ȶ���
    int my_stable = count_stable(board, rows, cols, my);
    int opp_stable = count_stable(board, rows, cols, opp);
    // ���������Ʒ���
    int corner_score = calculate_corner_score(board, rows, cols, my, opp);
    // �վ�����
    if (empty < 10) score += 100 * (my_cnt - opp_cnt);
    // ��̬�����ȶ���Ȩ��
    int stable_weight;
    if (empty_ratio > 2.0/3.0) {         // ǰ��
        stable_weight = 2;
    } else if (empty_ratio > 1.0/3.0) {  // ����
        stable_weight = 10;
    } else {                             // ����
        stable_weight = 40;
    }
    score += stable_weight * (my_stable - opp_stable);
    // �ۺϼ�Ȩ
    score += 5 * (my_moves - opp_moves);
    score += corner_score;
    return score;
}

// 3. alphabeta�ݹ飬����ͳһ����ֹԽ�磬memcpy��Χ�Ͻ�
struct Move { int x, y, score; };

int cmp_move(const void *a, const void *b) {
    return ((struct Move*)b)->score - ((struct Move*)a)->score;
}

int alphabeta(char board[MAX_SIZE][MAX_SIZE], int rows, int cols, int depth, int alpha, int beta, int is_max, char my, char opp, int buf_id) {
    if (depth == 0) return evaluate((char**)board, rows, cols, my, opp);
    if (buf_id+1 >= MAX_DEPTH) return evaluate((char**)board, rows, cols, my, opp); // ��ֹԽ��
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
                // ��������ģ��
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

// 4. will_give_corner����
int will_give_corner(char **board, int rows, int cols, int i, int j, char my, char opp) {
    // �þ�̬��������memcpy��Χ�Ͻ�
    memcpy(board_buf[0], board, sizeof(char)*rows*cols);
    do_move2(board_buf[0], rows, cols, i, j);
    int result = has_corner_move((char**)board_buf[0], rows, cols, opp, my);
    return result;
}

// 5. place��������ѭ����ʱ���ݹ��֧�����ʱ��
struct Point place(struct Player *player) {
    start_time = clock();
    int rows = player->row_cnt, cols = player->col_cnt;
    int best_score = INT_MIN;
    struct Point best = initPoint(-1, -1);
    int empty = 0;
    
    // �����λ����
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            if (player->mat[i][j] == ' ')
                empty++;
    
    int max_search_depth = get_dynamic_depth(rows, cols, empty);
    
    // ��ǰ�˳������û����Ч�ƶ�
    int valid_move_exists = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (is_valid(player, i, j)) {
                valid_move_exists = 1;
                best = initPoint(i, j);  // ����Ĭ���ƶ�
                break;
            }
        }
        if (valid_move_exists) break;
    }
    if (!valid_move_exists) return best;  // ����Ч�ƶ�ʱֱ�ӷ���
    
    // ��ѭ����ʱ��ÿ�������ʱ��
    for (int cur_depth = 1; cur_depth <= max_search_depth; cur_depth++) {
        int local_best_score = INT_MIN;
        struct Point local_best = initPoint(-1, -1);
        int updated = 0;
        
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                // ��ѭ��ʱ����
                if (time_exceeded()) {
                    if (local_best.X != -1) return local_best;
                    if (best.X != -1) return best;
                    return initPoint(i, j);
                }
                
                if (!is_valid(player, i, j)) continue;
                
                memcpy(board_buf[0], player->mat, sizeof(char)*rows*cols);
                do_move2(board_buf[0], rows, cols, i, j);
                
                // �ݹ��֧�����ʱ��
                int score = alphabeta(board_buf[0], rows, cols, cur_depth - 1, 
                                     INT_MIN, INT_MAX, 0, 'O', 'o', 0);
                
                if (score > local_best_score) {
                    local_best_score = score;
                    local_best = initPoint(i, j);
                    updated = 1;
                }
            }
        }
        
        // ����ȫ����ѽ��
        if (updated && local_best_score > best_score) {
            best_score = local_best_score;
            best = local_best;
        }
        
        // �����ɺ���ʱ��
        if (time_exceeded()) break;
    }
    
    // ȷ��ʼ�շ�����Ч�ƶ�
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