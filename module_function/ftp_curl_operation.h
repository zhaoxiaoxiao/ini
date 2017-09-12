
#ifndef __PROGRAM_FRAMEWORK_FTPCURL_H__
#define __PROGRAM_FRAMEWORK_FTPCURL_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum file_name_mode{
	FILE_MODE,
	FOLDER_MODE,
}FILE_NAME_MODE;

typedef enum curl_ftp_work_mode{
	CURL_FTP_EPRT,
	CURL_FTP_EPSV,
}CURL_FTP_WORK_MODE;

typedef struct curl_ftp_init_parame{
	const char *ip;
	const char *user;
	const char *pass;
	const char *base_dir;

	int timeout;
	CURL_FTP_WORK_MODE mode;

	unsigned short port;
}CURL_FTP_INIT_PARAME;

typedef void (*SCAN_CURL_FTP_FOLDER_CALLBACK)(const char *file_name,int file_size,FILE_NAME_MODE mode);

int global_curl_ftp_init();

int init_curl_ftp_fd(CURL_FTP_INIT_PARAME *init_parame);

int scan_curl_ftp_folder(int fop,const char *re_path,SCAN_CURL_FTP_FOLDER_CALLBACK call_function);

int upload_curl_ftp_file(int fop,const char *up_file,const char *re_path,const char *to_name);

int download_curl_ftp_file(int fop,const char *down_file,const char *re_path,const char *to_patn_name);

int rename_curl_ftp_file(int fop,const char *origin_name,const char *re_path,const char *to_name);

int delete_curl_ftp_file(int fop,const char *re_path,const char *file_name);

int delete_curl_ftp_dir(int fop,const char *re_path);

int create_curl_ftp_dir(int fop,const char *re_path);

void global_curl_ftp_cleanup();

#ifdef __cplusplus
}
#endif
#endif


