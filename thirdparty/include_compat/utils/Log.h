#pragma once
#include <stdio.h>

#define ALOGE(...) do { fprintf(stderr, "E: " __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define ALOGW(...) do { fprintf(stderr, "W: " __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define ALOGI(...) do { fprintf(stdout, "I: " __VA_ARGS__); fprintf(stdout, "\n"); } while(0)
#define ALOGD(...) do { fprintf(stdout, "D: " __VA_ARGS__); fprintf(stdout, "\n"); } while(0)
#define ALOGV(...) do { fprintf(stdout, "V: " __VA_ARGS__); fprintf(stdout, "\n"); } while(0)

#define LOG_FATAL_IF(cond, ...) do { if (cond) { ALOGE(__VA_ARGS__); abort(); } } while(0)
#define LOG_ALWAYS_FATAL(...) do { ALOGE(__VA_ARGS__); abort(); } while(0)
#define LOG_ALWAYS_FATAL_IF(cond, ...) do { if (cond) { ALOGE(__VA_ARGS__); abort(); } } while(0)

// sometimes LOGE is used
#define LOGE ALOGE
#define LOGW ALOGW
#define LOGI ALOGI
#define LOGD ALOGD
#define LOGV ALOGV
