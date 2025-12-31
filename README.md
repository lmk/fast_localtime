# fastkst_localtime

고성능 KST(한국 표준시, UTC+9) 전용 localtime 구현

## 개요

`fastkst_localtime`은 한국 표준시(KST, UTC+9) 변환에 최적화된 고성능 시간 변환 라이브러리입니다. 표준 `localtime()` 함수가 가진 성능 오버헤드를 제거하고, 64-bit 시간 값을 안전하게 처리할 수 있도록 설계되었습니다.

* glibc/time 소스를 참조하여 구현되었습니다.

## 주요 특징

### 1. 고성능
- KST 타임존 변환에 특화되어 표준 `localtime()` 함수보다 빠른 성능 제공
- Linux 커널에서 발생하는 시스템 콜, timezone 조회, mutex 락 등의 오버헤드 제거
- 직접적인 시간 계산으로 최소한의 연산만 수행

### 2. 64-bit 안전성
- 2038년 문제 해결 (32-bit time_t 제약 극복)
- **time_t 값 지원 범위**: 약 -2920년 ~ +2920년
- **struct tm의 tm_year 필드 제한**: 약 -21만년 ~ +21만년 (int 타입 제한)
- 64-bit time_t를 안전하게 처리

### 3. Thread-Safe
- `fastkst_localtime_safe()` 함수로 멀티스레드 환경 지원
- 재진입 가능한 설계
- 경쟁 상태(race condition) 없이 안전한 동시 호출

### 4. 고정 타임존
- 시스템 타임존 설정과 무관하게 항상 KST(UTC+9) 반환
- 타임존 조회 오버헤드 제거
- 예측 가능하고 일관된 동작

## API 함수

### fastkst_localtime()

```c
int fastkst_localtime(time_t t, struct tm *tp)
```

주요 시간 변환 함수입니다.

**매개변수:**
- `t`: 변환할 Unix timestamp (time_t)
- `tp`: 결과를 저장할 struct tm 포인터

**반환값:**
- `1`: 성공
- `0`: 실패 (errno 설정됨)

**지원 범위:**
- 32-bit time_t: 1901-12-13 ~ 2038-01-19 (KST)
- 64-bit time_t: 약 -2920년 ~ +2920년
- 실제 제한: struct tm의 tm_year(int) 범위로 약 -21만년 ~ +21만년

### fastkst_localtime_safe()

```c
int fastkst_localtime_safe(time_t t, struct tm *tp, int *err_code)
```

추가 검증 및 에러 처리 기능이 있는 Thread-safe 래퍼 함수입니다.

**매개변수:**
- `t`: 변환할 Unix timestamp (time_t)
- `tp`: 결과를 저장할 struct tm 포인터
- `err_code`: 에러 코드를 저장할 포인터 (NULL 가능)

**반환값:**
- `1`: 성공
- `0`: 실패 (err_code에 에러 코드 저장)

**특징:**
- struct tm 자동 초기화
- 명시적인 에러 코드 반환
- NULL 포인터 안전성 검증

## 사용 예제

### 기본 사용법

```c
#include <time.h>
#include <stdio.h>

// fastkst_localtime() 함수 선언
int fastkst_localtime(time_t t, struct tm *tp);

int main() {
    time_t now = time(NULL);
    struct tm kst_time;

    if (fastkst_localtime(now, &kst_time) == 1) {
        printf("현재 KST 시간: %04d-%02d-%02d %02d:%02d:%02d\n",
               kst_time.tm_year + 1900,
               kst_time.tm_mon + 1,
               kst_time.tm_mday,
               kst_time.tm_hour,
               kst_time.tm_min,
               kst_time.tm_sec);
    }

    return 0;
}
```

### Thread-Safe 사용법

```c
#include <time.h>
#include <stdio.h>

int fastkst_localtime_safe(time_t t, struct tm *tp, int *err_code);

int main() {
    time_t now = time(NULL);
    struct tm kst_time;
    int error_code;

    if (fastkst_localtime_safe(now, &kst_time, &error_code) == 1) {
        printf("KST: %04d-%02d-%02d %02d:%02d:%02d %s\n",
               kst_time.tm_year + 1900,
               kst_time.tm_mon + 1,
               kst_time.tm_mday,
               kst_time.tm_hour,
               kst_time.tm_min,
               kst_time.tm_sec,
               kst_time.tm_zone);
    } else {
        fprintf(stderr, "Error: %d\n", error_code);
    }

    return 0;
}
```

## 빌드 및 테스트

### 라이브러리로 사용

프로젝트에 `fastkst_localtime.c`를 포함하여 컴파일합니다:

```bash
gcc -c fastkst_localtime.c -o fastkst_localtime.o
gcc your_program.c fastkst_localtime.o -o your_program
```

### 테스트 프로그램 빌드

테스트 코드를 포함하여 빌드하려면:

```bash
gcc -DTEST_FASTKST_LOCALTIME -o fastkst_localtime fastkst_localtime.c -lpthread
./fastkst_localtime
```

### 테스트 내용

테스트 프로그램은 다음을 검증합니다:

1. **기본 기능 테스트**
   - Unix Epoch (1970-01-01)
   - 2038년 문제 (INT32_MAX)
   - 미래 시간 (2100년, 3000년)
   - 과거 시간 (1900년, 1969년)

2. **성능 벤치마크**
   - `localtime()` vs `fastkst_localtime()` 성능 비교
   - 100만회 반복 호출 측정
   - 속도 향상률 계산 및 출력

3. **Thread Safety 테스트**
   - 10개 스레드 동시 실행
   - 스레드당 1000회 반복
   - 총 10,000회 동시 호출 검증

4. **NULL 포인터 테스트**
   - NULL 입력 처리 검증
   - 적절한 에러 코드 반환 확인

## 성능

`fastkst_localtime()`은 표준 `localtime()` 함수보다 일반적으로 빠른 성능을 보여줍니다:

- 타임존 조회 오버헤드 제거
- 시스템 콜 최소화
- mutex 락 제거
- 캐시 친화적인 메모리 접근 패턴

실제 성능은 시스템 환경에 따라 다를 수 있으며, 테스트 프로그램을 통해 확인할 수 있습니다.

## 제한 사항

1. **KST 전용**: 이 라이브러리는 오직 KST(UTC+9) 타임존만 지원합니다. 다른 타임존이 필요한 경우 표준 `localtime()` 함수를 사용하세요.

환경 검증

```bash
$ date
2025. 12. 31. (수) 12:52:45 KST
$ timedatectl
               Local time: 수 2025-12-31 12:52:54 KST
           Universal time: 수 2025-12-31 03:52:54 UTC
                 RTC time: 수 2025-12-31 03:52:54
                Time zone: Asia/Seoul (KST, +0900)
System clock synchronized: yes
              NTP service: active
          RTC in local TZ: no
$ ls -l /etc/localtime
lrwxrwxrwx. 1 root root 32  8월 25  2023 /etc/localtime -> ../usr/share/zoneinfo/Asia/Seoul
$ echo $TZ
$
```

1. **DST 미지원**: 일광 절약 시간제(Daylight Saving Time)를 지원하지 않습니다. `tm_isdst`는 항상 0입니다.

2. **tm_year 범위**: struct tm의 `tm_year` 필드가 int 타입이므로, 표현 가능한 연도 범위는 약 -21만년 ~ +21만년입니다.

## 에러 처리

함수 실패 시 다음 에러 코드가 설정됩니다:

- `EINVAL`: 잘못된 인자 (NULL 포인터)
- `EOVERFLOW`: 연도가 struct tm의 범위를 초과

## 라이선스

Copyright (c) 2025 lmk (newtypez@gmail.com)

## 작성자

- **저자**: lmk (newtypez@gmail.com)
- **버전**: 0.1
- **날짜**: 2025-12-31

## 기여

버그 리포트나 개선 제안은 이슈 트래커를 통해 제출해 주세요.
