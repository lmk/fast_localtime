/**
 * @file fastkst_localtime.c
 * @author lmk (newtypez@gmail.com)
 * @brief KST(한국 표준시, UTC+9)를 위한 고성능 localtime 구현
 * @version 0.1
 * @date 2025-12-31
 * 
 * @copyright Copyright (c) 2025
 * 
 * @note
 *  - 고성능: KST 타임존 변환에 최적화되어 표준 localtime() 대비 현저히 빠른 성능
 *            localtime()은 linux 커널에서 구현된 함수로, timezone 조회, mutex 사용 등 많은 오버헤드가 있어 성능이 느림
 *  - 64-bit safe: 2038년 문제 해결, 2038-01-19 이후의 time_t 지원
 *        - time_t 지원 범위: 약 -2920억년 ~ +2920억년
 *        - struct tm의 tm_year 필드 실질적 제한: 약 -21억년 ~ +21억년
 *  - Thread-safe 옵션: 멀티스레드 환경에서는 fastkst_localtime_safe() 사용 권장
 *  - 고정 타임존: 시스템 타임존 설정과 무관하게 항상 KST(UTC+9) 반환
 *  - 테스트 코드 제공: TEST_FASTKST_LOCALTIME 정의 시 테스트 코드 활성화
 */
#include <time.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#define __isleap(year)        \
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

const unsigned short int __mon_yday[2][13] =
  {
    /* Normal years.  */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* Leap years.  */
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
  };

#define SECS_PER_HOUR   (60 * 60)
#define SECS_PER_DAY    (SECS_PER_HOUR * 24)

/**
 * @brief 64-bit safe time conversion function
 * @param[in] t time_t (supports 64-bit)
 * @param[in] offset timezone offset in seconds
 * @param[out] tp struct tm
 * @return int 1 success, 0 fail
 */
int __offtime64(time_t t, long int offset, struct tm *tp)
{
  int64_t days, rem;
  int64_t y;
  const unsigned short int *ip;
  
  if (tp == NULL) {
    errno = EINVAL;
    return 0;
  }

  days = t / SECS_PER_DAY;
  rem = t % SECS_PER_DAY;
  rem += offset;
  
  while (rem < 0)
    {
      rem += SECS_PER_DAY;
      --days;
    }
  while (rem >= SECS_PER_DAY)
    {
      rem -= SECS_PER_DAY;
      ++days;
    }
  
  tp->tm_hour = (int)(rem / SECS_PER_HOUR);
  rem %= SECS_PER_HOUR;
  tp->tm_min = (int)(rem / 60);
  tp->tm_sec = (int)(rem % 60);
  
  /* January 1, 1970 was a Thursday.  */
  tp->tm_wday = (int)((4 + days) % 7);
  if (tp->tm_wday < 0)
    tp->tm_wday += 7;
  
  y = 1970;

#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV (y, 4) - DIV (y, 100) + DIV (y, 400))

  while (days < 0 || days >= (__isleap (y) ? 366 : 365))
    {
      /* Guess a corrected year, assuming 365 days per year.  */
      int64_t yg = y + days / 365 - (days % 365 < 0);
      
      /* Adjust DAYS and Y to match the guessed year.  */
      days -= ((yg - y) * 365
               + LEAPS_THRU_END_OF (yg - 1)
               - LEAPS_THRU_END_OF (y - 1));
      y = yg;
    }
  
  /* tm_year 범위 체크: struct tm의 tm_year는 int 타입 */
  /* 1900을 기준으로 하므로 실질적 범위는 INT_MIN+1900 ~ INT_MAX+1900 */
  if (y < (int64_t)INT_MIN + 1900 || y > (int64_t)INT_MAX + 1900)
    {
      errno = EOVERFLOW;
      return 0;
    }
  
  tp->tm_year = (int)(y - 1900);
  tp->tm_yday = (int)days;
  
  ip = __mon_yday[__isleap(y)];
  for (y = 11; days < (long int) ip[y]; --y)
    continue;
  days -= ip[y];
  tp->tm_mon = (int)y;
  tp->tm_mday = (int)(days + 1);
  
  return 1;
}

/**
 * @brief high performance localtime for KST (64-bit safe version)
 * @param[in] t time_t (supports 64-bit)
 * @param[out] tp struct tm
 * @return int 1 success, 0 fail
 * 
 * @note 지원 범위:
 *       - 32-bit time_t: 1901-12-13 ~ 2038-01-19 (KST)
 *       - 64-bit time_t: 약 -2920억년 ~ +2920억년
 *       - 실질적 제한: struct tm의 tm_year(int) 범위
 *                     약 -21억년 ~ +21억년
 */
int fastkst_localtime(time_t t, struct tm *tp)
{
  // KST offset: UTC+9
  const long int kst_offset = 3600 * 9;
  int ret;
  
  if (tp == NULL) {
    errno = EINVAL;
    return 0;
  }
  
  ret = __offtime64(t, kst_offset, tp);
  
  if (ret == 1) {
    // normalize timezone info
    tp->tm_gmtoff = kst_offset;
    tp->tm_zone = "KST";
    tp->tm_isdst = 0;
  }
  
  return ret;
}

/**
 * @brief Thread-safe wrapper with additional validation
 * @param[in] t time_t
 * @param[out] tp struct tm
 * @param[out] err_code error code (optional, can be NULL)
 * @return int 1 success, 0 fail
 */
int fastkst_localtime_safe(time_t t, struct tm *tp, int *err_code)
{
  int ret;
  int local_errno = 0;
  
  if (tp == NULL) {
    if (err_code) *err_code = EINVAL;
    return 0;
  }
  
  // struct tm 초기화
  memset(tp, 0, sizeof(struct tm));
  
  ret = fastkst_localtime(t, tp);
  
  if (ret == 0) {
    local_errno = errno;
    if (err_code) *err_code = local_errno;
  }
  
  return ret;
}

/* 테스트 코드 */
#ifdef TEST_FASTKST_LOCALTIME
/* 빌드 방법 
gcc -DTEST_FASTKST_LOCALTIME -o fastkst_localtime fastkst_localtime.c -lpthread
./fastkst_localtime

*/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define NUM_THREADS 10
#define ITERATIONS_PER_THREAD 1000

typedef struct {
  time_t test_time;
  int thread_id;
  int success_count;
  int fail_count;
} thread_data_t;

void *thread_test_func(void *arg)
{
  thread_data_t *data = (thread_data_t *)arg;
  struct tm result;
  int ret;
  int i;
  
  for (i = 0; i < ITERATIONS_PER_THREAD; i++) {
    memset(&result, 0, sizeof(result));
    ret = fastkst_localtime_safe(data->test_time, &result, NULL);
    
    if (ret == 1) {
      // 결과 검증
      if (result.tm_year + 1900 < 1900 || result.tm_year + 1900 > 3000) {
        data->fail_count++;
      } else if (result.tm_mon < 0 || result.tm_mon > 11) {
        data->fail_count++;
      } else if (result.tm_mday < 1 || result.tm_mday > 31) {
        data->fail_count++;
      } else if (result.tm_hour < 0 || result.tm_hour > 23) {
        data->fail_count++;
      } else if (result.tm_min < 0 || result.tm_min > 59) {
        data->fail_count++;
      } else if (result.tm_sec < 0 || result.tm_sec > 59) {
        data->fail_count++;
      } else {
        data->success_count++;
      }
    } else {
      data->fail_count++;
    }
    
    // 다른 스레드와 경쟁 조건을 만들기 위해 약간의 지연
    usleep(1);
  }
  
  return NULL;
}

// 성능 측정을 위한 헬퍼 함수
static double get_time_usec(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}

// 성능 비교 함수
void benchmark_localtime_vs_fastkst(int iterations)
{
  time_t test_time = time(NULL);
  time_t test_time_for_validation = test_time;  // 검증용 고정 시간
  struct tm result1, result2;
  double start, end;
  double time_localtime = 0.0;
  double time_fastkst = 0.0;
  int i;
  
  printf("\n=== Performance Benchmark ===\n\n");
  printf("Iterations: %d\n", iterations);
  printf("Test time_t: %lld\n\n", (long long)test_time);
  
  // 표준 localtime 성능 측정
  start = get_time_usec();
  for (i = 0; i < iterations; i++) {
    time_t t = test_time + (i % 100 == 0 ? 1 : 0);  // CPU 캐시 효과를 줄이기 위해 약간의 변동
    struct tm *tmp = localtime(&t);
    if (tmp != NULL && i == iterations - 1) {
      // 마지막 반복 결과 저장 (비교용)
      result1 = *tmp;
    }
  }
  end = get_time_usec();
  time_localtime = (end - start) / iterations;
  
  // fastkst_localtime 성능 측정 (동일한 test_time 사용)
  start = get_time_usec();
  for (i = 0; i < iterations; i++) {
    time_t t = test_time + (i % 100 == 0 ? 1 : 0);  // 동일한 패턴 사용
    fastkst_localtime(t, &result2);
  }
  end = get_time_usec();
  time_fastkst = (end - start) / iterations;
  
  // 결과 출력
  printf("Results:\n");
  printf("  localtime():        %.3f microseconds/call\n", time_localtime);
  printf("  fastkst_localtime(): %.3f microseconds/call\n", time_fastkst);
  
  if (time_localtime > 0) {
    double speedup = time_localtime / time_fastkst;
    double improvement = ((time_localtime - time_fastkst) / time_localtime) * 100.0;
    printf("\n  Speedup: %.2fx faster\n", speedup);
    printf("  Improvement: %.2f%% faster\n", improvement);
    
    if (speedup > 1.0) {
      printf("  [SUCCESS] fastkst_localtime is faster than localtime()!\n");
    } else if (speedup < 1.0) {
      printf("  [NOTE] localtime is faster (%.2fx)\n", 1.0 / speedup);
    } else {
      printf("  [NOTE] Similar performance\n");
    }
  }
  
  // 결과 일치성 검증 (동일한 test_time에 대해)
  printf("\nResult Validation (using same time_t: %lld):\n", (long long)test_time_for_validation);
  struct tm *tmp1 = localtime(&test_time_for_validation);
  struct tm result1_fixed, result2_fixed;
  if (tmp1 != NULL) {
    result1_fixed = *tmp1;
  }
  fastkst_localtime(test_time_for_validation, &result2_fixed);
  
  // 시간대 차이 계산 (KST는 UTC+9이므로 9시간 차이)
  int hour_diff = result2_fixed.tm_hour - result1_fixed.tm_hour;
  if (hour_diff < 0) hour_diff += 24;
  
  if (result1_fixed.tm_year == result2_fixed.tm_year &&
      result1_fixed.tm_mon == result2_fixed.tm_mon &&
      result1_fixed.tm_mday == result2_fixed.tm_mday &&
      result1_fixed.tm_hour == result2_fixed.tm_hour &&
      result1_fixed.tm_min == result2_fixed.tm_min &&
      result1_fixed.tm_sec == result2_fixed.tm_sec) {
    printf("  [PASS] Results match (year/month/day/hour/min/sec)\n");
    printf("  Note: System timezone appears to be KST\n");
  } else if (hour_diff == 9 || hour_diff == 15) {
    // UTC+9 차이 (9시간 또는 15시간 = -9시간)
    printf("  [PASS] Results differ by timezone offset (expected):\n");
    printf("    localtime (system TZ):    %04d-%02d-%02d %02d:%02d:%02d\n",
           result1_fixed.tm_year + 1900, result1_fixed.tm_mon + 1, result1_fixed.tm_mday,
           result1_fixed.tm_hour, result1_fixed.tm_min, result1_fixed.tm_sec);
    printf("    fastkst_localtime (KST):  %04d-%02d-%02d %02d:%02d:%02d\n",
           result2_fixed.tm_year + 1900, result2_fixed.tm_mon + 1, result2_fixed.tm_mday,
           result2_fixed.tm_hour, result2_fixed.tm_min, result2_fixed.tm_sec);
    printf("    Timezone difference: %d hours (KST = UTC+9)\n", hour_diff == 9 ? 9 : -9);
  } else {
    printf("  [WARN] Results differ unexpectedly:\n");
    printf("    localtime (system TZ):    %04d-%02d-%02d %02d:%02d:%02d\n",
           result1_fixed.tm_year + 1900, result1_fixed.tm_mon + 1, result1_fixed.tm_mday,
           result1_fixed.tm_hour, result1_fixed.tm_min, result1_fixed.tm_sec);
    printf("    fastkst_localtime (KST):  %04d-%02d-%02d %02d:%02d:%02d\n",
           result2_fixed.tm_year + 1900, result2_fixed.tm_mon + 1, result2_fixed.tm_mday,
           result2_fixed.tm_hour, result2_fixed.tm_min, result2_fixed.tm_sec);
    printf("    Hour difference: %d hours\n", hour_diff);
  }
  
  printf("\n");
}

void test_fastkst_localtime(time_t test_time, const char *description)
{
  struct tm result;
  int ret;
  
  memset(&result, 0, sizeof(result));
  ret = fastkst_localtime(test_time, &result);
  
  if (ret == 1) {
    printf("[SUCCESS] %s\n", description);
    printf("  Time: %lld (0x%llx)\n", (long long)test_time, (unsigned long long)test_time);
    printf("  Date: %04d-%02d-%02d %02d:%02d:%02d %s\n",
           result.tm_year + 1900, result.tm_mon + 1, result.tm_mday,
           result.tm_hour, result.tm_min, result.tm_sec, result.tm_zone);
    printf("  Day of week: %d, Day of year: %d\n\n", result.tm_wday, result.tm_yday);
  } else {
    printf("[FAIL] %s\n", description);
    printf("  Time: %lld, errno: %d\n\n", (long long)test_time, errno);
  }
}

int main(void)
{
  time_t test_times[] = {
    0,                      // Unix Epoch
    1735657200,             // 2026-01-01 00:00:00 KST
    2147451247,             // 2038-01-19 03:14:07 KST
    2147451248LL,           // 2038-01-19 03:14:08 KST
    4102412400LL,           // 2100-01-01 00:00:00 KST
    32503647600LL,          // 3000-01-01 00:00:00 KST
    -118800,                // 1969-12-31 00:00:00 KST
    -2209021200LL,          // 1900-01-01 00:00:00 KST
  };
  const char *descriptions[] = {
    "Unix Epoch (1970-01-01 00:00:00 UTC)",
    "2026-01-01 00:00:00 KST",
    "2038-01-19 03:14:07 KST (INT32_MAX)",
    "2038-01-19 03:14:08 KST (INT32_MAX+1)",
    "2100-01-01 00:00:00 KST",
    "3000-01-01 00:00:00 KST",
    "1969-12-31 00:00:00 KST",
    "1900-01-01 00:00:00 KST",
  };
  int num_tests = sizeof(test_times) / sizeof(test_times[0]);
  int i;
  
  printf("=== FASTKST_LOCALTIME 64-bit Test ===\n\n");
  
  // 기본 기능 테스트
  for (i = 0; i < num_tests; i++) {
    test_fastkst_localtime(test_times[i], descriptions[i]);
  }
  
  // NULL 포인터 테스트
  printf("*** NULL TEST ***\n");
  if (fastkst_localtime(0, NULL) == 0) {
    printf("[SUCCESS] NULL pointer correctly rejected (errno: %d)\n\n", errno);
  }
  
  // 성능 비교 테스트
  benchmark_localtime_vs_fastkst(1000000);  // 100만 회 반복
  
  // 스레드 안전성 테스트
  printf("\n=== FASTKST_LOCALTIME_SAFE Thread Safety Test ===\n\n");
  printf("Configuration:\n");
  printf("  - Number of threads: %d\n", NUM_THREADS);
  printf("  - Iterations per thread: %d\n", ITERATIONS_PER_THREAD);
  printf("  - Total operations: %d\n\n", NUM_THREADS * ITERATIONS_PER_THREAD);
  
  pthread_t threads[NUM_THREADS];
  thread_data_t thread_data[NUM_THREADS];
  int t;
  int total_success = 0;
  int total_fail = 0;
  
  // 각 테스트 시간에 대해 멀티스레드 테스트 수행
  for (i = 0; i < num_tests; i++) {
    printf("Testing time_t: %lld\n", (long long)test_times[i]);
    
    // 모든 스레드 초기화
    for (t = 0; t < NUM_THREADS; t++) {
      thread_data[t].test_time = test_times[i];
      thread_data[t].thread_id = t;
      thread_data[t].success_count = 0;
      thread_data[t].fail_count = 0;
    }
    
    // 스레드 생성
    for (t = 0; t < NUM_THREADS; t++) {
      if (pthread_create(&threads[t], NULL, thread_test_func, &thread_data[t]) != 0) {
        fprintf(stderr, "Error creating thread %d\n", t);
        return 1;
      }
    }
    
    // 모든 스레드 완료 대기
    for (t = 0; t < NUM_THREADS; t++) {
      pthread_join(threads[t], NULL);
    }
    
    // 결과 집계
    int test_success = 0;
    int test_fail = 0;
    for (t = 0; t < NUM_THREADS; t++) {
      test_success += thread_data[t].success_count;
      test_fail += thread_data[t].fail_count;
    }
    
    total_success += test_success;
    total_fail += test_fail;
    
    printf("  Success: %d, Fail: %d\n", test_success, test_fail);
    if (test_fail == 0) {
      printf("  [PASS] Thread safety test passed\n\n");
    } else {
      printf("  [FAIL] Thread safety test failed\n\n");
    }
  }
  
  // NULL 포인터 테스트 (fastkst_localtime_safe)
  printf("*** NULL TEST (fastkst_localtime_safe) ***\n");
  if (fastkst_localtime_safe(0, NULL, NULL) == 0) {
    printf("[SUCCESS] NULL pointer correctly rejected (errno: %d)\n\n", errno);
  }
  
  // 전체 결과 출력
  printf("=== Final Thread Safety Results ===\n");
  printf("Total Success: %d\n", total_success);
  printf("Total Fail: %d\n", total_fail);
  printf("Success Rate: %.2f%%\n\n", 
         (double)total_success / (total_success + total_fail) * 100.0);
  
  if (total_fail == 0) {
    printf("[PASS] All thread safety tests passed!\n");
  } else {
    printf("[FAIL] Some thread safety tests failed!\n");
    return 1;
  }
  
  return 0;
}
#endif
