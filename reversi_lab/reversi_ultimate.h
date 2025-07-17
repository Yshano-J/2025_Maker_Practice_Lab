#include <string.h>
#include "../include/playerbase.h"
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>

#define DEPTH 3  // �������
#define CORNER_WEIGHT 50
#define EDGE_WEIGHT 15

// ��������
static const int dirs[8][2] = {{0,1}, {0,-1}, {1,1}, {1,0}, {1,-1}, {-1,0}, {-1,1}, {-1,-1}};

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

// λ�ü�ֵ����
int position_value(int x, int y, int rows, int cols) {
    // �ǵ�λ�ü�ֵ���
    if ((x == 0 && y == 0) || (x == 0 && y == cols - 1) ||
        (x == rows - 1 && y == 0) || (x == rows - 1 && y == cols - 1)) {
        return CORNER_WEIGHT;
    }
    
    // ��λ�ü�ֵ�θ�
    if (x == 0 || x == rows - 1 || y == 0 || y == cols - 1) {
        return EDGE_WEIGHT;
    }
    
    // ��ͨλ�ü�ֵ
    return 1;
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

// ��̬λ��Ȩ�أ�������㣩
int dynamic_weight(int x, int y, int rows, int cols) {
    // ����
    if ((x == 0 && y == 0) || (x == 0 && y == cols - 1) ||
        (x == rows - 1 && y == 0) || (x == rows - 1 && y == cols - 1))
        return 150;
    // X-square
    if ((x == 1 && y == 1) || (x == 1 && y == cols-2) ||
        (x == rows-2 && y == 1) || (x == rows-2 && y == cols-2))
        return -80;
    // C-square
    if ((x == 0 && y == 1) || (x == 1 && y == 0) ||
        (x == 0 && y == cols-2) || (x == 1 && y == cols-1) ||
        (x == rows-1 && y == 1) || (x == rows-2 && y == 0) ||
        (x == rows-1 && y == cols-2) || (x == rows-2 && y == cols-1))
        return -80;
    // ��
    // if (x == 0 || x == rows-1 || y == 0 || y == cols-1)
    //     return 15;
    // // ����
    return 1;
}

// ��ǿ��������
int evaluate(char **board, int rows, int cols, char my, char opp) {
    int score = 0, my_moves = 0, opp_moves = 0, my_stable = 0, opp_stable = 0;
    int empty = 0, my_cnt = 0, opp_cnt = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int w = dynamic_weight(i, j, rows, cols);
            if (board[i][j] == my) {
                score += w;
                my_cnt++;
            } else if (board[i][j] == opp) {
                score -= w;
                opp_cnt++;
            } else empty++;
            if (is_valid_custom(board, rows, cols, i, j, my, opp)) my_moves++;
            if (is_valid_custom(board, rows, cols, i, j, opp, my)) opp_moves++;
        }
    }
    my_stable = count_stable(board, rows, cols, my);
    opp_stable = count_stable(board, rows, cols, opp);
    // �վ�����
    if (empty < 10) score += 100 * (my_cnt - opp_cnt);
    // �ۺϼ�Ȩ
    score += 20 * (my_stable - opp_stable);
    score += 5 * (my_moves - opp_moves);
    return score;
}

//��minimax()�Ͻ���alphabeta��֦�Ż�
// Alpha-Beta��֦����С����
int alphabeta(char **board, int rows, int cols, int depth, int alpha, int beta, int is_max, char my, char opp) {
    if (depth == 0) {
        return evaluate(board, rows, cols, my, opp);
    }
    int found = 0;
    if (is_max) 
    {
        //AI�غ�(���غ�)
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (!is_valid_custom(board, rows, cols, i, j, my, opp)) continue;
                found = 1;
                char **new_board = (char **)malloc(rows * sizeof(char *));
                for (int r = 0; r < rows; r++) {
                    new_board[r] = (char *)malloc(cols * sizeof(char));
                    memcpy(new_board[r], board[r], cols);
                }
                do_move(new_board, rows, cols, i, j);
                int val = alphabeta(new_board, rows, cols, depth - 1, alpha, beta, 0, my, opp);
                for (int r = 0; r < rows; r++) free(new_board[r]);
                free(new_board);
                if (val > alpha) alpha = val;
                if (beta <= alpha) return alpha;
            }
        }
        if (!found) return evaluate(board, rows, cols, my, opp);
        return alpha;
    }
    else 
    {
        //���ֻغ�
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (!is_valid_custom(board, rows, cols, i, j, opp, my)) continue;
                found = 1;
                char **new_board = (char **)malloc(rows * sizeof(char *));
                for (int r = 0; r < rows; r++) {
                    new_board[r] = (char *)malloc(cols * sizeof(char));
                    memcpy(new_board[r], board[r], cols);
                }
                // ��������ģ��
                new_board[i][j] = opp;
                for (int d = 0; d < 8; d++) {
                    int nx = i + dirs[d][0];
                    int ny = j + dirs[d][1];
                    int cnt = 0;
                    while (nx >= 0 && nx < rows && ny >= 0 && ny < cols && new_board[nx][ny] == my) {
                        nx += dirs[d][0];
                        ny += dirs[d][1];
                        cnt++;
                    }
                    if (cnt > 0 && nx >= 0 && nx < rows && ny >= 0 && ny < cols && new_board[nx][ny] == opp) {
                        int tx = i + dirs[d][0], ty = j + dirs[d][1];
                        for (int k = 0; k < cnt; k++) {
                            new_board[tx][ty] = opp;
                            tx += dirs[d][0];
                            ty += dirs[d][1];
                        }
                    }
                }
                int val = alphabeta(new_board, rows, cols, depth - 1, alpha, beta, 1, my, opp);
                for (int r = 0; r < rows; r++) free(new_board[r]);
                free(new_board);
                if (val < beta) beta = val;
                if (beta <= alpha) return beta;
            }
        }
        if (!found) return evaluate(board, rows, cols, my, opp);
        return beta;
    }
}

// place����������������ӵ�
struct Point place(struct Player *player) {
    int rows = player->row_cnt, cols = player->col_cnt;
    int best_score = INT_MIN;
    struct Point best = initPoint(-1, -1);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (!is_valid(player, i, j)) continue;
            char **new_board = copy_board(player);
            do_move(new_board, rows, cols, i, j);
            int score = alphabeta(new_board, rows, cols, DEPTH - 1, INT_MIN, INT_MAX, 0, 'O', 'o');
            free_board(new_board, rows);
            if (score > best_score) {
                best_score = score;
                best = initPoint(i, j);
            }
        }
    }
    return best;
}