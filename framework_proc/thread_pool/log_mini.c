
#include <stdarg.h>

#include "common.h"
#include "log_mini.h"
#include "frame_tool.h"

#define LOG_RECORD_FILE_BUFLEN		(6*1024)
#define	LOG_RECORD_FILE_NAMELEN		512

static const char* log_level_str[NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};


typedef void (*OUTPUTFUCTION)(const char* msg, int len);
typedef void (*FLUSHFUNCTION)();

typedef struct log_record{
	int					is_file;
	OUTPUTFUCTION 		out;
	FLUSHFUNCTION		flu;
	LOG_LEVEL			lev;
}LOG_RECORD;

static LOG_RECORD log_reco = {0};
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
void log(LOG_LEVEL level,const char *fil,const char *fun,int l_num,const char *fmt, ...)
{
	char msg[4096] = {0},*p_char = msg;
	va_list args;
	int n = 0;
	
	if(log_reco.out == NULL)
	{
		log_reco.out = log_default_output;
		log_reco.flu = log_default_flush;
		log_reco.lev = INFO;
		log_reco.is_file = 0;
	}

	if(level < log_reco.lev)
		return;

	sprintf(p_char,"%s %s %s %d",log_level_str[level],fil,fun,l_num);
	p_char = frame_end_charstr(p_char);
	n = p_char - msg;
	
	va_start(args, fmt);
	n += vsprintf(p_char, fmt, args);
	va_end(args);

	if(log_reco.is_file == 0)
		log_reco.out(msg,n);

	return;
}

