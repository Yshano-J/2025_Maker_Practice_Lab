#include <string.h>
#include "../include/playerbase.h"
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>

#define DEPTH 3  // �������
#define BONUS 10
// ��ʷ���������֧��16x16����
int history[16][16] = {0};

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

void do_move(char **board, int rows, int cols, int x, int y, char cur_piece, char opp_piece) {
    board[x][y] = cur_piece;
    for (int d = 0; d < 8; d++) {
        int nx = x + dirs[d][0];
        int ny = y + dirs[d][1];
        int cnt = 0;
        while (nx >= 0 && nx < rows && ny >= 0 && ny < cols && board[nx][ny] == opp_piece) {
            nx += dirs[d][0];
            ny += dirs[d][1];
            cnt++;
        }
        if (cnt > 0 && nx >= 0 && nx < rows && ny >= 0 && ny < cols && board[nx][ny] == cur_piece) {
            int tx = x + dirs[d][0], ty = y + dirs[d][1];
            for (int k = 0; k < cnt; k++) {
                board[tx][ty] = cur_piece;
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
        while (ix >= 0 && ix < rows && board[ix][y] == me) ix += (x == 0 ? 1 : -1);
        // ����
        while (iy >= 0 && iy < cols && board[x][iy] == me) iy += (y == 0 ? 1 : -1);
        int r1 = (x==0?0:ix+1), r2 = (x==0?ix-1:rows-1);
        int c1 = (y==0?0:iy+1), c2 = (y==0?iy-1:cols-1);
        for (int a = r1; a <= r2; a++)
            for (int b = c1; b <= c2; b++)
                if (board[a][b] == me) stable++;
    }
    return stable;
}

// ��̬λ��Ȩ�أ���������ؼ汸���ο�pk_player.h��
int dynamic_weight(int x, int y, int rows, int cols, char **board) {
    // ����
    if ((x == 0 && y == 0) || (x == 0 && y == cols - 1) ||
        (x == rows - 1 && y == 0) || (x == rows - 1 && y == cols - 1))
        return 120;
    //������б�Խ�һ���磨1,1����
    if ((x == 1 && y == 1) || (x == 1 && y == cols-2) ||
        (x == rows-2 && y == 1) || (x == rows-2 && y == cols-2))
        return -40;
    //����������һ���磨0,1������1,0����
    if ((x == 0 && y == 1) || (x == 1 && y == 0) ||
        (x == 0 && y == cols-2) || (x == 1 && y == cols-1) ||
        (x == rows-1 && y == 1) || (x == rows-2 && y == 0) ||
        (x == rows-1 && y == cols-2) || (x == rows-2 && y == cols-1))
        return -20;
    // ��
    if (x == 0 || x == rows-1 || y == 0 || y == cols-1)
        return 15;
    // ����
    return 1;
}

// �����Ƶ��ȶ��Ӵ����㷨
void mark_stable(char **board, int rows, int cols, char me, int stable[16][16]) {
    memset(stable, 0, sizeof(int) * 16 * 16);
    // ����Ľ�
    int dx[4] = {0, 0, rows-1, rows-1};
    int dy[4] = {0, cols-1, 0, cols-1};
    for (int d = 0; d < 4; d++) {
        int x = dx[d], y = dy[d];
        if (board[x][y] != me) continue;
        stable[x][y] = 1;
    }
    // �˷�����ƣ�ֻ�и÷���ȫΪ�����򵽱߽�����ȶ�
    int changed;
    do {
        changed = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (board[i][j] != me || stable[i][j]) continue;
                int stable_flag = 1;
                for (int d = 0; d < 8; d++) {
                    int ni = i, nj = j;
                    while (1) {
                        ni += dirs[d][0];
                        nj += dirs[d][1];
                        if (ni < 0 || ni >= rows || nj < 0 || nj >= cols) break; // ���߽�
                        if (board[ni][nj] != me) { stable_flag = 0; break; }
                    }
                    if (!stable_flag) break;
                }
                if (stable_flag) {
                    stable[i][j] = 1;
                    changed = 1;
                }
            }
        }
    } while (changed);
}

int count_stable_pro(char **board, int rows, int cols, char me) {
    int stable[16][16];
    mark_stable(board, rows, cols, me, stable);
    int cnt = 0;
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            if (stable[i][j]) cnt++;
    return cnt;
}

// ��������ؼ汸��������������������ͻ����רҵ���ȶ���ֻ�ڲоּ���Ȩ�ز���
int evaluate(char **board, int rows, int cols, char my, char opp) {
    int score = 0, my_moves = 0, opp_moves = 0, my_stable = 0, opp_stable = 0;
    int empty = 0, my_cnt = 0, opp_cnt = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int w = dynamic_weight(i, j, rows, cols, board);
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
    // �����ȶ���
    my_stable = count_stable(board, rows, cols, my);
    opp_stable = count_stable(board, rows, cols, opp);
    // ֻ�ڲо������վ������רҵ���ȶ��Ӽ���Ȩ��
    if (empty < 10) {
        score += 100 * (my_cnt - opp_cnt); // �վ�������
        int my_stable_pro = count_stable_pro(board, rows, cols, my);
        int opp_stable_pro = count_stable_pro(board, rows, cols, opp);
        score += 2 * (my_stable_pro - opp_stable_pro); // רҵ���ȶ��Ӽ���Ȩ��
    }
    // �����ȶ�������
    score += 20 * (my_stable - opp_stable);
    // �ж���
    score += 5 * (my_moves - opp_moves);
    return score;
}

// �߷��ṹ��
struct Move {
    int x, y;
    int score;
};

// �߷�����ȽϺ���������
int cmp_move(const void *a, const void *b) {
    return ((struct Move*)b)->score - ((struct Move*)a)->score;
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
		//AI�غϣ����غϣ�
        // �ռ����п����߷�
        struct Move moves[144];
        int move_cnt = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (!is_valid_custom(board, rows, cols, i, j, my, opp)) continue;
                moves[move_cnt].x = i;
                moves[move_cnt].y = j;
                moves[move_cnt].score = history[i][j] + dynamic_weight(i, j, rows, cols, board);
                move_cnt++;
            }
        }
        // ����
        qsort(moves, move_cnt, sizeof(struct Move), cmp_move);
        // �������˳������
        for (int m = 0; m < move_cnt; m++) {
            int i = moves[m].x, j = moves[m].y;
            found = 1;
            char **new_board = (char **)malloc(rows * sizeof(char *));
            for (int r = 0; r < rows; r++) {
                new_board[r] = (char *)malloc(cols * sizeof(char));
                memcpy(new_board[r], board[r], cols);
            }
            do_move(new_board, rows, cols, i, j, my, opp);
            int val = alphabeta(new_board, rows, cols, depth - 1, alpha, beta, 0, my, opp);
            for (int r = 0; r < rows; r++) free(new_board[r]);
            free(new_board);
            if (val > alpha) alpha = val;
            if (beta <= alpha) {
                history[i][j] += depth * BONUS;
                return alpha;
            }
        }
        if (!found) return evaluate(board, rows, cols, my, opp);
        return alpha;
    }
    else 
    {
		// ���ֻغ�
        // �ռ����п����߷�
        struct Move moves[144];
        int move_cnt = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (!is_valid_custom(board, rows, cols, i, j, opp, my)) continue;
                moves[move_cnt].x = i;
                moves[move_cnt].y = j;
                moves[move_cnt].score = history[i][j] + dynamic_weight(i, j, rows, cols, board);
                move_cnt++;
            }
        }
        // ����
        qsort(moves, move_cnt, sizeof(struct Move), cmp_move);
        // �������˳������
        for (int m = 0; m < move_cnt; m++) {
            int i = moves[m].x, j = moves[m].y;
            found = 1;
            char **new_board = (char **)malloc(rows * sizeof(char *));
            for (int r = 0; r < rows; r++) {
                new_board[r] = (char *)malloc(cols * sizeof(char));
                memcpy(new_board[r], board[r], cols);
            }
            // ��������ģ��
			do_move(new_board, rows, cols, i, j, opp, my);
            int val = alphabeta(new_board, rows, cols, depth - 1, alpha, beta, 1, my, opp);
            for (int r = 0; r < rows; r++) free(new_board[r]);
            free(new_board);
            if (val < beta) beta = val;
            if (beta <= alpha) {
                history[i][j] += depth * BONUS;
                return beta;
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
    clock_t start = clock();
    for (int cur_depth = 1; cur_depth <= DEPTH; cur_depth++) {
        int local_best_score = INT_MIN;
        struct Point local_best = initPoint(-1, -1);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (!is_valid(player, i, j)) continue;
                char **new_board = copy_board(player);
                do_move(new_board, rows, cols, i, j, 'O', 'o');
                int score = alphabeta(new_board, rows, cols, cur_depth - 1, INT_MIN, INT_MAX, 0, 'O', 'o');
                free_board(new_board, rows);
                if (score > local_best_score) {
                    local_best_score = score;
                    local_best = initPoint(i, j);
                }
            }
        }
        // ÿ�μ����¼��ǰ����
        if (local_best_score > best_score || best.X == -1) {
            best_score = local_best_score;
            best = local_best;
        }
        // ʱ���жϣ����������5ms����
        clock_t now = clock();
        double elapsed_ms = (now - start) * 1000.0 / CLOCKS_PER_SEC;
        if (elapsed_ms > 95) break;
    }
    return best;
}