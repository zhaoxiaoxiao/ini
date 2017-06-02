
#include <time.h>
#include <sys/time.h>

#include "common.h"
#include "lottery.h"


#define RED_BALL_QUAN			6
#define BLUE_BALL_QUAN			1
#define RED_MAX_BALL_NUN		33
#define BLUE_MAX_BALL_NUM		16

#define WIN_BALL_NUM			7

static int red_ball_num[RED_MAX_BALL_NUN] = {0},
	blue_ball_num[BLUE_MAX_BALL_NUM] = {0};

static int win_num[WIN_BALL_NUM] = {0};
void reset_red_ball()
{
	int i = 0;
	for(i = 1;i <= RED_MAX_BALL_NUN;i++)
	{
		red_ball_num[i] = i;
	}
}

void reset_blue_ball()
{
	int i = 0;
	for(i = 1;i <= BLUE_MAX_BALL_NUM;i++)
	{
		blue_ball_num[i] = i;
	}
}

int random_number(int mod)
{
	srand((unsigned)time(NULL));
	return rand()%mod;
}

void bubble_sort(int *array,int num)
{
	int i = 0,j = 0,tmp = 0;

	for(i = 0;i < num;i++)
	{
		for(j = 0;j < num - i - 1;j++)
		{
			if(array[j] > array[j + 1])
			{
				tmp = array[j];
				array[j] = array[j + 1];
				array[j + 1] = tmp;
			}
		}
	}
}

void lottery_num(int note)
{
	int j = 0,i = 0,sub = 0,tmp = 0;
	for(j = 0;j < note;j++)
	{
		reset_red_ball();
		reset_blue_ball();
		//PERROR("result:: ");
		for(i = 0;i < RED_BALL_QUAN;i++)
		{
			sub = random_number(RED_MAX_BALL_NUN - i);
			//printf("  %d ",red_ball_num[sub]);
			win_num[i] = red_ball_num[sub];
			tmp = red_ball_num[(RED_MAX_BALL_NUN -i -1)];
			red_ball_num[(RED_MAX_BALL_NUN -i -1)] = red_ball_num[sub];
			red_ball_num[sub] = tmp;
		}

		sub = random_number(BLUE_MAX_BALL_NUM);
		//printf("-- %d\n",blue_ball_num[sub]);
		win_num[RED_BALL_QUAN] = blue_ball_num[sub];

		bubble_sort(win_num,RED_BALL_QUAN);
		PERROR("result:: ");
		for(i = 0;i < WIN_BALL_NUM;i++)
		{
			printf("  %d ",win_num[i]);
		}
		printf("\n");
	}
	
}

