#include "state.h"
#include <string.h>
#include <stdbool.h>

int point_weight[MAX];
int path[MAX];

typedef struct Edge
{
    int vertex; // 终点坐标
    int next;
} Edge;

Edge edge[MAX];  // 边数组
int head[MAX];   // 头结点
int edgeNum = 0; // 边的数量
int nodeNum = 0;
int is_get_min_path = 0;

void init_State(struct State *s)
{
    for (int i = 0; i <= MAX; i++)
    {
        s->pathLength[i] = INF;
        s->visited[i] = 0;
        s->minPath[i] = 0;
        s->secondMinPath = INF;
    }
    s->row = 0;
    s->column = 0;
}

void delete_State(struct State *s)
{
    for (int i = 0; i <= MAX; i++)
    {
        s->pathLength[i] = 0;
        s->visited[i] = 0;
        s->minPath[i] = 0;
        s->secondMinPath = 0;
    }
    s->row = 0;
    s->column = 0;
}

bool max(int x, int y)
{
    return x > y ? x : y;
}

void insertEdge(int u, int v)
{
    edge[++edgeNum].vertex = v;
    edge[edgeNum].next = head[u];
    head[u] = edgeNum;
}

void buildEdge(struct State *s, int weight, int column)
{
    int maxLine = s->column;
    point_weight[++nodeNum] = weight;
    if (s->row % 2 == 0)
    {
        insertEdge(nodeNum, nodeNum - maxLine);
        insertEdge(nodeNum - maxLine, nodeNum);
        insertEdge(nodeNum, nodeNum - maxLine + 1);
        insertEdge(nodeNum - maxLine + 1, nodeNum);
    }
    if (s->row % 2 == 1)
    {
        if (s->row > 1)
        {
            if (column < maxLine)
            {
                insertEdge(nodeNum, nodeNum - (maxLine - 1));
                insertEdge(nodeNum - (maxLine - 1), nodeNum);
            }
            if (column > 1)
            {
                insertEdge(nodeNum, nodeNum - (maxLine - 1) - 1);
                insertEdge(nodeNum - (maxLine - 1) - 1, nodeNum);
            }
        }
    }
    if (column > 1)
    {
        insertEdge(nodeNum, nodeNum - 1);
        insertEdge(nodeNum - 1, nodeNum);
    }
}

int GetWeight(struct PNG *p, int w, int h)
{
    int r, g, b;
    r = get_PXL(p, w, h)->red;
    g = get_PXL(p, w, h)->green;
    b = get_PXL(p, w, h)->blue;
    return 255 * 255 * 3 - r * r - g * g - b * b;
}

void parse(struct State *s, struct PNG *p)
{
    //p：按像素点采样
    int height =p->height;
    int width = p->width;
    int weight;
    s->row = 1;
    for (int h = 6; h < height; h += 8)
    {
        int line = 1;//当前行已加入的州的个数
        for (int w = 6; w < width; w += 8)
        {
            weight = GetWeight(p, w, h);
            if (weight == 0)
                continue;
            buildEdge(s, weight, line);
            line++;
        }
        if (s->row == 1)
        {
            s->column = line - 1;//表示第一行（奇数行）州的个数
        }
        line = 1;
        (s->row)++;
    }
    return;
}

void dijkstra(struct State *s)
{
    memset(s->visited, 0, sizeof(s->visited));
    for (int i = 0; i <= MAX; i++)
    {
        s->pathLength[i] = INF;
    }
    s->visited[1] = 1;
    s->pathLength[1] = 0;
    int k = 1;            // 加入路径的点的数量
    int currentPoint = 1; // 当前所选择的点
    if(!is_get_min_path) {
        s->minPath[1] = -1;   //记录最短路：记录前驱
        s->deletedEdge = -1;
    }
    while (k < nodeNum)
    {
        for (int i = head[currentPoint]; i != 0; i = edge[i].next)
        {
            int nextv = edge[i].vertex;
            if (i != s->deletedEdge && !s->visited[nextv] && 
                s->pathLength[nextv] > s->pathLength[currentPoint] + point_weight[nextv])
            {
                s->pathLength[nextv] = s->pathLength[currentPoint] + point_weight[nextv];
                if(!is_get_min_path) s->minPath[nextv] = currentPoint;
            }
        }
        int minDistance = INF;
        for (int i = 1; i <= nodeNum; i++)
        {
            if (!s->visited[i] && minDistance > s->pathLength[i])
            {
                currentPoint = i;
                minDistance = s->pathLength[i];
            }
        }
        if (minDistance == INF)
            break;
        s->visited[currentPoint] = 1;
        s->pathLength[currentPoint] = minDistance;
        k++;
    }
}

int solve1(struct State *s)
{
    dijkstra(s);
    is_get_min_path = 1;
    return s->pathLength[nodeNum];
}

int solve2(struct State *s)
{
    int u = 0; 
    int minLength = s->pathLength[nodeNum];
    for (int i = nodeNum; i != -1; i = s->minPath[i])
    {
        u = s->minPath[i];
        int tempEdge = 0;
        for (int p = head[u]; p; p = edge[p].next)
        {
            if (edge[p].vertex == i)
            {
                tempEdge = p;
                break;
            }
        }
        s->deletedEdge = tempEdge; // 删除边

        dijkstra(s);
        if (s->pathLength[nodeNum] > minLength && s->secondMinPath > s->pathLength[nodeNum])
        {
            s->secondMinPath = s->pathLength[nodeNum];
        }
    }
    return s->secondMinPath;
}