
#include <pwd.h>
#include <sys/types.h>
#include <sys/file.h>

#include "common.h"
#include "frame_tool.h"

static int mutex_filelock = 0;
static const char mutex_file[] = "/tmp/single_instance.lock";
/////////////////////////////////////////////////////////////////////////////////////////////////////
int lock_mutex_file()
{
	int rc = 0;

	mutex_filelock = open(mutex_file, O_CREAT|O_RDWR, 0666);
	rc = flock(mutex_filelock, LOCK_EX|LOCK_NB);
	if (rc && EWOULDBLOCK == errno)
	{
		PERROR("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		PERROR("\t balabalabalabalabalabalabala\n");
		PERROR("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		return rc;
	}
	
	return 0;
}

void release_mutex_file()
{
	if(mutex_filelock > 0)
	{
		flock(mutex_filelock, LOCK_UN);
		close(mutex_filelock);
		mutex_filelock = 0;
	}
}

int cpu_isbig_endian()
{
	union  
    {  
        int a;  
        char b;  
    }c;

	c.a = 1;
	if(c.b == 1)
	{
		PDEBUG("Notice::::::This cpu is little endian!!!!!!\n");
		return 0;
	}else{
		PDEBUG("Notice::::::This cpu is big endian!!!!!!!!!!\n");
		return 1;
	}
}

int get_local_file_size(const char *path_file)
{
	struct stat st;
	if (-1 == stat(path_file,&st))
    {
        return 0;          
    }
    if ( ((int)st.st_size <= 0))
    {
        return 0;          
    }
    
    return (int)st.st_size;
}

char *frame_end_charstr(char *buff)
{
	char *p = buff;
	while(*p!=0)
	{
		p++;
	}
	return p;
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

int framestr_isall_digital(const char *str)
{
	int flag = 0;
	const char *p = str;

	while(*p)
	{
		if(*p == 43 || *p == 45)
		{
			if(flag == 0)
			{
				flag++;
				p++;
			}else{
				flag = 0;
				break;
			}
		}
		
		if(*p < 48 || *p > 57)
		{
			flag = 0;
			break;
		}
		
		p++;
		flag++;
	}
	
	return flag;
}

char* framestr_frist_constchar(char *str,char c)
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

char* framestr_start_digital_char(char *dig_str)
{
	char *p = dig_str;
	if(!dig_str)
		return NULL;
	while(*p)
	{
		if(*p >= 48 && *p <= 57)
			break;
		p++;
	}
	return p;
}

char* framestr_end_digital_char(char *dig_str)
{
	char *p = dig_str;
	if(!dig_str)
		return NULL;
	while(*p)
	{
		if(*p < 48 || *p > 57)
			break;
		p++;
	}
	return p;
}

char* framestr_last_constchar(char *str,char c)
{
	char *p = NULL;
	if(!str)
		return NULL;
	p = frame_end_charstr(str);
	while(p >= str)
	{
		if(*p == c)
			return p;
		else
			p--;
	}
	return NULL;
}

char* framestr_first_conststr(char *str,char *tar)
{
	const char *q = tar;
	const char tar_fri = *tar;
	char *p = str,*p_for = NULL;
	int p_len = 0,q_len = 0,i;

	if(!str)
		return NULL;
	p_len = frame_strlen(p);
	q_len = frame_strlen(q);
	if(q_len > p_len)
		return NULL;
	
	do{
		p_for = framestr_frist_constchar(p,tar_fri);
		if(!p_for)
			return NULL;
		else
			p = p_for;
		p_len = frame_strlen(p);
		if(q_len > p_len)
			return NULL;
		for(i = 0;i < q_len;i++)
		{
			if(*p_for == *q)
			{
				p_for++;
				q++;
				continue;
			}else{
				break;
			}
		}
		if(i == q_len)
			return p;
		else
		{
			p++;
			q = tar;
		}
	}while(1);

	return NULL;
}

char *current_user_name(char *name,int len)
{
	//uid_t my_uid;
	int name_len = 0;
	struct passwd *my_info = NULL;

	my_info = getpwuid( getuid() );

	printf( "my name = [%s]\n", my_info->pw_name );
	printf( "my passwd = [%s]\n", my_info->pw_passwd );
	printf( "my uid = [%d]\n", my_info->pw_uid );
	printf( "my gid = [%d]\n", my_info->pw_gid );
	printf( "my gecos = [%s]\n", my_info->pw_gecos );
	printf( "my dir = [%s]\n", my_info->pw_dir );
	printf( "my shell = [%s]\n", my_info->pw_shell );

	name_len = frame_strlen(my_info->pw_name);
	if(name_len >= len)
	{
		return NULL;
	}

	strcpy(name,my_info->pw_name);
	return name;
}

char *frame_getcwd(char *cwd,int len)
{
	return getcwd(cwd,len); 
}

size_t get_system_pagesize()
{
	size_t page = 0;
	page = getpagesize();
	PDEBUG("page size = %d Byte\n",page);
	return page;
}

int frame_call_shell_cmd(const char *cmd)
{
	pid_t status;
	status = system(cmd);
	
	if (-1 == status)  
    {  
        PERROR("system error!\n");  
    }  
    else  
    {  
        PDEBUG("exit status value = [0x%x]\n", status);  
  
        if (WIFEXITED(status))  
        {  
            if (0 == WEXITSTATUS(status))  
            {  
                PDEBUG("run shell script successfully.\n");  
            }  
            else  
            {  
                PERROR("run shell script fail, script exit code: %d\n", WEXITSTATUS(status));  
            }  
        }  
        else  
        {  
            PERROR("exit status = [%d]\n", WEXITSTATUS(status));  
        }  
    }  
  
    return 0; 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void copy_file_to_targetfile(const char *src_file,const char *dest_file)
{
	int result = 0,len = 0;
	char copy_buf[1024] = {0};
	FILE *in = NULL,*out = NULL;
	PERROR("%s copy to %s\n",src_file,dest_file);

	result = access(src_file, F_OK);
    if(result != 0)
    {
        PERROR("file  %s read error!errno:%d,err:%s\n",src_file,errno,strerror(errno));
        return;
    }

	in = fopen(src_file,"rb");
    if (NULL == in)
    {
        PERROR("open File %s failed!errno:%d,err:%s",src_file,errno,strerror(errno));
        return;            
    }

	out = fopen(dest_file,"wab");
    if (NULL == out)
    {
        PERROR("open File %s failed!errno:%d,err:%s",dest_file,errno,strerror(errno));
        fclose(in);
        return;            
    }
	do{
        len=fread(copy_buf,1,1024,in);
        if(len > 0)
        {
            fwrite(copy_buf,1,len,out);
            memset(copy_buf,0,1024);
        }
    }while(len > 0);
    fclose(out);  
    fclose(in);
	return;
}

#define MAC_FULL_PATH_LEN			1024

void foreach_filedir_under_folder(const char *dir_path)
{
	int len = 0;
	DIR *p_dir = NULL;
	struct dirent *p_dirent = NULL;
	char folder_path[MAC_FULL_PATH_LEN] = {0},file_dir_path[MAC_FULL_PATH_LEN] = {0};

	if(dir_path == NULL)
	{
		PERROR("The transmit parameter is null but can't be null\n");
		return;
	}
	len = frame_strlen(dir_path);
	if(len >= (MAC_FULL_PATH_LEN - 1))
	{
		PERROR("The dir full path too long\n");
		return;
	}

	memcpy(folder_path,dir_path,len);
	if(folder_path[len - 1] != '/')
		folder_path[len] = '/';

	p_dir = opendir(folder_path);
	do{
		p_dirent = readdir(p_dir);
		if(p_dirent)
		{
			if(p_dirent->d_type == DT_DIR)
			{
				PDEBUG("Folder name:: ");
			}else{
				len = snprintf(file_dir_path,MAC_FULL_PATH_LEN,"%s%s",dir_path,p_dirent->d_name);
				if(len < MAC_FULL_PATH_LEN)
					PDEBUG("File size %d name:: ",get_local_file_size(file_dir_path));
				else
					PDEBUG("File name len more than buff can't get size and name:: ");
			}
			printf("%s\n",p_dirent->d_name);
		}
	}while(p_dirent);
	closedir(p_dir);	 
}

void call_file_system_running(const char *file_name)
{
	if(execve(file_name,NULL,NULL) < 0)
	{
		perror("execl error!");
	}
}

