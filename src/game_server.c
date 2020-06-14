#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define OPSZ 4
#define WORD_STATIS 3

/* 통계 구조체 자료형 정의 */
typedef struct statis {
	char alphabet;
	float prob;
	float succ;
	float total;
	int check;
} statis_t;

void error_handling(char *message);
void init(int* count, char* game, char* word, char* src);
int hangman(int* count, char* game, char word[], char user, statis_t statis[][26], int word_c); /* 게임 진행 */
void generator(char* game, char word[]); /* 단어 캡슐화 (게임 초기) */
void ranking(float* data, char* str, int n);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	char opinfo[BUF_SIZE], message[BUF_SIZE];
	int i, j, user_connect, recv_len;

	/* Game 변수 */
	int end, word_c, count = 0;
	char word[BUF_SIZE], game[BUF_SIZE], src[20];

	FILE *fp = NULL;
	statis_t statis[WORD_STATIS][26]; // 통계를 위한 구조체

	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

	// sort를 위한 배열
	float statis_prob[26];
	char statis_alphabet[26];
	float statis_succ[5]; // 오름차순
	float statis_total[5]; // 오름차순

	if(argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error");
	clnt_adr_sz = sizeof(clnt_adr);

	for(user_connect = 0; user_connect < 5; user_connect++)
	{
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

		if(clnt_sock == -1)
			error_handling("accept() error");
		else
			printf("Connected client %d \n", user_connect+1);

		// 통계 구조체 init
		for (i = 0; i < WORD_STATIS; i++)
		{
			/*
			 * 0: 3글자 단어 통계
			 * 1: 4글자 단어 통계
			 * 2: 5글자 단어 통계
			 */
			for(j = 0; j < 26; j++)
			{
				statis[i][j].alphabet = 97 + j; // 알파벳 아스키코드 값으로 넣음
				statis[i][j].check = 0;
				statis[i][j].prob = 0.;
				statis[i][j].succ = 0.;
				statis[i][j].total = 0.;
			}
		}

		while ((recv_len = read(clnt_sock, opinfo, BUF_SIZE)) != 0)
		{
			switch (opinfo[0]) {
				case 1:
					/*
					 * Game Init
					 * 게임 변수 초기화
					 */
					printf("[Server] 게임 시작!\n");
					init(&count, game, word, src);
					word_c = opinfo[1];

					srand(time(NULL));
					sprintf(src, "word/%d.txt", opinfo[1]);
					fp = fopen(src, "r");

					if (fp != NULL)
					{
						char line[255];
						char *pStr;
						int i = 1, key = 0;

						pStr = fgets(line, sizeof(line), fp);
						key = rand() % atoi(pStr);

						while(!feof(fp))
				        {
							if (i == key)
								strcpy(word, pStr);
							i++;
							pStr = fgets(line, sizeof(line), fp);
				        }
				        fclose(fp);
					}

					generator(game, word);
					printf("[Server] 단어 생성: %s", word);
					write(clnt_sock, word, strlen(word)+1);

					break;
				case 2:
					printf("[Client] : %c\n", opinfo[1]);

					end = hangman(&count, game, word, opinfo[1], statis, word_c);
					// 1: Success 2: 게임 끝 0: 게임 진행
					switch (end) {
						case 1:
						case 2:
							for(i = 0; i < 26; i++)
								statis[word_c-3][i].check = 0; // Check 리셋
							break;
					}
					sprintf(message, "%d %d %s", end, count, game);
					write(clnt_sock, message, strlen(message));

					message[0] = 0; // Array Clear
					break;
				case 3:
					printf("[Client] 확인하고 싶은 단어의 글자 수 : %d\n", opinfo[1]);

					// 초기 값
					for(i = 0; i < 26; i++)
					{
						statis_prob[i] = statis[opinfo[1]-3][i].prob;
						statis_alphabet[i] = statis[opinfo[1]-3][i].alphabet;
					}

					// 확률 Top 3 - Insertion sort
					ranking(statis_prob, statis_alphabet, 26);

					for (i = 0; i < 26; i++)
					{
						if (statis[opinfo[1]-3][i].alphabet == statis_alphabet[25])
						{
							statis_succ[4] = statis[opinfo[1]-3][i].succ;
							statis_total[4] = statis[opinfo[1]-3][i].total;
						}
						else if (statis[opinfo[1]-3][i].alphabet == statis_alphabet[24])
						{
							statis_succ[3] = statis[opinfo[1]-3][i].succ;
							statis_total[3] = statis[opinfo[1]-3][i].total;
						}
						else if (statis[opinfo[1]-3][i].alphabet == statis_alphabet[23])
						{
							statis_succ[2] = statis[opinfo[1]-3][i].succ;
							statis_total[2] = statis[opinfo[1]-3][i].total;
						}
						else if (statis[opinfo[1]-3][i].alphabet == statis_alphabet[22])
						{
							statis_succ[1] = statis[opinfo[1]-3][i].succ;
							statis_total[1] = statis[opinfo[1]-3][i].total;
						}
						else if (statis[opinfo[1]-3][i].alphabet == statis_alphabet[21])
						{
							statis_succ[0] = statis[opinfo[1]-3][i].succ;
							statis_total[0] = statis[opinfo[1]-3][i].total;
						}
					}
					sprintf(message, "1. < %c >\n   확률: %f (맞춘 수: %d 시도: %d)\n2. < %c >\n   확률: %f (맞춘 수: %d 시도: %d)\n3. < %c >\n   확률: %f (맞춘 수: %d 시도: %d)\n4. < %c >\n   확률: %f (맞춘 수: %d 시도: %d)\n5. < %c >\n   확률: %f (맞춘 수: %d 시도: %d)\n",
						statis_alphabet[25], statis_prob[25], (int)statis_succ[4], (int)statis_total[4],
						statis_alphabet[24], statis_prob[24], (int)statis_succ[3], (int)statis_total[3],
						statis_alphabet[23], statis_prob[23], (int)statis_succ[2], (int)statis_total[2],
						statis_alphabet[22], statis_prob[22], (int)statis_succ[1], (int)statis_total[1],
						statis_alphabet[21], statis_prob[21], (int)statis_succ[0], (int)statis_total[0]);

					write(clnt_sock, message, strlen(message)+1);
					message[0] = 0; // Array Clear
					break;
			}
		}
		close(clnt_sock);
	}
	close(serv_sock);
	return 0;
}

void init(int* count, char* game, char* word, char* src)
{
	/*
	 * init
	 * 문자열 배열 초기화
	 * Game Count 초기화
	 */

	*count = 0; // count reset
	game[0] = 0; // Array Clear
	word[0] = 0; // Array Clear
	word[0] = 0; // Array Clear
}

void generator(char* game, char word[])
{
	/*
	 * init
	 * 단어 캡슐화
	 */

	strcpy(game, word); // word 문자열 만큼 game에 복사 (크기 할당)

	for (int i = 0; i < strlen(word)-1; i++)
		game[i] = '*';
}

void ranking(float* data, char* str, int n)
{
	/*
	 * Insertion Sort 방식
	 * 오름차순 정렬
	 */

	int i, j;
	char alphabet;
	float prob;

	for (i = 1; i < n; i++)
	{
		prob = data[i];
		alphabet = str[i];
		for (j = i-1; j >= 0 && prob < data[j]; j--)
		{
			data[j+1] = data[j];
			str[j+1] = str[j];
		}
		data[j+1] = prob;
		str[j+1] = alphabet;
	}
}

int hangman(int* count, char* game, char word[], char user, statis_t statis[][26], int word_c)
{
	/*
	 * hangman
	 * 입력된 문자들이 단어와 일치하는 check 1: 같음 / 0: 다름
	 * 성공 시 카운트를 내리지 않음
	 */

	int check = 0, conv;
	conv = user;

	for (int i = 0; i < strlen(word)-1; i++)
	{
		if (word[i] == user)
		{
			game[i] = user;
			check = 1;
		}
	}

	if (!check) // 카운트
		*count += 1;

	if (*count == 10) // 10번 카운트 시 게임 종료
		return 2;
	else
	{
		// 통계 계산
		if (!statis[word_c-3][conv-97].check) // check가 1이라면 이전에 계산 되었음.
		{
			if(check) // 사용자가 단어를 맞췄을 경우. succ + 1
			{
				statis[word_c-3][conv-97].check = 1;
				statis[word_c-3][conv-97].succ += 1.;
				statis[word_c-3][conv-97].total += 1.;
				statis[word_c-3][conv-97].prob = statis[word_c-3][conv-97].succ / statis[word_c-3][conv-97].total;
			}
			else
			{
				statis[word_c-3][conv-97].check = 1;
				statis[word_c-3][conv-97].total += 1.;
				statis[word_c-3][conv-97].prob = statis[word_c-3][conv-97].succ / statis[word_c-3][conv-97].total;
			}
		}
		return !strcmp(game, word); // 비교 후 리턴
	}
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
