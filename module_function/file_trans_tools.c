
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>



#if 1
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)



FILE *read_fp = NULL,*write_fp = NULL;

///////////////////////////////////////////////////////////////////////////////////////////
char* framestr_frist_constchar(char *str,const char c)
{
	char *p = str;
	if(!str)
		return NULL;
	while(*p)
	{
		if(*p == c)
			return p;
		else
			p++;
	}
	return NULL;
}

int frame_strlen(const char *str)
{
	int len = 0;
	if(!str)
		return len;
	while(*str)
	{
		str++;
		len++;
	}
	return len;
}

void clean_before_exit()
{
	if(read_fp)
		fclose(read_fp);
	
	if(write_fp)
		fclose(write_fp);

	read_fp = NULL;
	write_fp = NULL;
}

void make_sql_sentence(char *mac,char *com,char *sql)
{
	//sprintf(sql,"REPLACE INTO elefence_terminal_company (`machdr`,`company_en`,`company_cn`) VALUES(\"%s\",\"%s\",\"%s\");\n",mac,com,com);
	sprintf(sql,"INSERT INTO elefence_terminal_company (`machdr`,`company_en`,`company_cn`) VALUES(\"%s\",\"%s\",\"%s\") \
ON DUPLICATE KEY UPDATE `company_en` = \"%s\";\n",mac,com,com,com);
	//sprintf(sql,"(\"%s\",\"%s\",\"%s\"),",mac,com,com);
	PDEBUG("sql senten::%s\n",sql);
}

int read_mac_file(char *mac,char *com)
{
	int ret = 0,i = 0,len = 0;
	char *pData = NULL,*seq = NULL,*p_char = NULL,*q_char = NULL;
	size_t nLen = 0;
	ssize_t nRead = 0;

	do{
		nRead = getline(&pData, &nLen, read_fp);
		PDEBUG("read file line::%s\n",pData);
		if(nRead < 6)
		{
			ret = -1;
			break;
		}
		seq = framestr_frist_constchar(pData,',');
		if(seq == NULL)
			continue;

		if(seq - pData != 6)
		{
			continue;
		}

		p_char = pData;
		q_char = mac;
		for(i = 0;i < 6;i ++)
		{
			*q_char = *p_char;
			q_char++;
			p_char++;
			if(i > 0 && i%2 == 1 && i < 5)
			{
				*q_char = '-';
				q_char++;
			}
		}

		seq++;
		len = frame_strlen(seq);
		len--;
		memcpy(com,seq,len);
		break;
	}while(1);

	if(pData)
		free(pData);
	return ret;
}

void trans_mac_sql_file()
{
	int ret = 0,len = 0;;
	char mac[32] = {0},comp[1024] = {0},sql_sen[2048] = {0};

	do{
		ret = read_mac_file(mac,comp);
		if(ret < 0)
			break;
		make_sql_sentence(mac,comp,sql_sen);
		len = frame_strlen(sql_sen);
		fwrite(sql_sen,1,len,write_fp);
		memset(mac,0,32);
		memset(comp,0,1024);
		memset(sql_sen,0,2048);
	}while(1);
	
}

int init_open_file()
{
	read_fp = fopen("mac.txt", "r");
	if(read_fp == NULL)
	{
		PERROR("There is something wrong about open read file\n");
		return -1;
	}
	
	write_fp = fopen("term_comp.sql", "w");
	if(read_fp == NULL)
	{
		PERROR("There is something wrong about open write file\n");
		return -1;
	}
}

void sig_catch(int sig)
{
    PERROR("!!!!!!!!well, we catch signal in this process :::%d\n\n",sig);
    switch(sig)
    {
        case SIGINT:
            break;
        case SIGSEGV:
            break;
        case SIGTERM:
            break;
    	default:
			break;
    }
	clean_before_exit();
    exit(0);
}

int main(int argc,char* argv[])
{
	(void)signal(SIGINT, sig_catch);//ctrl + c 
	(void)signal(SIGSEGV, sig_catch);//memory
	(void)signal(SIGTERM, sig_catch);//kill
	init_open_file();
	trans_mac_sql_file();
	clean_before_exit();
	return 0;
}
