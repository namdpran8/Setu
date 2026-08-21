#pragma once
#include <android-base/logging.h>

#define ALOGV(...) ::android::base::LogMessage(__FILE__, __LINE__, static_cast<::android::base::LogSeverity>(0), "Windroid", -1).stream() << __VA_ARGS__
#define ALOGD(...) ::android::base::LogMessage(__FILE__, __LINE__, static_cast<::android::base::LogSeverity>(1), "Windroid", -1).stream() << __VA_ARGS__
#define ALOGI(...) ::android::base::LogMessage(__FILE__, __LINE__, static_cast<::android::base::LogSeverity>(2), "Windroid", -1).stream() << __VA_ARGS__
#define ALOGW(...) ::android::base::LogMessage(__FILE__, __LINE__, static_cast<::android::base::LogSeverity>(3), "Windroid", -1).stream() << __VA_ARGS__
#define ALOGE(...) ::android::base::LogMessage(__FILE__, __LINE__, static_cast<::android::base::LogSeverity>(4), "Windroid", -1).stream() << __VA_ARGS__
