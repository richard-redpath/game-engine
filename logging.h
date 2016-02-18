#ifndef __LOGGING_H__
#define __LOGGING_H__

#define ANSI_COLOUR_RED       "\x1b[31m"
#define ANSI_COLOUR_RESET     "\x1b[0m"

#if defined(DEBUG) && !defined(NO_PRINT)
#define DEBUG_LOG(...) {fprintf(stdout,"[%s (%4d)] ", __FILE__, __LINE__);fprintf(stdout,__VA_ARGS__);fprintf(stdout,"\n");fflush(stdout);}
#define DEBUG_ERR(...) {fprintf(stderr,ANSI_COLOUR_RED);fprintf(stderr,"[%s (%4d)] ", __FILE__, __LINE__);fprintf(stderr,__VA_ARGS__);fprintf(stderr,ANSI_COLOUR_RESET);fprintf(stderr,"\n");fflush(stderr);}
#else
#define DEBUG_LOG(...)
#define DEBUG_ERR(...)
#endif

#endif
