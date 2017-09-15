
#include <curl/curl.h>
#include <pthread.h>

#include "common.h"
#include "frame_tool.h"
#include "ftp_curl_operation.h"

#define BUFF_INCREASE_NUM_BASE		1024

#define BUFF_CMD_CURLFTP_LEN		1024


typedef struct curl_ftp_oper_fd{
	char						*info_buf;
	const char					*ip;
	const char					*user;
	const char					*pass;
	const char					*base_dir;
	const char					*base_curl;
	char						*file_name_buff;
	
	int							info_buf_len;
	int							use_flag;
	int 						time_out;
	CURL_FTP_WORK_MODE			ftp_mode;

	unsigned short				port;

	CURL 						*p_curl;
	SCAN_CURL_FTP_FOLDER_CALLBACK call_function;
	pthread_mutex_t				scan_mutex;
	struct curl_ftp_oper_fd		*next;
	struct curl_ftp_oper_fd		*pre;
}CURL_FTP_OPER_FD;

static CURL_FTP_OPER_FD *head = NULL,*tail = NULL;
static pthread_mutex_t init_fd_mutex;

//////////////////////////////////////////////////////////////////////////////////////
void scan_file_list_under_ftpdir(const char *file_name,int file_size,FILE_NAME_MODE mode)
{
	if(mode == FILE_MODE)
	{
		PERROR("file name :: %s and size :: %d\n",file_name,file_size);
	}else{
		PERROR("folder name :: %s\n",file_name);
	}
}

///-rw-r--r--    1 503      503          3950 Apr 17 12:09 440305-743218887-1480827600-01779-ST_SOURCE_FJ_0001.bcp.ok
const char* find_file_name_point(const char *file_line,int key_byte,int *file_size)
{
	size_t file_size_len = 0;
	char file_size_buff[16] = {0};
	int file_flag = 0,sapce_flag = 0,space_num = 0;
    const char *pp = NULL,*p_size = NULL;
    if(frame_strlen(file_line) <= 49)
        return pp;
	
    for(pp = file_line;*pp > 0;pp++)
    {
        if(file_flag == 0)
        {
            if(*pp == key_byte)
            {
                file_flag++;
                continue;
            }else
                break;
        }
        
        if(*pp == 32)
        {
            if(sapce_flag == 0)
            {
                space_num++;
				if(file_size && space_num == 5)
				{
					file_size_len = pp - p_size;
					if( file_size_len < 16 )
					{
						memcpy(file_size_buff,p_size,file_size_len);
						*file_size = atoi(file_size_buff);
					}
				}
                sapce_flag++;
            }else{
                continue;
            }
        }else{
            if(sapce_flag)
                sapce_flag = 0;
			if(space_num == 4 && p_size == NULL)
			{
				p_size = pp;
			}
            if(space_num == 8)
            {
                return pp;
            }
        }
    }
    return NULL;
}

size_t curl_ftp_scanfile_callback(void *buffer, size_t size, size_t nmemb, void *userp)
{
	size_t file_name_len = 0;
	int file_size = 0;
	const char *p1 = (const char *)buffer,*pp = NULL,*p = NULL;
	CURL_FTP_OPER_FD *curr = (CURL_FTP_OPER_FD*)userp;

	if(curr->file_name_buff == NULL)
	{
		curr->file_name_buff = (char *)malloc(BUFF_INCREASE_NUM_BASE);
		if(curr->file_name_buff == NULL)
		{
			PERROR("There is something wrong with linux system malloc function\n");
			return (size * nmemb);
		}
		memset(curr->file_name_buff,0,BUFF_INCREASE_NUM_BASE);
	}
	
	while(1)
    {
        p = strchr(p1,'\n');
        pp = curr->file_name_buff;
        while(*pp)
            pp++;
        if(p)
        {
            file_name_len = p - p1;
            memcpy((void *)pp,(const void *)p1,file_name_len);
        }else{
            p = p1;
            while(p && *p)
                p++;

            if(p - p1 >= 1)
            {
                file_name_len = p - p1;
                memcpy((void *)pp,(const void *)p1,file_name_len);
            }
            break;
        }
        pp = curr->file_name_buff;
        pp = find_file_name_point(pp,100,&file_size);/////'dddd'
        if(pp && curr->call_function)
        {
            curr->call_function(pp,file_size,FOLDER_MODE);
        }else{
        	pp = curr->file_name_buff;
			pp = find_file_name_point(pp,45,&file_size);/////'-----'
	        if(pp && curr->call_function)
	        {
	            curr->call_function(pp,file_size,FILE_MODE);
	        }
        }
        memset(curr->file_name_buff,0,BUFF_INCREASE_NUM_BASE);
        p1 = p + 1;
    }
	return (size * nmemb);
}


int init_curl_ftp_parame(CURL_FTP_OPER_FD *fop,CURL_FTP_INIT_PARAME *init_parame)
{
	int len = 0;
	char *curr = NULL;
	if(fop == NULL || init_parame == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}

	if(fop->info_buf == NULL)
	{
		fop->info_buf_len = BUFF_INCREASE_NUM_BASE;
		fop->info_buf = (char *)malloc(fop->info_buf_len);
		if(fop->info_buf == NULL)
		{
			PERROR("There is something wrong with linux system malloc function\n");
			return SYSTEM_MALLOC_ERROR;
		}
		memset(fop->info_buf,0,fop->info_buf_len);
	}
	curr = fop->info_buf;
	
	if(init_parame->ip == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	len = frame_strlen(init_parame->ip);
	len++;
	fop->info_buf_len -= len;
	if(fop->info_buf_len < 0)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	memcpy(curr,init_parame->ip,(len - 1));
	fop->ip = curr;
	curr += len;
	
	if(init_parame->user == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	len = frame_strlen(init_parame->user);
	len++;
	fop->info_buf_len -= len;
	if(fop->info_buf_len < 0)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	memcpy(curr,init_parame->user,(len - 1));
	fop->user = curr;
	curr += len;
	
	if(init_parame->pass == NULL)
	{	
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	len = frame_strlen(init_parame->pass);
	len++;
	fop->info_buf_len -= len;
	if(fop->info_buf_len < 0)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	memcpy(curr,init_parame->pass,(len - 1));
	fop->pass = curr;
	curr += len;

	if(init_parame->base_dir== NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	len = frame_strlen(init_parame->base_dir);
	len += 2;
	fop->info_buf_len -= len;
	if(fop->info_buf_len < 0)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	
	memcpy(curr,init_parame->base_dir,(len - 2));
	if(curr[len - 3] == '/')
	{
		curr[len - 2] = 0;
	}else{
		curr[len - 2] = '/';
	}
	if(curr[0] == '/')
		curr++;
	fop->base_dir = curr;
	curr += len;

	fop->time_out = init_parame->timeout;
	fop->ftp_mode = init_parame->mode;
	fop->port = init_parame->port;

	len = snprintf(curr,fop->info_buf_len,"ftp://%s:%s@%s:%d/",fop->user,fop->pass,fop->ip,fop->port);
	if(len >= fop->info_buf_len)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	fop->base_curl= curr;
}

void curl_scan_file_option(CURL_FTP_OPER_FD *curr,const char *url)
{
	CURLcode ret_set;
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_URL, url);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}
	
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_WRITEDATA, (void*)curr);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_WRITEFUNCTION, curl_ftp_scanfile_callback);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTPLISTONLY, 0);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TIMEOUT, curr->time_out);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_CONNECTTIMEOUT, 60);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_NOSIGNAL, 1);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	if(curr->ftp_mode == CURL_FTP_EPRT)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPRT, 1);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}else{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPSV, 1);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}
}

void curl_upload_file_option(CURL_FTP_OPER_FD *curr,const char *url,FILE *local,int local_size,struct curl_slist *header_list)
{
	CURLcode ret_set;
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_URL, url);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	if(header_list)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_POSTQUOTE, header_list);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_READDATA, local);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_INFILESIZE, local_size);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TIMEOUT, curr->time_out);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_CONNECTTIMEOUT, 60);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_NOSIGNAL, 1);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_UPLOAD, 1);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	if(curr->ftp_mode == CURL_FTP_EPRT)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPRT, 1);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}else{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPSV, 1);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}
}

void curl_download_file_option(CURL_FTP_OPER_FD *curr,const char *url,FILE *local)
{
	CURLcode ret_set;
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_URL, url);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_WRITEDATA, local);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TIMEOUT, curr->time_out);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_CONNECTTIMEOUT, 60);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_NOSIGNAL, 1);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}
/*
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TRANSFERTEXT, 1);//ftp transfer using ASCII////CURLOPT_BINARYTRANSFER
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}
*/
	if(curr->ftp_mode == CURL_FTP_EPRT)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPRT, 1);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}else{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPSV, 1);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}
}

void cmd_curl_ftp_option(CURL_FTP_OPER_FD *curr,const char *url,struct curl_slist *header_list)
{
	CURLcode ret_set;
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_URL, url);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_QUOTE, header_list);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TIMEOUT, curr->time_out);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_CONNECTTIMEOUT, 60);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_NOSIGNAL, 1);
	if(ret_set != CURLE_OK)
	{
		PERROR("There is something wrong with curl option set\n");
	}

	if(curr->ftp_mode == CURL_FTP_EPRT)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPRT, 1);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}else{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPSV, 1);
		if(ret_set != CURLE_OK)
		{
			PERROR("There is something wrong with curl option set\n");
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////
int global_curl_ftp_init()
{
	head = NULL;
	tail = NULL;
	pthread_mutex_init(&init_fd_mutex,NULL);
}

int init_curl_ftp_fd(CURL_FTP_INIT_PARAME *init_parame)
{
	int fd = 0,ret = 0;
	CURL_FTP_OPER_FD *curr = NULL;
	
	pthread_mutex_lock(&init_fd_mutex);
	if(head)
	{	
		curr = head;
		do{
			if(curr->use_flag == 0)
				break;
			fd++;
			curr = curr->next;
		}while(curr);
		if(curr)
			curr->use_flag = 1;
	}
	pthread_mutex_unlock(&init_fd_mutex);

	if(curr == NULL)
	{
		curr = (CURL_FTP_OPER_FD *)malloc(sizeof(CURL_FTP_OPER_FD));
		if(curr == NULL)
		{
			PERROR("There is something wrong with linux system malloc function\n");
			return SYSTEM_MALLOC_ERROR;
		}
		memset((void *)curr,0,sizeof(CURL_FTP_OPER_FD));
		curr->use_flag = 1;

		if(head == NULL)
		{
			head = curr;
			tail = curr;
		}else{
			tail->next = curr;
			curr->pre = tail;
			tail = curr;
		}
	}else{
		curr->info_buf_len = BUFF_INCREASE_NUM_BASE;
		memset(curr->info_buf,0,curr->info_buf_len);
		curr->ip = NULL;
		curr->user = NULL;
		curr->pass = NULL;
		curr->base_dir = NULL;
		curr->base_curl = NULL;
	}
	
	ret = init_curl_ftp_parame(curr,init_parame);
	if(ret < 0)
		return ret;
	return fd;
}

int scan_curl_ftp_folder(int fop,const char *re_path,SCAN_CURL_FTP_FOLDER_CALLBACK call_function)
{
	CURLcode ret;
	int i = 0,len = 0,file_size = 0;
	char cmd_buf[BUFF_CMD_CURLFTP_LEN] = {0};
	const char *pp = NULL;
	CURL_FTP_OPER_FD *curr = NULL;
	if(fop < 0 || head == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}
	if(call_function == NULL)
	{
		PERROR("There is no parameter of callback and set default callback function\n");
		call_function = scan_file_list_under_ftpdir;
	}
	curr = head;
	for(i = 0;i < fop;i++)
	{
		curr = curr->next;
		if(curr == NULL)
		{
			PERROR("There is something wrong with teansmit paramter error\n");
			return TRANSMIT_PARAMETER_ERROR;
		}
	}
	pthread_mutex_lock(&curr->scan_mutex);
	curr->call_function = call_function;
	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
	{
		PERROR("There is something worong with system call function\n");
		pthread_mutex_unlock(&curr->scan_mutex);
		return SYSTEM_FUNCTION_CALL_ERROR;
	}
	
	if(re_path)
	{
		len = frame_strlen(re_path);
		len--;
		if(re_path[len] == '/')
			sprintf(cmd_buf,"%s%s%s",curr->base_curl,curr->base_dir,re_path);
		else
			sprintf(cmd_buf,"%s%s%s/",curr->base_curl,curr->base_dir,re_path);
	}else{
		sprintf(cmd_buf,"%s%s",curr->base_curl,curr->base_dir);
	}

	curl_scan_file_option(curr,cmd_buf);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		PERROR("There is something with curl ftp operation :: %d!errno:%d,err:%s\n",ret,errno,strerror(errno));
	}
	
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;

	if(curr->file_name_buff)
	{
		pp = curr->file_name_buff;
	    pp = find_file_name_point(pp,100,&file_size);/////'ddddddd'
	    if(pp)
	    {
	    	if(curr->call_function)
	        	curr->call_function(pp,file_size,FOLDER_MODE);
	    }else{
	    	pp = curr->file_name_buff;
	    	pp = find_file_name_point(pp,45,&file_size);/////'---'
	    	if(pp && curr->call_function)
	    	{
	    		curr->call_function(pp,file_size,FILE_MODE);
	    	}
	    }
	    memset(curr->file_name_buff,0,BUFF_INCREASE_NUM_BASE);
	}
	pthread_mutex_unlock(&curr->scan_mutex);
	return (int)ret;
}

int upload_curl_ftp_file(int fop,const char *up_file,const char *re_path,const char *to_name)
{
	CURLcode ret;
	char cmd_buf[BUFF_CMD_CURLFTP_LEN] = {0};
	const char *up_name = NULL;
	int i = 0,re_name = 0;
	FILE *up_fp = NULL;
	CURL_FTP_OPER_FD *curr = NULL;
	struct curl_slist *header_list = NULL;
	
	if( up_file == NULL || fop < 0 || head == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}

	curr = head;
	for(i = 0;i < fop;i++)
	{
		curr = curr->next;
		if(curr == NULL)
		{
			PERROR("There is something wrong with teansmit paramter error\n");
			return TRANSMIT_PARAMETER_ERROR;
		}
	}

	if(to_name == NULL)
	{
		to_name = strrchr(up_file,'/');
		if(to_name == NULL)
			to_name = up_file;
	}else{
		re_name = 1;
	}
	up_name = strrchr(up_file,'/');
	if(up_name == NULL)
		up_name = up_file;

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
	{
		PERROR("There is something worong with system call function\n");
		return SYSTEM_FUNCTION_CALL_ERROR;
	}

	up_fp = fopen(up_file, "rb");
	if(up_fp == NULL)
	{
		PERROR("There is something worong with system call function\n");
		return SYSTEM_FUNCTION_CALL_ERROR;
	}

	if(re_name)
	{
		sprintf(cmd_buf,"RNFR %s",up_name);
		header_list = curl_slist_append(header_list, cmd_buf);
		memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
		sprintf(cmd_buf,"RNTO %s",to_name);
		header_list = curl_slist_append(header_list, cmd_buf);
	}

	memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
	if(re_path)
	{
		sprintf(cmd_buf,"%s%s%s/%s",curr->base_curl,curr->base_dir,re_path,up_name);
	}else{
		sprintf(cmd_buf,"%s%s%s",curr->base_curl,curr->base_dir,up_name);
	}

	curl_upload_file_option(curr,cmd_buf,up_fp,get_local_file_size(up_file),header_list);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		PERROR("There is something with curl ftp operation :: %d!errno:%d,err:%s\n",ret,errno,strerror(errno));
	}
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;
	fclose(up_fp);
	return (int)ret;
}

int download_curl_ftp_file(int fop,const char *down_file,const char *re_path,const char *to_patn_name)
{
	CURLcode ret;
	char cmd_buf[BUFF_CMD_CURLFTP_LEN] = {0};
	FILE *down_fp = NULL;
	int i = 0;
	CURL_FTP_OPER_FD *curr = NULL;

	if(fop < 0 || down_file == NULL || to_patn_name == NULL || head == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}

	curr = head;
	for(i = 0;i < fop;i++)
	{
		curr = curr->next;
		if(curr == NULL)
		{
			PERROR("There is something wrong with teansmit paramter error\n");
			return TRANSMIT_PARAMETER_ERROR;
		}
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
	{
		PERROR("There is something worong with system call function\n");
		return SYSTEM_FUNCTION_CALL_ERROR;
	}

	down_fp = fopen(to_patn_name, "wab");
	if(down_fp == NULL)
	{
		PERROR("There is something worong with system call function\n");
		return SYSTEM_FUNCTION_CALL_ERROR;
	}

	if(re_path)
	{
		sprintf(cmd_buf,"%s%s%s/%s",curr->base_curl,curr->base_dir,re_path,down_file);
	}else{
		sprintf(cmd_buf,"%s%s%s",curr->base_curl,curr->base_dir,down_file);
	}

	curl_download_file_option(curr,cmd_buf,down_fp);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		PERROR("There is something with curl ftp operation :: %d!errno:%d,err:%s\n",ret,errno,strerror(errno));
	}
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;
	fclose(down_fp);
	return (int)ret;
}

int rename_curl_ftp_file(int fop,const char *origin_name,const char *re_path,const char *to_name)
{
	CURLcode ret;
	char cmd_buf[BUFF_CMD_CURLFTP_LEN] = {0};
	int i = 0;
	CURL_FTP_OPER_FD *curr = NULL;
	struct curl_slist *header_list = NULL;
	
	if(origin_name == NULL || to_name == NULL || fop < 0 || head == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}

	curr = head;
	for(i = 0;i < fop;i++)
	{
		curr = curr->next;
		if(curr == NULL)
		{
			PERROR("There is something wrong with teansmit paramter error\n");
			return TRANSMIT_PARAMETER_ERROR;
		}
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
	{
		PERROR("There is something worong with system call function\n");
		return SYSTEM_FUNCTION_CALL_ERROR;
	}

	sprintf(cmd_buf,"RNFR %s",origin_name);
	header_list = curl_slist_append(header_list, cmd_buf);
	memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
	sprintf(cmd_buf,"RNTO %s",to_name);
	header_list = curl_slist_append(header_list, cmd_buf);

	memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
	if(re_path)
	{
		sprintf(cmd_buf,"%s%s%s/%s",curr->base_curl,curr->base_dir,re_path,origin_name);
	}else{
		sprintf(cmd_buf,"%s%s%s",curr->base_curl,curr->base_dir,origin_name);
	}

	cmd_curl_ftp_option(curr,cmd_buf,header_list);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		PERROR("There is something with curl ftp operation :: %d!errno:%d,err:%s\n",ret,errno,strerror(errno));
	}
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;
	return (int)ret;
}

int delete_curl_ftp_file(int fop,const char *re_path,const char *file_name)
{
	CURLcode ret;
	char cmd_buf[BUFF_CMD_CURLFTP_LEN] = {0};
	int i = 0;
	CURL_FTP_OPER_FD *curr = NULL;
	struct curl_slist *header_list = NULL;
	
	if(file_name == NULL || fop < 0 || head == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}

	curr = head;
	for(i = 0;i < fop;i++)
	{
		curr = curr->next;
		if(curr == NULL)
		{
			PERROR("There is something wrong with teansmit paramter error\n");
			return TRANSMIT_PARAMETER_ERROR;
		}
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
	{
		PERROR("There is something worong with system call function\n");
		return SYSTEM_FUNCTION_CALL_ERROR;
	}

	sprintf(cmd_buf,"DELE %s",file_name);
	header_list = curl_slist_append(header_list, cmd_buf);

	memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
	if(re_path)
	{
		sprintf(cmd_buf,"%s%s%s/%s",curr->base_curl,curr->base_dir,re_path,file_name);
	}else{
		sprintf(cmd_buf,"%s%s%s",curr->base_curl,curr->base_dir,file_name);
	}

	cmd_curl_ftp_option(curr,cmd_buf,header_list);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		PERROR("There is something with curl ftp operation :: %d!errno:%d,err:%s\n",ret,errno,strerror(errno));
	}
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;
	return (int)ret;
}

int delete_curl_ftp_dir(int fop,const char *re_path)
{
	CURLcode ret;
	char cmd_buf[BUFF_CMD_CURLFTP_LEN] = {0};
	int i = 0;
	CURL_FTP_OPER_FD *curr = NULL;
	struct curl_slist *header_list = NULL;
	
	if(re_path == NULL || fop < 0 || head == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}

	curr = head;
	for(i = 0;i < fop;i++)
	{
		curr = curr->next;
		if(curr == NULL)
		{
			PERROR("There is something wrong with teansmit paramter error\n");
			return TRANSMIT_PARAMETER_ERROR;
		}
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
	{
		PERROR("There is something worong with system call function\n");
		return SYSTEM_FUNCTION_CALL_ERROR;
	}

	sprintf(cmd_buf,"RMD %s",re_path);
	header_list = curl_slist_append(header_list, cmd_buf);

	memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
	sprintf(cmd_buf,"%s%s",curr->base_curl,curr->base_dir);

	cmd_curl_ftp_option(curr,cmd_buf,header_list);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		PERROR("There is something with curl ftp operation :: %d!errno:%d,err:%s\n",ret,errno,strerror(errno));
	}
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;
	return (int)ret;
}

int create_curl_ftp_dir(int fop,const char *re_path)
{
	CURLcode ret;
	char cmd_buf[BUFF_CMD_CURLFTP_LEN] = {0};
	int i = 0;
	CURL_FTP_OPER_FD *curr = NULL;
	struct curl_slist *header_list = NULL;
	
	if(re_path == NULL || fop < 0 || head == NULL)
	{
		PERROR("There is something wrong with teansmit paramter error\n");
		return TRANSMIT_PARAMETER_ERROR;
	}

	curr = head;
	for(i = 0;i < fop;i++)
	{
		curr = curr->next;
		if(curr == NULL)
		{
			PERROR("There is something wrong with teansmit paramter error\n");
			return TRANSMIT_PARAMETER_ERROR;
		}
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
	{
		PERROR("There is something worong with system call function\n");
		return SYSTEM_FUNCTION_CALL_ERROR;
	}

	sprintf(cmd_buf,"MKD %s",re_path);
	header_list = curl_slist_append(header_list, cmd_buf);

	memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
	sprintf(cmd_buf,"%s%s",curr->base_curl,curr->base_dir);

	cmd_curl_ftp_option(curr,cmd_buf,header_list);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		PERROR("There is something with curl ftp operation :: %d!errno:%d,err:%s\n",ret,errno,strerror(errno));
	}
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;
	return (int)ret;
}

void reset_curl_ftp_fd(int fop)
{
	int i = 0;
	CURL_FTP_OPER_FD *curr = NULL;
	if(fop < 0 || head == NULL)
		return;
	curr = head;
	for(i = 0;i < fop;i++)
	{
		curr = curr->next;
		if(curr == NULL)
		{
			PERROR("There is something wrong with teansmit paramter error\n");
			return;
		}
	}

	curr->use_flag = 0;
}

void global_curl_ftp_cleanup()
{
	CURL_FTP_OPER_FD *curr = NULL;
	pthread_mutex_lock(&init_fd_mutex);
	if(head)
	{
		do
		{
			if(curr == NULL)
			{
				curr = tail;
			}
			
			if(curr->info_buf)
			{
				free(curr->info_buf);
				curr->info_buf = NULL;
			}

			if(curr->file_name_buff)
			{
				free(curr->file_name_buff);
				curr->file_name_buff = NULL;
			}

			if(curr->pre)
			{
				curr = curr->pre;
				free(curr->next);
				curr->next = NULL;
			}else{
				free(curr);
				curr = NULL;
			}
		}while(curr);
	}

	head = tail = NULL;
	pthread_mutex_unlock(&init_fd_mutex);
}

//////////////////////////////////////////////////////////////////////////////////
void test_curl_ftp_function()
{
	CURL_FTP_INIT_PARAME init_parame = {0};
	int fop_22 = 0,fop_219 = 0;
	unsigned short port = 21;
	const char ip_22[] = "192.168.8.22",ip_219[] = "192.168.8.219",user_[] = "appftp",pass_[] = "123",
		base_dir_22[] = "/usr/seentech/app/22",base_dir_219[] = "/usr/seentech/app/219";

	global_curl_ftp_init();

	init_parame.ip = ip_22;
	init_parame.base_dir = base_dir_22;
	init_parame.user = user_;
	init_parame.pass = pass_;
	init_parame.port = port;
	init_parame.mode = CURL_FTP_EPSV;
	init_parame.timeout = 300;
	fop_22 = init_curl_ftp_fd(&init_parame);
	scan_curl_ftp_folder(fop_22,NULL,NULL);
	upload_curl_ftp_file(fop_22,"Makefile",NULL,NULL);

	init_parame.ip = ip_219;
	init_parame.base_dir = base_dir_219;
	fop_219 = init_curl_ftp_fd(&init_parame);
	scan_curl_ftp_folder(fop_219,NULL,NULL);
	upload_curl_ftp_file(fop_219,"Makefile",NULL,NULL);
}

