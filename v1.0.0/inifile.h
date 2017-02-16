
/*
*   程序设计为读写操作ini文件使用，设计思想：
*   ini文件节点构成一个链表，链表中每个节点下有键值链表,如果文件刚刚开始为注释，则为一个节点。
*   每对键值构成键值链表中一个节点，
*   其实读写ini文件就是对这些节点的操作
*   ini文件中空白符与制表符自动忽略
*
*
*	注意下面几个常量的定义，长度不能越界
*	每个节点或者键值字符串最大长度不能大于 MAX_STRING_LEN
*	文件名长度不能大于MAX_FILENAME_LEN
*	配置文件中每一行长度不能大于 MAX_FILLINE_LEN
*	传递参数字符数组必须以'\0'结尾
*/

#define MAX_STRING_LEN 		64//定义字符串最大长度
#define MAX_FILENAME_LEN	64//定义文件名最大长度
#define MAX_FILLINE_LEN		128//定义文件名每一行最大长度

#define CONTENT_NEELINE			10
#define CONTENT_SPACE			32
#define CONTENT_TAB				9
#define CONTENT_NULL			0
#define CONTENT_SECTION			91//[
#define CONTENT_COMMA			59//;
#define CONTENT_EQUALITY		61//=

typedef enum LineType{
	EMPTYLIST = -5,//
    WRITEERROR = -4,//写文件严重错误
	REPEATNODEKEY = -3,//节点或者节点下键值重复
    OPENFILEERROR = -2,//打开文件错误
    TOOLONG = -1,//行内容太长
    ERRORTYPR = 0,//既不是节点，也不是节点，在文章中的这一行直接被忽略
    NODETYPE = 1,//节点
    KEYVALUE = 2,//键值
    NOTESTYPE = 3,//注释，注释可以作为节点存储也可以作为节点下键值存储
}LineType;

typedef enum method_type{
	initIniFile_type,
	getValueOfKey_type,
	updateValueOfKey_type,
	addValueOfKey_type,
	deleteValueOfKey_type,
	deleteSection_type,
	addSetction_type,
	exitOperationIniFile_type,
}METHOD_TYPE;

/*
*ini文件键值节点结构体
*/
typedef struct KeyValueNode{
    char key[(MAX_STRING_LEN+4)];
    char value[(MAX_STRING_LEN+4)];
    int isNote;//是否是注释，1是注释，默认是0，不是注释
    struct KeyValueNode *pre;
    struct KeyValueNode *next;
}KeyValueNode;

//ini文件键值节点链表结构体
typedef struct ListkeyValueNode{
    KeyValueNode *head;
    KeyValueNode *tail;
    int len;
}ListkeyValueNode;

//ini文件中节点节点结构体
typedef struct IniFileNode{
    char section[(MAX_STRING_LEN+4)];
    int isNote;//是否是注释，1是注释，默认是0，不是注释
    ListkeyValueNode *listkeyValueNode;
    struct IniFileNode *pre;
    struct IniFileNode *next;
}IniFileNode;

//ini文件中节点构成的链表
typedef struct ini_file{
    IniFileNode *head;
    IniFileNode *tail;
    char fileName[MAX_FILENAME_LEN];
    int len;
}INI_FILE;

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


/* *
 * *  @brief       	初始化一个ini文件
 * *  @author      	zhaoxiaoxiao
 * *  @date        	2014-06-21
 * *  @fileName   	文件路径
 * *  @len   		文件路径长度，必须小于MAX_FILENAME_LEN,end with '\0'
 * *  @return      	ini文件在内存中操作的链表头指针，空说明初始化失败
 * */
INI_FILE *initIniFile(const char *fileName,int len);

/* *
 * *  @brief       	                通过建获取ini文件值
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针,
 * *  @parameter                    节点名称,参数结构体，详见结构体描述
 * *  @return      	                值指针；value will be lost after exitOperationIniFile
 * */
char *getValueOfKey(INI_FILE *p_inifile,INI_PARAMETER *parameter);

/* *
 * *  @brief       	                修改一个ini文件键值，只能修改值，不能修改键
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter                    节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，其他失败，-1参数错误，-2节点没有找到，-3键值没有找到
 * */
int updateValueOfKey(INI_FILE *p_inifile,INI_PARAMETER *parameter);

/* *
 * *  @brief       	                增加一个ini文件键值
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter                    节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，-1没有参数错误，-2节点没有找到，-3是已存在键值
 * */
int addValueOfKey(INI_FILE *p_inifile,INI_PARAMETER *parameter);

/* *
 * *  @brief       	                删除一个ini文件键值
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter					节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，-1没有参数错误，-2节点没有找到，-3是没有找到键值
 * */
int deleteValueOfKey(INI_FILE *p_inifile,INI_PARAMETER *parameter);

/* *
 * *  @brief       	                删除一个ini文件节点
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter					节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，-1没有参数错误，-2节点没有找到，
 * */
int deleteSection(INI_FILE *p_inifile,INI_PARAMETER *parameter);

/* *
 * *  @brief       	                增加一个ini文件节点
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter                    节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，-1没有参数错误，-2节点已经存在
 * */
int addSetction(INI_FILE *p_inifile,INI_PARAMETER *parameter);

/* *
 * *  @brief       	                清除一个ini文件所有操作，释放所有内存
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @return      	                0操作成功，-1没有参数错误?
 * */
int exitOperationIniFile(INI_FILE **p_inifile);



