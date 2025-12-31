/**
 * @file fastkst_localtime.h
 * @brief High performance KST (UTC+9) localtime implementation
 * @author lmk (newtypez@gmail.com)
 * @version 0.1
 * @date 2025-12-31
 *
 * @copyright Copyright (c) 2025
 *
 * @note
 *  - Performance: Optimized KST timezone conversion, faster than standard localtime()
 *  - 64-bit safe: Solves the 2038 problem, supports 64-bit time_t
 *  - Thread-safe option: Use fastkst_localtime_safe() for multi-threaded environments
 *  - Fixed timezone: Always returns KST (UTC+9) regardless of system timezone
 */

#ifndef FASTKST_LOCALTIME_H
#define FASTKST_LOCALTIME_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief High performance localtime for KST (64-bit safe version)
 * @param[in] t time_t (supports 64-bit)
 * @param[out] tp struct tm pointer to store the result
 * @return int 1 on success, 0 on failure
 *
 * @note Supported range:
 *       - 32-bit time_t: 1901-12-13 ~ 2038-01-19 (KST)
 *       - 64-bit time_t: approximately -2920 years ~ +2920 years
 *       - Actual limit: struct tm's tm_year (int) range
 *                       approximately -210,000 years ~ +210,000 years
 *
 * @note Error codes:
 *       - EINVAL: Invalid argument (NULL pointer)
 *       - EOVERFLOW: Year exceeds struct tm range
 *
 * @example
 * @code
 *   time_t now = time(NULL);
 *   struct tm kst_time;
 *   if (fastkst_localtime(now, &kst_time) == 1) {
 *       printf("%04d-%02d-%02d %02d:%02d:%02d\n",
 *              kst_time.tm_year + 1900,
 *              kst_time.tm_mon + 1,
 *              kst_time.tm_mday,
 *              kst_time.tm_hour,
 *              kst_time.tm_min,
 *              kst_time.tm_sec);
 *   }
 * @endcode
 */
int fastkst_localtime(time_t t, struct tm *tp);

/**
 * @brief Thread-safe wrapper with additional validation
 * @param[in] t time_t to convert
 * @param[out] tp struct tm pointer to store the result
 * @param[out] err_code error code pointer (optional, can be NULL)
 * @return int 1 on success, 0 on failure
 *
 * @note This function provides additional safety features:
 *       - Automatic struct tm initialization (memset to 0)
 *       - Explicit error code return via err_code parameter
 *       - NULL pointer validation
 *       - Safe for concurrent calls from multiple threads
 *
 * @note Error codes:
 *       - EINVAL: Invalid argument (NULL pointer for tp)
 *       - EOVERFLOW: Year exceeds struct tm range
 *
 * @example
 * @code
 *   time_t now = time(NULL);
 *   struct tm kst_time;
 *   int error_code;
 *
 *   if (fastkst_localtime_safe(now, &kst_time, &error_code) == 1) {
 *       printf("KST: %04d-%02d-%02d %02d:%02d:%02d %s\n",
 *              kst_time.tm_year + 1900,
 *              kst_time.tm_mon + 1,
 *              kst_time.tm_mday,
 *              kst_time.tm_hour,
 *              kst_time.tm_min,
 *              kst_time.tm_sec,
 *              kst_time.tm_zone);
 *   } else {
 *       fprintf(stderr, "Error: %d\n", error_code);
 *   }
 * @endcode
 */
int fastkst_localtime_safe(time_t t, struct tm *tp, int *err_code);

#ifdef __cplusplus
}
#endif

#endif /* FASTKST_LOCALTIME_H */
