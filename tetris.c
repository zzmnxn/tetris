#include "tetris.h"
#include <limits.h>

static struct sigaction act, oact;

typedef struct Node {
    char name[NAMELEN];
    int score;
    struct Node *link;
} Node;

int rank_cnt = 0;
Node *head = NULL;
RecNode *Recroot = NULL;
int recommendY, recommendX, recommendR;
long long space=0;
int main() {
    int exit = 0;

    initscr();
    noecho();
    keypad(stdscr, TRUE);

    srand((unsigned int)time(NULL));
    createRankList();

    while (!exit) {
        clear();
        switch (menu()) {
            case MENU_PLAY: play(); break;
            case MENU_RANK: rank(); break;
            case MENU_EXIT: exit = 1; break;
            case MENU_REC_PLAY: recommendedPlay(); break;
            default: break;
        }
    }

    endwin();
    system("clear");
    free(Recroot);
    return 0;
}

void InitTetris() {
    int i, j;

    for (j = 0; j < HEIGHT; j++)
        for (i = 0; i < WIDTH; i++)
            field[j][i] = 0;

    for (int i = 0; i < 3; i++) {
        nextBlock[i] = rand() % 7;
    }
    blockRotate = 0;
    blockY = -1;
    blockX = WIDTH / 2 - 2;
    score = 0;    
    gameOver = 0;
    timed_out = 0;
    Recroot = (RecNode *)malloc(sizeof(RecNode));    
    Recroot->level = 0;
    Recroot->accumulatedScore = 0;
    Recroot->curBlockID = nextBlock[0];
    memcpy(Recroot->reckField, field, sizeof(field));
    modified_recommend(Recroot);

    DrawOutline();
    DrawField();
    DrawBlockWithFeatures(blockY, blockX, nextBlock[0], blockRotate);
    DrawNextBlock(nextBlock);
    PrintScore(score);
}

void DrawOutline() {    
    int i, j;
    DrawBox(0, 0, HEIGHT, WIDTH);
    move(2, WIDTH + 10);
    printw("NEXT BLOCK");
    DrawBox(3, WIDTH + 10, 4, 8);
    DrawBox(9, WIDTH + 10, 4, 8);
    move(16, WIDTH + 10);
    printw("SCORE");
    DrawBox(17, WIDTH + 10, 1, 8);
}

int GetCommand() {
    int command;
    command = wgetch(stdscr);
    switch (command) {
        case KEY_UP: break;
        case KEY_DOWN: break;
        case KEY_LEFT: break;
        case KEY_RIGHT: break;
        case ' ':
            break;
        case 'q':
        case 'Q':
            command = QUIT;
            break;
        default:
            command = NOTHING;
            break;
    }
    return command;
}

int ProcessCommand(int command) {
    int ret = 1;
    int drawFlag = 0;
    switch (command) {
        case QUIT:
            ret = QUIT;
            break;
        case KEY_UP:
            if ((drawFlag = CheckToMove(field, nextBlock[0], (blockRotate + 1) % 4, blockY, blockX)))
                blockRotate = (blockRotate + 1) % 4;
            break;
        case KEY_DOWN:
            if ((drawFlag = CheckToMove(field, nextBlock[0], blockRotate, blockY + 1, blockX)))
                blockY++;
            break;
        case KEY_RIGHT:
            if ((drawFlag = CheckToMove(field, nextBlock[0], blockRotate, blockY, blockX + 1)))
                blockX++;
            break;
        case KEY_LEFT:
            if ((drawFlag = CheckToMove(field, nextBlock[0], blockRotate, blockY, blockX - 1)))
                blockX--;
            break;
        default:
            break;
    }
    if (drawFlag) DrawChange(field, command, nextBlock[0], blockRotate, blockY, blockX);
    return ret;    
}

void DrawField() {
    int i, j;
    for (j = 0; j < HEIGHT; j++) {
        move(j + 1, 1);
        for (i = 0; i < WIDTH; i++) {
            if (field[j][i] == 1) {
                attron(A_REVERSE);
                printw(" ");
                attroff(A_REVERSE);
            } else {
                printw(".");
            }
        }
    }
}

void PrintScore(int score) {
    move(18, WIDTH + 11);
    printw("%8d", score);
}

void DrawNextBlock(int *nextBlock) {
    int i, j;
    for (i = 0; i < 4; i++) {
        move(4 + i, WIDTH + 13);
        for (j = 0; j < 4; j++) {
            if (block[nextBlock[1]][0][i][j] == 1) {
                attron(A_REVERSE);
                printw(" ");
                attroff(A_REVERSE);
            } else {
                printw(" ");
            }
        }
    }

    for (i = 0; i < 4; i++) {
        move(10 + i, WIDTH + 13);
        for (j = 0; j < 4; j++) {
            if (block[nextBlock[2]][0][i][j] == 1) {
                attron(A_REVERSE);
                printw(" ");
                attroff(A_REVERSE);
            } else {
                printw(" ");
            }
        }
    }
}

void DrawBlock(int y, int x, int blockID, int blockRotate, char tile) {
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            if (block[blockID][blockRotate][i][j] == 1 && i + y >= 0) {
                move(i + y + 1, j + x + 1);
                attron(A_REVERSE);
                printw("%c", tile);
                attroff(A_REVERSE);
            }
        }
    move(HEIGHT, (WIDTH + 10));
}

void DrawBox(int y, int x, int height, int width) {
    int i, j;
    move(y, x);
    addch(ACS_ULCORNER);
    for (i = 0; i < width; i++)
        addch(ACS_HLINE);
    addch(ACS_URCORNER);
    for (j = 0; j < height; j++) {
        move(y + j + 1, x);
        addch(ACS_VLINE);
        move(y + j + 1, x + width + 1);
        addch(ACS_VLINE);
    }
    move(y + j + 1, x);
    addch(ACS_LLCORNER);
    for (i = 0; i < width; i++)
        addch(ACS_HLINE);
    addch(ACS_LRCORNER);
}

void play() {
    int command;
    clear();
    act.sa_handler = BlockDown;
    sigaction(SIGALRM, &act, &oact);
    InitTetris();
    do {
        if (timed_out == 0) {
            alarm(1);
            timed_out = 1;
        }

        command = GetCommand();
        if (ProcessCommand(command) == QUIT) {
            alarm(0);
            DrawBox(HEIGHT / 2 - 1, WIDTH / 2 - 5, 1, 10);
            move(HEIGHT / 2, WIDTH / 2 - 4);
            printw("Good-bye!!");
            refresh();
            getch();
            return;
        }
    } while (!gameOver);

    alarm(0);
    getch();
    DrawBox(HEIGHT / 2 - 1, WIDTH / 2 - 5, 1, 10);
    move(HEIGHT / 2, WIDTH / 2 - 4);
    printw("GameOver!!");
    refresh();
    getch();
    newRank(score);
}

char menu() {
    printw("1. play\n");
    printw("2. rank\n");
    printw("3. recommended play\n");
    printw("4. exit\n");
    return wgetch(stdscr);
}

int CheckToMove(char f[HEIGHT][WIDTH], int currentBlock, int blockRotate, int blockY, int blockX) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (block[currentBlock][blockRotate][i][j] == 1) {
                if (f[blockY + i][blockX + j])
                    return 0;
                else if ((blockY + i) >= HEIGHT || (blockX + j) >= WIDTH)
                    return 0;
                else if ((blockX + j) < 0 || (blockY + i) < 0)
                    return 0;
            }
        }
    }
    return 1;
}

void DrawChange(char f[HEIGHT][WIDTH], int command, int currentBlock, int blockRotate, int blockY, int blockX) {
    int b_blockX = blockX;
    int b_blockY = blockY;
    int b_Rotate = blockRotate;

    switch (command) {
        case KEY_UP:
            b_Rotate = (b_Rotate + 3) % 4;
            break;
        case KEY_DOWN:
            b_blockY -= 1; 
            break;
        case KEY_LEFT:
            b_blockX += 1; break;
        case KEY_RIGHT:
            b_blockX -= 1; break;
    }

    DrawBlock(b_blockY, b_blockX, currentBlock, b_Rotate, '.');
    DrawField();
    DrawBlock(blockY, blockX, currentBlock, blockRotate, ' ');
    DrawBlockWithFeatures(blockY, blockX, currentBlock, blockRotate);
    //move(HEIGHT, WIDTH + 10);
}

void BlockDown(int sig) {
    if (CheckToMove(field, nextBlock[0], blockRotate, blockY + 1, blockX)) {
        blockY++;
        DrawChange(field, KEY_DOWN, nextBlock[0], blockRotate, blockY, blockX);
        DrawBlockWithFeatures(blockY, blockX, nextBlock[0], blockRotate);
    }else{
        if (blockY == -1)
            gameOver = 1;
        else
        {
            score += AddBlockToField(field, nextBlock[0], blockRotate, blockY, blockX);
            score += DeleteLine(field);
            PrintScore(score);
            for (int i = 0; i < VISIBLE_BLOCKS - 1; i++)
            {
                nextBlock[i] = nextBlock[i + 1];
            }
            nextBlock[VISIBLE_BLOCKS - 1] = rand() % 7;

            blockY = -1;
            blockX = WIDTH / 2 - 2;
            blockRotate = 0;
            Recroot->curBlockID = nextBlock[0];
            memcpy(Recroot->reckField, field, sizeof(field));
            modified_recommend(Recroot);
            DrawField();
            DrawBlockWithFeatures(blockY, blockX, nextBlock[0], blockRotate);
            DrawNextBlock(nextBlock);
        }
    }

    timed_out = 0;
}

int AddBlockToField(char f[HEIGHT][WIDTH], int currentBlock, int blockRotate, int blockY, int blockX) {
    int touched = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (block[currentBlock][blockRotate][i][j]) {
                f[blockY + i][blockX + j] = 1;
                if (f[blockY + i + 1][blockX] == 1 || blockY + i >= HEIGHT - 1) {
                    touched++;
                }
            }
        }
    }
    return (touched * 10);
}

int DeleteLine(char f[HEIGHT][WIDTH]) {
    int j;
    int flag = 1;
    int count = 0;
    for (int i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            if (f[i][j] == 0) {
                flag = 0;
                break;
            }
        }
        if (flag) {
            for (int k = i; k > 0; k--) {
                for (int l = 0; l < WIDTH; l++) {
                    f[k][l] = f[k - 1][l];
                }
            }
            for (int l = 0; l < WIDTH; l++) {
                f[0][l] = 0;
            }
            count++;
        }
        flag = 1;
    }
    return count * count * 100;
}

void DrawShadow(int y, int x, int blockID, int blockRotate) {
    int line = y;
    for (;;) {
        if (!CheckToMove(field, blockID, blockRotate, line, x)) {
            break;
        }
        line++;
    }
    line--;
    DrawBlock(line, x, blockID, blockRotate, '/');
}

void DrawRecommend() {
    DrawBlock(recommendY, recommendX, nextBlock[0], recommendR, 'R');
}

void DrawBlockWithFeatures(int y, int x, int blockID, int blockRotate) {
    DrawShadow(y, x, blockID, blockRotate);
    DrawBlock(y, x, blockID, blockRotate, ' ');
    DrawRecommend();
}

void createRankList() {
    FILE *fp;
    int i, j;
    Node *n_node;
    fp = fopen("rank.txt", "r");
    if (fp == NULL) {
        fp = fopen("rank.txt", "w");
        if (fp == NULL) {
            printw("file does not exist\n");
            return;
        }
        fclose(fp);
        fp = fopen("rank.txt", "r");
    }
    int cnt;
    char iname[NAMELEN];

	fscanf(fp, "%d",&cnt);	
	for(int i=0;i<cnt;i++){
		int iscore;
		fscanf(fp, "%s %d",iname,&iscore);
		n_node=(Node*)malloc(sizeof(Node));
		if(n_node==NULL){
			printw("error: memory allocaion failure\n");
		}
		else{
			strcpy(n_node->name,iname);
			n_node->score=iscore;
			n_node->link=NULL;

			if(head==NULL){
				head=n_node;
			}
			else{
				Node*tmp=head;
				while(tmp->link!=NULL){
					tmp=tmp->link;
				}
				tmp->link=n_node;
			}
		}	

	}
	rank_cnt=cnt;
	fclose(fp);
}

void rank(){
	int X=1, Y=rank_cnt, ch;
	clear();
	printw("1 : List ranks from X to Y\n");
	printw("2 : List ranks by a specific name\n");
	printw("3 : Delete a specific rank X\n");
	ch=wgetch(stdscr);
	Node*curr=head;
	
	if (ch == '1') {
		printw("X: \n");
		echo();
		scanw("%d",&X);
		printw("Y: \n");
		scanw("%d",&Y);
		noecho();
		printw("\tname\t|\tscore\t\n");
		printw("-------------------------\n");
		if(X>Y||X>rank_cnt||Y>rank_cnt||X<0||Y<0){
			printw("search failute : no rank in the list\n");
		}
		else{
			for(int i=1;i<X;i++){
				curr=curr->link;
			}
			for(int i=X;i<=Y;i++){
				printw("\t%s\t|\t%d\t\n",curr->name,curr->score);
				curr=curr->link;
			}
		}

	}
	else if ( ch == '2') {
		char str[NAMELEN+1];
		echo();
		printw("Input the name : \n");
		scanw("%s",str);
		noecho();
		printw("\tname\t|\tscore\n--------------------------------\n");
		Node *curr=head;
		int check=0;

		while(curr){
			if(!strcmp(str,curr->name)){
				printw("\t%s\t|\t%d\n", curr->name, curr->score);
				check=1;
			}
			curr=curr->link;
		}
		if(!check){
			printw("search failure: no name in the list\n");
		}
	}

	else if ( ch == '3') {
		echo();
		printw("Input the rank : \n");
		int num;
		scanw("%d", &num);
		noecho();

		Node *curr=head;
		Node *prev=NULL;

		if (num <= 0 || num > rank_cnt) {
            printw("search failure: the rank not in the list\n");
        }
	
		for(int i=1;i<num;++i){
			if(curr==NULL){
				break;
			}
			prev=curr;
			curr=curr->link;
		}

		if (curr!= NULL) {
                if (prev == NULL) { 
                    head = curr->link;
                } else {
                    prev->link = curr->link;
                }
                free(curr);
                printw("result: the rank deleted\n");
                rank_cnt -= 1;
				writeRankFile();
            }
	}
	if(rank_cnt==0){
		head=NULL;
	}
	getch();
	clear();

}

void writeRankFile(){
	FILE *fp = fopen("rank.txt", "w");

	Node*curr=head;

	fprintf(fp,"%d\n",rank_cnt);

	for(int i=0;i<rank_cnt;i++){
		fprintf(fp, "%s %d\n",curr->name, curr->score);
		curr=curr->link;
	}
	fclose(fp);

}

void newRank(int score){
	char str[NAMELEN+1];
	clear();
	echo();
	printw("Your NAME : ");
	getstr(str);

	Node*new=(Node*)malloc(sizeof(Node));
	strcpy(new->name,str);
	new->score=score;
	new->link=NULL;
	
	if(head==NULL) {
		head=new;
	}
	else {
		Node*curr=head;
		Node *prev=NULL;
		while(curr!=NULL&& score<curr->score){
			prev=curr;
			curr=curr->link;
		}
		if(prev==NULL){
			new->link=head;
			head=new;
		}
		else{
			prev->link=new;
			new->link=curr;
		}
	}
	rank_cnt+=1;
	writeRankFile();
}


int recommend(RecNode *root) {
    int i, j, k;
    int max_score = -1;
    int idx = 0;
    int maxRotate = 4; // 기본적으로 회전수는 4로 초기화
    // 각 블록의 회전수를 설정 (정사각형 블록은 1, 직선 블록은 2 등)
    switch (root->curBlockID) {
        case 0: // 직선 블록
            maxRotate = 2;
            break;
        case 4: // 정사각형 블록
            maxRotate = 1;
            break;
        default:
            maxRotate = 4;
            break;
    }

    // 자식 노드 초기화
    if (root->level < VISIBLE_BLOCKS) {
        for (i = 0; i < CHILDREN_MAX; i++) {
            root->c[i] = (RecNode *)malloc(sizeof(RecNode));
            if (root->c[i] == NULL) {
                printf("Memory allocation failed for node %d\n", i);
                exit(1);
            }
            root->c[i]->level = root->level + 1;
            root->c[i]->accumulatedScore = root->accumulatedScore;
        }
    }

    // 각 회전 상태에 대해 가능한 모든 위치에서 점수 계산
    for (i = 0; i < maxRotate; i++) {
        for (j = -1; j < WIDTH-1; j++) {
            //놓을 수 없는 위치는 점수를 -1로 설정한 후 continue
            if (CheckToMove(root->reckField, nextBlock[root->level], i, 0, j)==0) {
                if (idx < CHILDREN_MAX) {
                    root->c[idx]->accumulatedScore = -1;
                    idx++;
                }
                continue;
            }
            //놓을 수 있는 자리는 자식 노드의 필드 상태르 복사한다.
            memcpy(root->c[idx]->reckField, root->reckField, sizeof(field));
            //블록을 가능한 최대 높이로 이동
            for (k = 0; k < HEIGHT; k++) {
                if (CheckToMove(root->c[idx]->reckField, nextBlock[root->level], i, k + 1, j)==0) {
                    break; //더 이상 아래로 이동할 수 없을 때 멈춤
                }
            }
            //자식 노드의 블록 위치 및 회전 설정
            root->c[idx]->recBlockX = j;
            root->c[idx]->recBlockY = k;
            root->c[idx]->recBlockRotate = i;
            //블록을 필드에 추가하고 점수 계산
            root->c[idx]->accumulatedScore += AddBlockToField(root->c[idx]->reckField, root->curBlockID, i, k, j);
            root->c[idx]->accumulatedScore += DeleteLine(root->c[idx]->reckField);
            idx++;//자식 노드 인덱스 증가

            if (idx >= CHILDREN_MAX) {
                break; //최대 자식 노드 수 초과시 반복 중단
            }
        }

        if (idx >= CHILDREN_MAX) {
            break;
        }
    }

    // 재귀적으로 점수 계산
    if (root->level < VISIBLE_BLOCKS - 1) {
        for (i = 0; i < CHILDREN_MAX; i++) {
            if (root->c[i]->accumulatedScore >0) {
                root->c[i]->accumulatedScore += recommend(root->c[i]);
            }
        }
    }

    // 최대 점수 및 위치 갱신
    for (i = 0; i < CHILDREN_MAX; i++) {
        if (root->c[i]->accumulatedScore >= max_score) { //자식 노드의 점수가 최대 점수보다 큰 경우
            if (root->level == 0) {
                if (recommendY < root->c[i]->recBlockY || max_score < root->c[i]->accumulatedScore) {
                    recommendX = root->c[i]->recBlockX;
                    recommendY = root->c[i]->recBlockY;
                    recommendR = root->c[i]->recBlockRotate;
                }
            }
            max_score = root->c[i]->accumulatedScore;
        }
        free(root->c[i]);
    }
    return max_score;
}


int alphaBeta(RecNode *root, int depth, int alpha, int beta, bool maximizingPlayer) {
    int i, j, k; // 반복문 인덱스 변수
    if (depth == 0 || root->level >= VISIBLE_BLOCKS) {
        return root->accumulatedScore; // 최대 깊이 또는 최대 레벨에 도달하면 누적 점수를 반환
    }

    int maxRotate = 4; // 기본적으로 블록의 최대 회전수를 4로 설정
    switch (root->curBlockID) {
        case 0: // 직선 블록
            maxRotate = 2; // 직선 블록은 2번 회전 가능
            break;
        case 4: // 정사각형 블록
            maxRotate = 1; // 정사각형 블록은 회전 불가
            break;
    }

    if (maximizingPlayer) { // 최대화 플레이어 (우리의 경우)
        int maxEval = -INT_MAX; // 초기 최대 평가 값을 -무한대로 설정
        int idx=0;
        for (i = 0; i < maxRotate; i++) { // 가능한 모든 회전 상태에 대해 반복
            for (j = -2; j < WIDTH+2 ; j++) { // 가능한 모든 위치에 대해 반복
                if (!CheckToMove(root->reckField, nextBlock[root->level], i, 0, j)) continue; // 블록을 놓을 수 없는 위치인 경우 건너뜀
                
                RecNode *child = (RecNode *)malloc(sizeof(RecNode));
                space+=sizeof(*child); // 자식 노드 메모리 할당
                root->c[idx]=child;
                child->level = root->level + 1; // 자식 노드의 레벨 설정
                child->accumulatedScore = root->accumulatedScore; // 누적 점수 초기화
                memcpy(child->reckField, root->reckField, sizeof(field)); // 자식 노드의 필드 상태를 복사

                for (k = 0; k < HEIGHT; k++) { // 블록을 가능한 최대 높이로 이동
                    if (!CheckToMove(child->reckField, nextBlock[root->level], i, k + 1, j)) break; // 더 이상 아래로 이동할 수 없으면 멈춤
                }

                child->recBlockX = j; // 자식 노드의 블록 위치 설정
                child->recBlockY = k;
                child->recBlockRotate = i; // 자식 노드의 블록 회전 설정
                child->curBlockID=nextBlock[child->level];
                child->accumulatedScore += AddBlockToField(child->reckField, root->curBlockID, i, k, j); // 블록을 필드에 추가하고 점수 계산
                child->accumulatedScore += DeleteLine(child->reckField); // 줄 삭제 후 점수 추가

                int eval = alphaBeta(child, depth - 1, alpha, beta, false); // 재귀적으로 최소화 플레이어의 점수 계산
                if (eval > maxEval) { // 평가 값이 현재 최대 평가 값보다 큰 경우
                    maxEval = eval; // 최대 평가 값 갱신
                    if (root->level == 0) { // 최상위 레벨에서 추천 좌표를 업데이트
                        recommendX = j;
                        recommendY = k;
                        recommendR = i;
                    }
                }

                alpha = max(alpha, eval); // alpha 값 갱신
                free(child); // 자식 노드 메모리 해제

                if (beta <= alpha) break;
                 // Beta cut-off: 더 이상 탐색할 필요가 없으면 반복 종료
                idx++;
            }
            if (beta <= alpha) break; // Alpha cut-off: 더 이상 탐색할 필요가 없으면 반복 종료
        }
        return maxEval; // 최대 평가 값 반환
    } else { // 최소화 플레이어 (적의 경우)
        int minEval = INT_MAX; // 초기 최소 평가 값을 무한대로 설정
        int idx=0;
        for (i = 0; i < maxRotate; i++) { // 가능한 모든 회전 상태에 대해 반복
            for (j = -2; j < WIDTH+2; j++) { // 가능한 모든 위치에 대해 반복
                if (!CheckToMove(root->reckField, nextBlock[root->level], i, 0, j)) continue; // 블록을 놓을 수 없는 위치인 경우 건너뜀
                
                RecNode *child = (RecNode *)malloc(sizeof(RecNode)); 
                space+=sizeof(*child);// 자식 노드 메모리 할당
                root->c[idx]=child;
                child->level = root->level + 1; // 자식 노드의 레벨 설정
                child->accumulatedScore = root->accumulatedScore; // 누적 점수 초기화
                memcpy(child->reckField, root->reckField, sizeof(field)); // 자식 노드의 필드 상태를 복사

                for (k = 0; k < HEIGHT; k++) { // 블록을 가능한 최대 높이로 이동
                    if (!CheckToMove(child->reckField, nextBlock[root->level], i, k + 1, j)) break; // 더 이상 아래로 이동할 수 없으면 멈춤
                }

                child->recBlockX = j; // 자식 노드의 블록 위치 설정
                child->recBlockY = k;
                child->recBlockRotate = i; // 자식 노드의 블록 회전 설정
                child->curBlockID=nextBlock[child->level];
                child->accumulatedScore += AddBlockToField(child->reckField, root->curBlockID, i, k, j); // 블록을 필드에 추가하고 점수 계산
                child->accumulatedScore += DeleteLine(child->reckField); // 줄 삭제 후 점수 추가

                int eval = alphaBeta(child, depth - 1, alpha, beta, true); // 재귀적으로 최대화 플레이어의 점수 계산
                minEval = min(minEval, eval); // 최소 평가 값 갱신
                free (child);

                beta = min(beta, eval); // beta 값 갱신
                if (beta <= alpha) break; // Alpha cut-off: 더 이상 탐색할 필요가 없으면 반복 종료
                idx++;
            }

            if (beta <= alpha) break; // Beta cut-off: 더 이상 탐색할 필요가 없으면 반복 종료
        }
        return minEval; // 최소 평가 값 반환
    }
}

int modified_recommend(RecNode *root) {
    return alphaBeta(root,VISIBLE_BLOCKS, -INT_MAX, INT_MAX, true); // 깊이 3으로 Alpha-Beta Pruning 수행
}

void recDown(int sig){
	if(!CheckToMove(field, nextBlock[0], blockRotate, blockY + 1, blockX)){
		gameOver=1;
        return;
    }
	DrawField();
	DrawBlockWithFeatures(blockY, blockX, nextBlock[0], blockRotate);
	score+=AddBlockToField(field,nextBlock[0], recommendR, recommendY, recommendX);
	score+=DeleteLine(field);
    PrintScore(score);
	for(int i=0; i<VISIBLE_BLOCKS-1; i++) {
		nextBlock[i]=nextBlock[i+1];
	}
	nextBlock[VISIBLE_BLOCKS-1]=rand()%7;
	DrawNextBlock(nextBlock);

	RecNode root;
	root.level=0;
	root.accumulatedScore=0;
	memcpy(root.reckField, field, sizeof(field));
	modified_recommend(&root);
	blockRotate=0;
	blockY=-1;
	blockX=WIDTH/2-2;
	timed_out=0;
}

void recommendedPlay() {
    int command;
    clear();
	time_t start, stop;
    double duration;
    start=time(NULL);
	act.sa_handler = recDown;
	sigaction(SIGALRM, &act, &oact);
	InitTetris();

	do{
		if(!timed_out){
			alarm(1);
			timed_out = 1;
		}

		command = wgetch(stdscr);
        switch(command){
            case 'q':
                command=QUIT;
                break;
            default:
                command=NOTHING;
                break;
        }
		if(ProcessCommand(command) == QUIT){
			alarm(0);
			DrawBox(HEIGHT / 2 - 1, WIDTH / 2 - 5, 1, 10);
			move(HEIGHT / 2, WIDTH / 2 - 4);
			printw("Good-bye!!");
			refresh();
			getch();
			return;
		}
        stop=time(NULL);
        duration = (double)difftime(stop, start);

        move(3, WIDTH + 22);
        printw("time(t)         : %8.2f  ", duration);
        move(4, WIDTH + 22);
        printw("score(t)/time(t) : %8.2f ", score / duration);
        move(6, WIDTH + 22);
        printw("space(t)         : %8.2f  ", (double)space);
        move(7, WIDTH + 22);
        printw("space(t)/time(t) : %8.2f  ", (double)space / duration);

	}while(!gameOver);

	alarm(0);
    getch();
	DrawBox(HEIGHT / 2 - 1, WIDTH / 2 - 5, 1, 10);
	move(HEIGHT / 2, WIDTH / 2 - 4);
	printw("Good-bye!!");
	refresh();
	getch();
	
	// user code
}