
#include <pthread.h>

#include "common.h"
#include "log_mini.h"
#include "frame_tool.h"

#define LOG_RECORD_FILE_BUFLEN		(6*1024)
#define	LOG_RECORD_FILE_NAMELEN		512
#define LOG_RECORD_FILE_SUFFIXLEN	64

static const char* log_level_str[NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
};

typedef void (*OUTPUTFUCTION)(const char* msg, int len);
typedef void (*FLUSHFUNCTION)();

typedef struct log_record{
	OUTPUTFUCTION 		out;
	FLUSHFUNCTION		flu;
	LOG_LEVEL			lev;
}LOG_RECORD;

static LOG_RECORD log_recod = {0};

////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct log_file_info{
	FILE* fp_;
	char buffer_[LOG_RECORD_FILE_BUFLEN];
	size_t writtenBytes_;
}LOG_FILE_INFO;

static LOG_FILE_INFO log_file_info = {0};

void open_log_file(const char *filename)
{
	assert(log_file_info.fp_ == NULL);

	log_file_info.fp_ = fopen(filename, "ae");
	log_file_info.writtenBytes_ = 0;
	setbuffer(log_file_info.fp_, log_file_info.buffer_, LOG_RECORD_FILE_BUFLEN);

	assert(log_file_info.fp_);
}

void file_append_log(const char* logline, const size_t len)
{
	int err = 0;
	size_t remain = 0,n = 0,x = 0;
	n = fwrite_unlocked(logline,1,len,log_file_info.fp_);
	remain = len - n;
	while (remain > 0)
	{
		x = fwrite_unlocked(logline + n, 1, remain,log_file_info.fp_);
		if (x == 0)
		{
			err = ferror(log_file_info.fp_);
			if (err)
			{
				fprintf(stderr, "AppendFile::append() failed %d %s\n", err,strerror(err));
			}
			break;
		}
		n += x;
		remain = len - n; // remain -= x
	}
	
	log_file_info.writtenBytes_ += len;
}

void close_log_file()
{
	assert(log_file_info.fp_);
	
	fclose(log_file_info.fp_);
	log_file_info.fp_= NULL;
	memset(log_file_info.buffer_,0,LOG_RECORD_FILE_BUFLEN);
	log_file_info.writtenBytes_ = 0;
}

void file_flush_operation()
{
	assert(log_file_info.fp_);
	fflush(log_file_info.fp_);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct log_file{
	pthread_mutex_t 	mutex_;
	size_t 				rollSize_;
	char 				base_file_name[LOG_RECORD_FILE_NAMELEN];
}LOG_FILE;

static LOG_FILE log_file_ = {0};

void get_log_file_name(char *name,time_t* now)
{
	struct tm tm;
	int len = 0;
	char *p_char = name;

	len = frame_strlen(name);
	len += LOG_RECORD_FILE_SUFFIXLEN;
	if(len >= LOG_RECORD_FILE_NAMELEN)
	{
		fprintf(stderr, "Get log file name too long out of buffer\n");
	}

	len = frame_strlen(name);
	memcpy(p_char,log_file_.base_file_name,len);
	p_char = p_char + len;

	*now = time(NULL);
	gmtime_r(now, &tm);
	sprintf(p_char,".%d%d%d-%d%d%d.log",tm.tm_year,tm.tm_mon + 1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
	return;
}

void log_file_roll_file()
{
	time_t now = 0;
	char log_name[LOG_RECORD_FILE_NAMELEN] = {0};

	get_log_file_name(log_name,&now);

	if(log_file_info.fp_)
		close_log_file();
	open_log_file(log_name);
}

void log_file_outpuut(const char* msg, int len)
{
	pthread_mutex_lock(&(log_file_.mutex_));
	file_append_log(msg,len);
	if (log_file_info.writtenBytes_ > log_file_.rollSize_)
	{
		log_file_roll_file();
	}
	pthread_mutex_unlock(&(log_file_.mutex_));
}

void log_file_flush()
{
	file_flush_operation();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

void log_default_output(const char* msg, int len)
{
	size_t n = fwrite(msg, 1, len, stdout);
}

void log_default_flush()
{
	fflush(stdout);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
void log_set_level(LOG_LEVEL le)
{
	log_recod.lev = le;
}

void log_flush()
{
	if(log_recod.flu)
		log_recod.flu();
	else
		fprintf(stderr, "There is no define flush fuction\n");
}

void log(LOG_LEVEL level,const char *fil,const char *fun,int l_num,const char *fmt, ...)
{
	char msg[4096] = {0},*p_char = msg;
	va_list args;
	int n = 0;
	
	if(log_recod.out == NULL)
	{
		log_recod.out = log_default_output;
		log_recod.flu = log_default_flush;
		log_recod.lev = INFO;
	}

	if(level < log_recod.lev)
		return;

	sprintf(p_char,"%s %s %s %d",log_level_str[level],fil,fun,l_num);
	p_char = frame_end_charstr(p_char);
	n = p_char - msg;
	
	va_start(args, fmt);
	n += vsprintf(p_char, fmt, args);
	va_end(args);

	log_recod.out(msg,n);

	return;
}

void log_set_filename(const char *name,int size)
{
	int name_len = 0;

	name_len = frame_strlen(name);
	name_len += LOG_RECORD_FILE_SUFFIXLEN;
	if(name_len >= LOG_RECORD_FILE_NAMELEN)
	{
		fprintf(stderr, "Get log file name too long out of buffer\n");
		return;
	}

	name_len = frame_strlen(name);
	memcpy(log_file_.base_file_name,name,name_len);
	pthread_mutex_init(&(log_file_.mutex_), NULL);
	log_file_.rollSize_ = size;

	log_recod.out = log_file_outpuut;
	log_recod.flu = log_file_flush;
	log_recod.lev = INFO;
}

