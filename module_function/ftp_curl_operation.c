
#include <curl/curl.h>

#include "common.h"
#include "frame_tool.h"
#include "ftp_curl_operation.h"

#define BUFF_INCREASE_NUM_BASE		1024

#define BUFF_CMD_CURLFTP_LEN		1024

#define MAX_LOCAL_FILE_SIZE			100*1024*1024

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
///-rw-r--r--    1 503      503          3950 Apr 17 12:09 440305-743218887-1480827600-01779-ST_SOURCE_FJ_0001.bcp.ok
const char* find_file_name_point(const char *file_line,int key_byte,int *file_size)
{
	size_t file_size_len = 0;
	char file_size_buff[16] = {0};
	int file_flag = 0,sapce_flag = 0,space_num = 0;
    const char *pp = NULL,*p_size = NULL;
    if(strlen(file_line) <= 49)
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
			if(space_num == 4)
				p_size = pp;
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
	const char *p1 = buffer,*pp = NULL,*p = NULL;
	CURL_FTP_OPER_FD *curr = (CURL_FTP_OPER_FD*)userp;

	if(curr->file_name_buff == NULL)
	{
		curr->file_name_buff = (char *)malloc(BUFF_INCREASE_NUM_BASE);
		if(curr->file_name_buff == NULL)
		{
			;
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
        }
		pp = find_file_name_point(pp,45,file_size);/////'-----'
        if(pp && curr->call_function)
        {
            curr->call_function(pp,file_size,FILE_MODE);
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
		return -2;

	if(fop->info_buf == NULL)
	{
		fop->info_buf_len = BUFF_INCREASE_NUM_BASE;
		fop->info_buf = (char *)malloc(fop->info_buf_len);
		if(fop->info_buf == NULL)
			return -1;
		memset(fop->info_buf,0,fop->info_buf_len);
	}
	curr = fop->info_buf;
	
	if(init_parame->ip == NULL)
		return -2;
	len = frame_strlen(init_parame->ip);
	len++;
	init_parame->info_buf_len -= len;
	if(init_parame->info_buf_len < 0)
		return -2;
	memcpy(curr,init_parame->ip,(len - 1));
	fop->ip = curr;
	curr += len;

	if(init_parame->user == NULL)
		return -2;
	len = frame_strlen(init_parame->user);
	len++;
	init_parame->info_buf_len -= len;
	if(init_parame->info_buf_len < 0)
		return -2;
	memcpy(curr,init_parame->user,(len - 1));
	fop->user = curr;
	curr += len;

	if(init_parame->pass == NULL)
		return -2;
	len = frame_strlen(init_parame->pass);
	len++;
	init_parame->info_buf_len -= len;
	if(init_parame->info_buf_len < 0)
		return -2;
	memcpy(curr,init_parame->pass,(len - 1));
	fop->pass = curr;
	curr += len;

	if(init_parame->base_dir== NULL)
		return -2;
	len = frame_strlen(init_parame->base_dir);
	len += 2;
	init_parame->info_buf_len -= len;
	if(init_parame->info_buf_len < 0)
		return -2;
	memcpy(curr,init_parame->base_dir,(len - 1));
	if(curr[len -2] == '/')
	{
		curr[len - 1] = 0;
	}else{
		curr[len - 1] = '/';
	}
	fop->base_dir = curr;
	curr += len;

	len = sprintf(curr,init_parame->info_buf_len,"ftp://%s:%s@%s:%d/",fop->user,fop->pass,fop->ip,fop->port);
	if(len >= init_parame->info_buf_len)
		return -2;
	
	fop->time_out = init_parame->timeout;
	fop->base_dir = init_parame->base_dir;
	fop->port = init_parame->port;
}

int get_local_file_size(const char *path_file)
{
	struct stat st;
	if (-1 == stat(path_file,&st))
    {
        return 0;          
    }
    if ( ((int)st.st_size <= 0) || (st.st_size > MAX_LOCAL_FILE_SIZE) )
    {
        return 0;          
    }
    
    return (int)st.st_size;
}

void curl_scan_file_option(CURL_FTP_OPER_FD *curr,const char *url)
{
	CURLcode ret_set;
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_URL, url);
	if(ret_set != CURLE_OK)
	{
		;
	}
	
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_WRITEDATA, curr);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_WRITEFUNCTION, url);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTPLISTONLY, 0);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TIMEOUT, curr->time_out);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_CONNECTTIMEOUT, 60);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_NOSIGNAL, 1);
	if(ret_set != CURLE_OK)
	{
		;
	}

	if(curr->ftp_mode == CURL_FTP_EPRT)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPRT, 1);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}else{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPSV, 1);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}
}

void curl_upload_file_option(CURL_FTP_OPER_FD *curr,const char *url,FILE *local,int local_size,struct curl_slist *header_list)
{
	CURLcode ret_set;
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_URL, url);
	if(ret_set != CURLE_OK)
	{
		;
	}

	if(header_list)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_POSTQUOTE, header_list);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_READDATA, local);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_INFILESIZE, local_size);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TIMEOUT, curr->time_out);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_CONNECTTIMEOUT, 60);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_NOSIGNAL, 1);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_UPLOAD, 1);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
	if(ret_set != CURLE_OK)
	{
		;
	}

	if(curr->ftp_mode == CURL_FTP_EPRT)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPRT, 1);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}else{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPSV, 1);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}
}

void curl_download_file_option(CURL_FTP_OPER_FD *curr,const char *url,FILE *local)
{
	CURLcode ret_set;
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_URL, url);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_WRITEDATA, local);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TIMEOUT, curr->time_out);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_CONNECTTIMEOUT, 60);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_NOSIGNAL, 1);
	if(ret_set != CURLE_OK)
	{
		;
	}

	if(curr->ftp_mode == CURL_FTP_EPRT)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPRT, 1);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}else{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPSV, 1);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}
}

void cmd_curl_ftp_option(CURL_FTP_OPER_FD *curr,const char *url,struct curl_slist *header_list)
{
	CURLcode ret_set;
	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_URL, url);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_QUOTE, header_list);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_TIMEOUT, curr->time_out);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_CONNECTTIMEOUT, 60);
	if(ret_set != CURLE_OK)
	{
		;
	}

	ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_NOSIGNAL, 1);
	if(ret_set != CURLE_OK)
	{
		;
	}

	if(curr->ftp_mode == CURL_FTP_EPRT)
	{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPRT, 1);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}else{
		ret_set = curl_easy_setopt(curr->p_curl, CURLOPT_FTP_USE_EPSV, 1);
		if(ret_set != CURLE_OK)
		{
			;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////
int global_curl_ftp_init()
{
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
			return -1;
		}
		memset((void *)curr,0,sizeof(CURL_FTP_OPER_FD));
		curr->use_flag = 1;
	}else{
		curr->info_buf_len = BUFF_INCREASE_NUM_BASE;
		memset(curr->info_buf,0,curr->info_buf_len);
		curr->ip = NULL;
		curr->user = NULL;
		curr->pass = NULL;
		curr->base_dir = NULL;
		curr->base_curl = NULL;
	}
	
	if(head == NULL)
	{
		head == curr;
		tail = curr;
	}else{
		tail->next = curr;
		curr->pre = tail;
		tail = curr;
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
	if(call_function == NULL)
		return -2;
	
	for(i = 0;i < fop;i++)
	{
		if(curr == NULL)
			curr = head;
		else
			curr = curr->next;

		if(curr == NULL)
			return -2;
	}
	pthread_mutex_lock(&curr->scan_mutex);
	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
		return -3;

	if(re_path)
	{
		len = frame_strlen(re_path);
		len--;
		if(re_path[len] == '/')
			sprintf(cmd_buf,"%s%s%s",curr->base_curl,curr->base_dir,re_path);
		else
			sprintf(cmd_buf,"%s%s%s/",curr->base_curl,curr->base_dir,re_path);
	}else{
		sprintf(cmd_buf,"%s%s%s",curr->base_curl,curr->base_dir);
	}
	curl_scan_file_option(curr,cmd_buf);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		;
	}
	
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;

	pp = curr->file_name_buff;
    pp = find_file_name_point(pp,100,&file_size);/////'ddddddd'
    if(pp)
    {
        curr->call_function(pp,file_size,FOLDER_MODE);
    }else{
    	pp = find_file_name_point(pp,45,&file_size);/////'---'
    	if(pp)
    	{
    		curr->call_function(pp,file_size,FILE_MODE);
    	}
    }
    memset(curr->file_name_buff,0,BUFF_INCREASE_NUM_BASE);
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
	
	if(up_file == NULL || head == NULL)
		return -2;

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

	up_fp = fopen(up_file, "rb");
	if(up_fp == NULL)
	{
		return -3;
	}

	for(i = 0;i < fop;i++)
	{
		if(curr == NULL)
			curr = head;
		else
			curr = curr->next;

		if(curr == NULL)
			return -2;
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
		return -3;

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
		;
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

	for(i = 0;i < fop;i++)
	{
		if(curr == NULL)
			curr = head;
		else
			curr = curr->next;

		if(curr == NULL)
			return -2;
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
		return -3;

	down_fp = fopen(to_patn_name, "wab");
	if(down_fp == NULL)
		return -3;

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
		;
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
	
	if(origin_name == NULL || to_name == NULL)
		return -2;

	for(i = 0;i < fop;i++)
	{
		if(curr == NULL)
			curr = head;
		else
			curr = curr->next;

		if(curr == NULL)
			return -2;
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
		return -3;

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
		;
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
	
	if(file_name == NULL)
		return -2;

	for(i = 0;i < fop;i++)
	{
		if(curr == NULL)
			curr = head;
		else
			curr = curr->next;

		if(curr == NULL)
			return -2;
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
		return -3;

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
		;
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
	
	if(re_path == NULL)
		return -2;

	for(i = 0;i < fop;i++)
	{
		if(curr == NULL)
			curr = head;
		else
			curr = curr->next;

		if(curr == NULL)
			return -2;
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
		return -3;

	sprintf(cmd_buf,"RMD %s",re_path);
	header_list = curl_slist_append(header_list, cmd_buf);

	memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
	sprintf(cmd_buf,"%s%s",curr->base_curl,curr->base_dir);

	cmd_curl_ftp_option(curr,cmd_buf,header_list);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		;
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
	
	if(re_path == NULL)
		return -2;

	for(i = 0;i < fop;i++)
	{
		if(curr == NULL)
			curr = head;
		else
			curr = curr->next;

		if(curr == NULL)
			return -2;
	}

	if(curr->p_curl)
	{
		curl_easy_cleanup(curr->p_curl);
		curr->p_curl = NULL;
	}

	curr->p_curl = curl_easy_init();
	if(curr->p_curl == NULL)
		return -3;

	sprintf(cmd_buf,"MKD %s",re_path);
	header_list = curl_slist_append(header_list, cmd_buf);

	memset(cmd_buf,0,BUFF_CMD_CURLFTP_LEN);
	sprintf(cmd_buf,"%s%s",curr->base_curl,curr->base_dir);

	cmd_curl_ftp_option(curr,cmd_buf,header_list);
	ret = curl_easy_perform(curr->p_curl);
	if (ret != CURLE_OK)
	{
		;
	}
	curl_easy_cleanup(curr->p_curl);
	curr->p_curl = NULL;
	return (int)ret;
}

void global_curl_ftp_cleanup()
{
	CURL_FTP_OPER_FD *curr = NULL;
	pthread_mutex_lock(&init_fd_mutex);

	pthread_mutex_unlock(&init_fd_mutex);
}


