#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define RLT_SIZE 4
#define OPSZ 4

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sock;
	char opmsg[BUF_SIZE], message[BUF_SIZE];
	int i;
	struct sockaddr_in serv_adr;
	FILE *fp = NULL;

	/* Game 변수 */
	int mode = 1, count = 1;
	char solution[BUF_SIZE], src[20];

	if(argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("connect() error!");
	else
		puts("Connected...........");

	while (1) {
		/*
		 * Game mode
		 * 0. Quit 1. Game Start 2. History
		 */

		int game, user; // scanf 변수

		printf("==============================================\n");
 		printf(" WELCOME TO HANGMAN GAME VER.0.1!\n");
		printf("----------------------------------------------\n");
		fputs("0. Quit\n1. Game Start\n2. Probability Top 5\n", stdout);
		printf("==============================================\n");
		printf("Input : ");
		scanf("%d", &game);

		if (game == 0)
		{
			printf("Good Bye!! / From. Jaehee\n");
			break;
		}

		switch (game) {
			case 1:
				/*
				 * Game Start
				 * init
				 */
				mode = 1;
				opmsg[0] = (char)mode;

				printf("==============================================\n");
				fputs("[init] 글자 수를 입력하세요. (3-5)\n", stdout);
				printf("Input : ");
				scanf("%d", (int*)&opmsg[1]);
				printf("==============================================\n");
				write(sock, opmsg, OPSZ+1);

				memset(&opmsg, 0, sizeof(opmsg));
				read(sock, message, BUF_SIZE);

				solution[0] = 0; // Array Clear
				strcpy(solution, message); // 정답 저장

				printf("[Game] 단어가 생성되었습니다.\n");
				printf("----------------------------------------------\n");
				printf("0. 게임 플레이\n1. 정답공개 후 플레이\n");
				printf("==============================================\n");
				printf("Input : ");
				scanf("%d", &user);

				if (user)
					printf("Solution: %s", message);

				message[0] = 0; // Array Clear
				mode = 2; // Next Step

				while(1)
				{
					/* Game */
					opmsg[0] = (char)mode;
					char *token = NULL;

					fgetc(stdin); // 입력 버퍼 내용 제거
					fputs("\n\n\n[Game] 글자(영어)를 입력하세요.\n", stdout);
					printf("==============================================\n");
					printf("Input : ");
					scanf("%c", (char*)&opmsg[1]);
					write(sock, opmsg, OPSZ+1);
					read(sock, message, BUF_SIZE);

					// Step 1 Token: 1: Success 2: 게임 끝 0: 게임 진행
					token = strtok(message, " ");

					if (atoi(token) == 1) // 성공
					{
						printf("==============================================\n");
						printf("::: Congratulations!! :::\nAnswer : %s\n\n", solution);
						break;
					}
					else if (atoi(token) == 2) // Game over
					{
						fp = fopen("animation/10.txt", "r");

						if (fp != NULL)
						{
							char line[255];
							char *pStr;

							pStr = fgets(line, sizeof(line), fp);

							while(!feof(fp))
					        {
								printf("%s", pStr);
					            pStr = fgets(line, sizeof(line), fp);
					        }
					        fclose(fp);
						}
						printf("==============================================\n");
						printf("::: Game Over!! :::\nAnswer : %s\n\n", solution);
						break;
					}
					// Step 2 Token: count 값
					token = strtok(NULL, " ");
					count = atoi(token);
					// Step 3 Token: 게임 메세지
					token = strtok(NULL, " ");

					sprintf(src, "animation/%d.txt", count);
					fp = fopen(src, "r");

					if (fp != NULL)
					{
						char line[255];
						char *pStr;

						pStr = fgets(line, sizeof(line), fp);

						while(!feof(fp))
				        {
							printf("%s", pStr);
				            pStr = fgets(line, sizeof(line), fp);
				        }
				        fclose(fp);
					}

					printf("----------------------------------------------\n");
					printf("단어: %s", token);
					printf("----------------------------------------------\n");
					printf("남은 수: (%d)\n", 10-count);
					printf("==============================================\n");

					message[0] = 0; // Array Clear
				}
				break;
			case 2:
				/*
				 * Game Statistics
				 * 확률 Top 5
				*/
				mode = 3;
				opmsg[0] = (char)mode;

				fgetc(stdin); // 입력 버퍼 내용 제거
				fputs("\n\n\n[Statis] 글자 수를 입력하세요. (3-5)\n", stdout);
				printf("----------------------------------------------\n");
				printf("* 이전 게임기록으로\n");
				printf("  각 글자의 확률을 계산해\n");
				printf("  <Top 5>로 보여줍니다.\n");
				printf("==============================================\n");
				printf("Input : ");
				scanf("%d", (int*)&opmsg[1]);
				write(sock, opmsg, OPSZ+1);

				read(sock, message, BUF_SIZE);
				printf("----------------------------------------------\n");
				printf("%s\n", message);

				message[0] = 0; // Array Clear

				break;
		}
	}
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
