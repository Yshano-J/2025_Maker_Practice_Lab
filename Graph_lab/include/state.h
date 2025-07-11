#ifndef STATE_H_
#define STATE_H_
#include "suan_png.h"
#include "pxl.h"

#define MAX 65536
#define INF 0x3f3f3f3f

struct State
{
    int visited[MAX];
    int pathLength[MAX]; // ��·������
    int minPath[MAX];    // �����··��
    int secondMinPath;   // �ζ�·
    int deletedEdge;     // ɾȥ�ı�
    int row;
    int column;
};

void init_State(struct State *s);
void delete_State(struct State *s);
void assign(struct State *a, struct State *b);
void parse(struct State *s, struct PNG *p);
int solve1(struct State *s);
int solve2(struct State *s);

#endif