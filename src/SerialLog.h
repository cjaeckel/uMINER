#ifndef _SeriaLog_H
#define _SeriaLog_H

#include <Arduino.h>

#ifndef UMN_LOG_LEVEL //default log level
#define UMN_LOG_LEVEL UMN_LOG_LEVEL_INFO
#endif

#define UMN_LOG_LEVEL_ALL        (0)
#define UMN_LOG_LEVEL_ERROR      (1)
#define UMN_LOG_LEVEL_WARN       (2)
#define UMN_LOG_LEVEL_INFO       (3)
#define UMN_LOG_LEVEL_DEBUG      (4)
#define UMN_LOG_LEVEL_TRACE      (5)

#define LOG_FRMT(format) "[%s:%d] " format, __FILE__, __LINE__
#define IS_LOG_LEV(level) (UMN_LOG_LEVEL >= (level))

#if IS_LOG_LEV(UMN_LOG_LEVEL_TRACE)
#define logTRC(format, ...) do {Serial.printf(LOG_FRMT(format), ##__VA_ARGS__);} while(0)
#else
#define logTRC(format, ...)  do {} while(0)
#endif

#if IS_LOG_LEV(UMN_LOG_LEVEL_DEBUG)
#define logDBG(format, ...) do {Serial.printf(LOG_FRMT(format), ##__VA_ARGS__);} while(0)
#define logDBG0(format, ...) do {Serial.printf(format, ##__VA_ARGS__);} while(0)
#else
#define logDBG(format, ...)  do {} while(0)
#define logDBG0(format, ...)  do {} while(0)
#endif

#if IS_LOG_LEV(UMN_LOG_LEVEL_INFO)
#define logINF(format, ...) do {Serial.printf(LOG_FRMT(format), ##__VA_ARGS__);} while(0)
#define logINF0(format, ...) do {Serial.printf(format, ##__VA_ARGS__);} while(0)
#else
#define logINF(format, ...)  do {} while(0)
#define logINF0(format, ...)  do {} while(0)
#endif

#if IS_LOG_LEV(UMN_LOG_LEVEL_WARN)
#define logWRN(format, ...) do {Serial.printf(LOG_FRMT(format), ##__VA_ARGS__);} while(0)
#else
#define logWRN(format, ...)  do {} while(0)
#endif

#if IS_LOG_LEV(UMN_LOG_LEVEL_ERROR)
#define logERR(format, ...) do {Serial.printf(LOG_FRMT(format), ##__VA_ARGS__);} while(0)
#else
#define logERR(format, ...)  do {} while(0)
#endif

#if IS_LOG_LEV(UMN_LOG_LEVEL_ALL)
#define logALL(format, ...) do {Serial.printf(LOG_FRMT(format), ##__VA_ARGS__);} while(0)
#else
#define logALL(format, ...)  do {} while(0)
#endif

#endif  //_SeriaLog_H
