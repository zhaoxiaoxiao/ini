
#ifndef __PROGRAM_FRAMEWORK_LOG_H__
#define __PROGRAM_FRAMEWORK_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum log_level
{
	TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
}LOG_LEVEL;

#define LOG_FATAL(fmt, args...)		log(FATAL,__FILE__,__FUNCTION__,__LINE__,fmt, ## args)
#define LOG_ERROR(fmt, args...)		log(ERROR,__FILE__,__FUNCTION__,__LINE__,fmt, ## args)
#define LOG_WARN(fmt, args...)		log(WARN,__FILE__,__FUNCTION__,__LINE__,fmt, ## args)
#define LOG_INFO(fmt, args...)		log(INFO,__FILE__,__FUNCTION__,__LINE__,fmt, ## args)
#define LOG_DEBUG(fmt, args...)		log(DEBUG,__FILE__,__FUNCTION__,__LINE__,fmt, ## args)
#define LOG_TRACE(fmt, args...)		log(DEBUG,__FILE__,__FUNCTION__,__LINE__,fmt, ## args)


void log(LOG_LEVEL level,const char *fil,const char *fun,int l_num,const char *fmt, ...);

void log_set_level(LOG_LEVEL le);

void log_set_filename(const char *name,int size);


#ifdef __cplusplus
}
#endif
#endif

