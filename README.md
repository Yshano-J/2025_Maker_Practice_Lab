# 2025_Maker_Practice_Lab 🚀🚀🚀
   #### Security Maker Practice Experiment for the Third Semester of 2025, National School of Cyber Security, Wuhan University
   #### 😎🥰👻🥳 无敌的 “蒜头君的小弟”

## 一、黑白棋实验说明

### 1. 黑白棋的规则
### 2. 游戏有两名 AI 玩家（由两份代码控制下棋）
    - 机器人 AI
    - 你的 AI
### 3. 实验任务
你需要在 `player.h` 文件中编写 `struct Point place(struct Player *player);` 函数，你的 AI 程序会在你的回合中根据此函数决策落子的位置。当一方本回合无可落子位置时，返回 `-1 -1` 表示放弃本回合。

---

### 游戏规则
在经典黑白棋规则的基础之上，增加一个额外信息：**棋子分数**。对于棋盘上的每个格子，均有一个 1 至 9 之间的分数，格子上的分数属于格子上的棋子所属人。

现在计算机每回合决策的程序（`computer.h`）已经写好了，你需要设计你的 AI 程序（`player.h`），用于决策在你的回合中落子的位置。当一方本回合无可落子位置时，返回 `-1 -1` 表示放弃本回合。

对于你的 AI：
- 己方棋子在棋盘上用大写 `O` 表示
- 对方棋子用小写 `o` 表示
- 初始每个人的棋子所在格子的分数为 0
- 其余可通过 `mat` 初始时的值得到
- 如果移动不合法（包含移动到不合法位置、在不必放弃的回合直接放弃等），则分数直接记为 **-100**

---

### 任务一
- 地图中所有格子除了中间四个以外的分数均为 1
- 程序编译通过可以获得基础 **10 分**
- 给定三张地图（在 `data/map*.txt` 中查看）
- 对手 AI 代码在 `code/computer.h` 中查看
- 每张地图与对手 AI 对战两次
- **评分规则**：若总分大于对手，则该张图获得 **30 分**，否则不得分

### 任务二
- 地图中所有格子除了中间四个以及四个角以外的分数均为 1
- 给定四张地图（在 `data/map*.txt` 中查看）
- 每张地图与对手 AI 对战两次
- **评分规则**：若总分大于对手，则该张图获得 **25 分**，否则不得分

### 任务三
- 地图中所有格子除了中间四个以及最外层一圈以外的格子分数均为 1
- 给定四张地图（在 `data/map*.txt` 中查看）
- 每张地图与对手 AI 对战两次
- **评分规则**：若总分大于对手，则该张图获得 **25 分**，否则不得分

### 任务四
- 地图中除了中间四个格子以外的其余格子的分数范围为 **1～9**
- 给定四张地图（在 `data/map*.txt` 中查看）
- 每张地图与对手 AI 对战两次
- **评分规则**：若总分大于对手，则该张图获得 **25 分**，否则不得分

---

## PlayerBase 结构体
```c
struct Player {
    char **mat;          // Game Data
    int row_cnt;         // Count of rows in current map
    int col_cnt;         // Count of columns in current map
    int your_score;      // Your AI's score
    int opponent_score;  // Opponent AI's score
};

struct Point place(struct Player *player);  // Place function to implement
void init(struct Player *player);           // Init function to implement
```

### 测试运行

- 执行 `./run.sh` 脚本即可测试
- 在 `logs` 目录内查看游戏过程和最终分数
- 执行 `./run.sh --visible` 进行可视化测试
- 将要测试的地图内容复制到 `data/map.txt` 中进行测试

### 限制说明

#### 时间限制
- 初始化时间限制：**1s**
- 每回合移动计算时间限制：**100ms**
- 若超时，系统认为此次不移动

#### 空间限制
- 空间限制为 **512MB**（最终评测时生效）

#### 函数调用限制
**禁止使用以下系统调用及函数**：
- 新建进程（调用 `fork`）
- 读写文件
- 调用 PlayerBase 类的 `_recv`, `_send`, `_work`, `_init_thread`, `_walk_thread`, `_syscall_check` 方法

### 程序异常处理
若 AI 进程在某次 `place` 计算时异常退出，则系统认为该 AI 失败，游戏结束。

*** 

## 二、蒜头抢分大作战实验说明
### 游戏简介
两只蒜头将在一个地图上拼命地抢分，谁最终的分数更高，谁就获得比赛的胜利。与此同时，地图上还有两个破坏者，意图破坏两只蒜头抢分，以达到自己不可告人的目的。你将要控制其中一只蒜头，运用智慧帮助你的蒜头取得比赛的胜利。接下来，我们会为你详细介绍游戏规则。

### 游戏规则

### 角色介绍
1. 游戏只有 2 种角色：蒜头和破坏者
2. 每局比赛初始均有 2 只蒜头：
   - 你需要写出 AI 控制其中一只蒜头
   - 另一只蒜头将由其他同学写出的 AI 进行控制
3. 每局比赛初始均有 2 个破坏者
4. 保证初始时任意两个角色不会在地图的同一位置中出现
5. 保证两个破坏者的角色对两位蒜头是绝对公平的

### 地图介绍
1. 地图为 n 行 m 列由格子组成的矩阵。每个格子为以下几种类型中的一种：
   - **墙**：任何角色都无法移动到这种格子（`#`）
   - **地面**：角色可以移动到这种格子上（`.`）
   - **带有普通星的地面**：`o`（小写字母 o）
   - **带有超级星的地面**：`O`（大写字母 O）
2. 各种角色的初始位置都一定是地图中的地面而非墙
3. 保证地图对两只蒜头是绝对公平的
4. 坐标表示：`(x - 1, y - 1)` 表示地图中第 x 行 y 列的格子（下标从 0 开始）

### 移动规则
1. 每个角色当前回合可以移动至上下左右四个相邻地面格子中的一个，或选择不移动
2. 蒜头和蒜头之间、破坏者和破坏者之间不会发生碰撞
3. 破坏者移动规则：
   - 到蒜头 A 的最短距离为 a，到蒜头 B 的最短距离为 b
   - 朝蒜头 A 移动的概率：$\frac{b}{a + b}$
   - 朝蒜头 B 移动的概率：$\frac{a}{a + b}$
   - 若蒜头处于强化状态或已消失，破坏者将等概率随机游走
4. 若 AI 移动到非法位置，则视为自杀

### 回合流程
1. 每个回合分为两部分：
   - **蒜头移动阶段**：每只蒜头同时移动
   - **破坏者移动阶段**：每个破坏者同时移动

### 抢分规则
1. 蒜头与普通星重合：增加 10 分
2. 蒜头与超级星重合：
   - 增加 10 分
   - 蒜头变为强化状态
3. 破坏者与普通状态蒜头重合：蒜头消失并扣 500 分
4. 强化状态蒜头与破坏者重合：增加 200 分
5. 蒜头间重合：
   - 强化状态蒜头 vs 普通状态蒜头：普通蒜头消失，分数转移
   - 强化状态蒜头 vs 强化状态蒜头：无事发生

### 优先级（从高到低）
1. 强化蒜头 → 普通蒜头
2. 强化蒜头 → 破坏者
3. 破坏者 → 普通蒜头
4. 蒜头收集星星

### 状态转换
1. 普通蒜头 + 超级星 → 强化状态（持续 20 回合）
2. 强化蒜头 + 超级星 → 刷新强化持续时间至 20 回合
3. 破坏者被吃掉后：回到初始位置并有短暂强化期

### 计分规则
1. 每回合存活蒜头扣 1 分
2. 收集普通星：+10 分
3. 收集超级星：+10 分
4. 被破坏者吃掉：-500 分
5. 吃掉破坏者：+200 分
6. 被其他蒜头吃掉：转移 $\lfloor \frac{score}{2} \rfloor$ 分数
7. 自杀：-400 分

### 结束条件
1. 两只蒜头均已消失
2. 游戏进行 $n \times m$ 回合（$n$ 行 $m$ 列）
3. 地图上所有星星消失

---

### 目录概览
```c
|-- bin
|   |-- check_computer
|   |-- check_player
|   |-- computer
|   |-- judge
|   `-- player
|-- code
|   |-- computer.h
|   `-- player.h
|-- data
|   `-- map.txt
|-- include
|   `-- playerbase.h
|-- lib
|   `-- libplayer.a
|-- log
|-- Makefile
|-- run.sh
`-- src
    |-- check_computer.c
    |-- check_player.c
    |-- main_computer.c
    `-- main_player.c
```

---

### 文件说明
- 在 `code/player.h` 文件中实现 AI 算法
- **不要修改** Makefile 和其他脚本/头文件
- 所有代码逻辑必须实现在 `player.h` 文件内

---

### 地图格式
`data/map.txt` 示例（10×10 地图）：
1. 第一行：两个整数 `n m`（行数 列数）
2. 接下来 `n` 行：每行 `m` 个字符表示地图
3. 接下来 4 行：破坏者1、破坏者2、对手蒜头、你的蒜头的初始坐标 `(x,y)`

符号说明：
| 类型 | 符号 |
|------|------|
| 墙 | `#` |
| 普通星地面 | `o` |
| 超级星地面 | `O` |
| 普通地面 | `.` |

---

### PlayerBase 结构体
```c
struct Player {
    char **mat; // Game Data

    int row_cnt; // Count of rows in current map
    int col_cnt; // Count of columns in current map

    int ghost_posx[2]; // X-coordinate of 2 ghosts' position
    int ghost_posy[2]; // Y-coordinate of 2 ghosts' position

    int your_posx; // X-coordinate of your AI's position
    int your_posy; // Y-coordinate of your AI's position

    int opponent_posx; // X-coordinate of opponent AI's position
    int opponent_posy; // Y-coordinate of opponent AI's position

    int your_status; // -1=died, x>0=super status rounds, 0=normal
    int opponent_status; // -1=died, x>0=super status rounds, 0=normal

    int your_score; // Your AI's score
    int opponent_score; // Opponent AI's score
};

struct Point walk(struct Player *player); // 需实现的移动函数
void init(struct Player *player); // 初始化函数
```
### 测试运行
- 执行 `./run.sh` 脚本测试
- 在 `logs` 目录查看游戏过程和分数
- 执行 `./run.sh --visible` 进行可视化测试

### 时间限制
1. **初始化时间限制**：1s  
2. **每回合移动计算时间限制**：100ms  
3. **超时处理**：视为不移动  
4. **评测保证**：程序在本地时间限制内完成，评测机上必能获得预期结果

### 空间限制
- 最终评测空间限制：**256MB**

### 函数调用限制
**禁止使用以下系统调用及函数**：
1. 新建进程（调用 `fork`）
2. 读写文件
3. 调用 PlayerBase 类的：
   - `_recv`
   - `_send`
   - `_work`
   - `_init_thread`
   - `_walk_thread`
   - `_syscall_check`

### 程序异常处理
- 若 AI 在 `walk` 计算时异常退出 → 视为不移动
- 程序异常退出或运行超时 → 数据恢复至**上一回合状态**
