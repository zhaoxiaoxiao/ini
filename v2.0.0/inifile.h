#ifndef __PROGRAM_INIFILE_VTWO_H__
#define __PROGRAM_INIFILE_VTWO_H__
#ifdef __cplusplus
extern "C" {
#endif


#define MAX_OPERATION_FILE_NUM			2//The macro define max inifile can operation same time
#define MAX_INIFILE_SECTION_NUM			10
#define MAX_KEYVALUE_UNDER_SECTION		10

#define INIFILE_SIZE_SPACE				(1024 * 3)
//////////////////////////////////////////////////////////////////////////

#define INIFILE_NO_EXIST				-1
#define INIFILE_FORMAT_ERROR			-2

#define INIFILE_ERROR_PARAMETER			-3
#define INIFILE_ERROR_ARRAYLEN			-4
#define INIFILE_ERROR_SYSTEM			-5

#define INIFILE_NUM_OUT_ARRAY			-6
#define INIFILE_SECTION_ARRAY			-7
#define INIFILE_KEYVALUE_ARRAY			-8

#define INIFILE_MEMORY_POOL_ERROR		-9
#define INIFILE_MEMORY_POOL_OUT			-10

#define INIFILE_SECTION_NOFOUND			-11
#define INIFILE_KEYVALUE_NOFOUND		-12

#define INIFILE_SECTION_ALREAD			-13
#define INIFILE_KEYVALUE_ALREAD			-14

#define INIFILE_REWRITE_ERROR			-15
/**
* operation parameter;
* every buff char must be end of '\0'
* every buff len must less MAX_STRING_LEN 
**/
typedef struct ini_parameter{
	char*	section;
	int		section_len;
	char*	key;
	int		key_len;
	char*	value;
	int		value_len;
}INI_PARAMETER;

int init_ini_file(const char *filename,int len);

const char *get_value_ofkey(int ini_fd,INI_PARAMETER *parameter);

int update_value_ofkey(int ini_fd,INI_PARAMETER *parameter);

int add_value_ofkey(int ini_fd,INI_PARAMETER *parameter);

int delete_value_ofkey(int ini_fd,INI_PARAMETER *parameter);

int delete_ini_section(int ini_fd,INI_PARAMETER *parameter);

int add_ini_section(int ini_fd,INI_PARAMETER *parameter);

void destroy_ini_source(int ini_fd);

#ifdef __cplusplus
}
#endif
#endif


