
`/dev/ksancov`는 macOS XNU 커널의 SanitizerCoverage 기반 커버리지 측정 디바이스입니다. 이 도구를 사용하여 커널 코드의 실행 경로를 추적하고 분석할 수 있습니다.

## 요구사항

- macOS 개발 환경
- XNU 커널 소스 코드
- `CONFIG_KSANCOV` 옵션이 활성화된 커널 빌드


## 기본 개념

### 커버리지 모드

1. **TRACE 모드**: 실행된 프로그램 카운터(PC) 주소들을 순서대로 기록
2. **COUNTERS 모드**: 각 엣지(분기점)의 실행 횟수를 카운트
3. **STKSIZE 모드**: PC 주소와 함께 스택 크기 정보도 기록

### 주요 구성 요소

- **디바이스 파일**: `/dev/ksancov`
- **공유 메모리**: 커널과 사용자 공간 간 데이터 교환
- **ioctl 인터페이스**: 디바이스 제어
- **atomic 연산**: 스레드 안전한 상태 관리

## API 레퍼런스

### 디바이스 열기/닫기
```c
int ksancov_open(void);  // /dev/ksancov 열기
int close(int fd);       // 디바이스 닫기
```

### 모드 설정
```c
// TRACE 모드 설정 (max_entries: 최대 기록할 PC 수)
int ksancov_mode_trace(int fd, size_t max_entries);

// COUNTERS 모드 설정
int ksancov_mode_counters(int fd);

// STKSIZE 모드 설정
int ksancov_mode_stksize(int fd, size_t max_entries);
```

### 버퍼 매핑
```c
// 커버리지 데이터 버퍼 매핑
int ksancov_map(int fd, uintptr_t *buf, size_t *sz);

// 엣지-주소 매핑 버퍼 매핑
int ksancov_map_edgemap(int fd, uintptr_t *buf, size_t *sz);
```

### 스레드 관리
```c
// 현재 스레드를 커버리지 수집 대상으로 연결
int ksancov_thread_self(int fd);
```

### 커버리지 제어
```c
// 커버리지 수집 시작
int ksancov_start(void *buf);

// 커버리지 수집 중지
int ksancov_stop(void *buf);

// 커버리지 데이터 초기화
int ksancov_reset(void *buf);
```

### 데이터 읽기
```c
// 총 엣지 수 조회
size_t ksancov_nedges(int fd);

// TRACE 모드에서 기록된 엔트리 수
size_t ksancov_trace_head(ksancov_trace_t *trace);

// TRACE 모드에서 i번째 PC 주소 읽기
uintptr_t ksancov_trace_entry(ksancov_trace_t *trace, size_t i);

// 엣지 인덱스에 해당하는 PC 주소
uintptr_t ksancov_edge_addr(ksancov_edgemap_t *kemap, size_t idx);
```

## 사용 패턴


### On-Demand 모드

특정 커널 모듈(kext)에 대해서만 커버리지 수집을 활성화할 수 있습니다:

```c
// 특정 번들의 커버리지 활성화
int ksancov_on_demand_set_gate(int fd, const char *bundle, uint64_t value);

// 특정 번들의 커버리지 상태 확인
int ksancov_on_demand_get_gate(int fd, const char *bundle, uint64_t *gate);

// 특정 번들의 가드 범위 조회
int ksancov_on_demand_get_range(int fd, const char *bundle, uint32_t *start, uint32_t *stop);
```

## 에러 처리

일반적인 에러 코드:
- `EINVAL`: 잘못된 인자
- `ENOMEM`: 메모리 부족
- `EBUSY`: 디바이스가 이미 사용 중
- `ENXIO`: 디바이스가 존재하지 않음
- `ENOTSUP`: 지원되지 않는 기능



## 관련 파일

- 커널 헤더: `san/coverage/kcov_ksancov.h`
- 사용자 헤더: `san/tools/ksancov.h`
- 커널 구현: `san/coverage/kcov_ksancov.c`
- 사용자 도구: `san/tools/ksancov.c`






# KSANCOV 커버리지 도구 사용 가이드



### 1. 환경 설정
```bash
# 저장소 클론 또는 파일 다운로드 후
cd MacOs-KSANCOV-coverage-tools

# 환경 설정 자동 실행
./setup.sh
```


### 프로젝트 구조

```
MacOs-KSANCOV-coverage-tools/
├── setup.sh                 # 환경 설정 스크립트
├── run_demo.sh              # 통합 데모 실행 스크립트
├── build_and_run.sh         # 기존 빌드/실행 스크립트
├── coverage_analyzer.py     # Python 커버리지 분석기
├── ksancov_example.c        # 고급 C 예제 프로그램
├── simple_coverage_test.c   # 간단한 C 테스트 프로그램
├── KSANCOV_README.md        # 기술 문서
├── USAGE_GUIDE.md          # 이 사용 가이드
└── QUICK_START.md          # 빠른 시작 가이드
```

### 도구별 사용법

### 1. setup.sh - 환경 설정

자동으로 환경을 설정하고 필요한 도구들을 빌드합니다.

```bash
./setup.sh
```

**기능:**
- macOS 버전 및 Xcode Command Line Tools 확인
- `/dev/ksancov` 디바이스 확인
- 커널 설정 확인
- ksancov 도구 빌드
- 테스트 프로그램 컴파일
- 권한 설정

### 2. run_demo.sh - 통합 데모

모든 기능을 통합적으로 테스트할 수 있는 스크립트입니다.

```bash
# 전체 데모 실행
./run_demo.sh

# 특정 기능만 테스트
./run_demo.sh check      # 환경 확인
./run_demo.sh trace      # TRACE 모드 데모
./run_demo.sh counters   # COUNTERS 모드 데모
./run_demo.sh custom     # 커스텀 프로그램 데모
./run_demo.sh python     # Python 분석기 데모
```

### 3. coverage_analyzer.py - Python 분석기

Python으로 작성된 커버리지 분석 도구입니다.

```bash
# 환경 확인
python3 coverage_analyzer.py check

# TRACE 모드 분석
python3 coverage_analyzer.py trace [program]

# COUNTERS 모드 분석
python3 coverage_analyzer.py counters [program]

# 전체 분석
python3 coverage_analyzer.py full
```

**특징:**
- 자동 환경 확인
- 다양한 커버리지 모드 지원
- 결과 분석 및 시각화
- 에러 처리 및 로깅

### 4. ksancov_example.c - 고급 예제

C로 작성된 고급 커버리지 측정 예제입니다.

```bash
# 컴파일 (setup.sh에서 자동 수행)
gcc -o ksancov_example ksancov_example.c

# 실행
sudo ./ksancov_example              # 모든 예제 실행
sudo ./ksancov_example trace        # TRACE 모드만
sudo ./ksancov_example counters     # COUNTERS 모드만
sudo ./ksancov_example fork         # FORK 모드만
```

**포함된 예제:**
- TRACE 모드 사용법
- COUNTERS 모드 사용법
- 자식 프로세스에서 커버리지 수집
- 상세한 결과 분석

### 5. simple_coverage_test.c - 간단한 테스트

기본적인 커버리지 측정을 위한 간단한 테스트 프로그램입니다.

```bash
# 컴파일 (setup.sh에서 자동 수행)
gcc -o simple_coverage_test simple_coverage_test.c

# 실행
sudo ./simple_coverage_test
```

**기능:**
- 파일 시스템 작업
- 프로세스 정보 조회
- 메모리 할당/해제
- 시간 관련 작업
- 소켓 생성/해제

## 커버리지 모드 설명

### TRACE 모드
- 실행된 프로그램 카운터(PC) 주소들을 순서대로 기록
- 실행 경로 추적에 유용
- 메모리 사용량이 많음

### COUNTERS 모드
- 각 엣지(분기점)의 실행 횟수를 카운트
- 커버리지 통계에 유용
- 메모리 효율적

### STKSIZE 모드
- PC 주소와 함께 스택 크기 정보도 기록
- 스택 사용량 분석에 유용


## 문제 해결

### 일반적인 문제들

#### 1. `/dev/ksancov` 디바이스가 없는 경우
```bash
# 커널 설정 확인
sysctl kern.kcov.od.support_enabled

# 커널을 CONFIG_KSANCOV로 다시 빌드 필요
```

#### 2. 권한 문제
```bash
# sudo 권한으로 실행
sudo ./ksancov --trace --entries 1000

# 또는 디바이스 권한 확인
ls -la /dev/ksancov
```

#### 3. 컴파일 오류
```bash
# Xcode Command Line Tools 설치
xcode-select --install

# 또는 다시 설치
./setup.sh
```

#### 4. ksancov 도구가 없는 경우
```bash
# XNU 소스에서 수동 빌드
cd /path/to/xnu/xnu/san/tools
gcc -o ksancov ksancov.c
cp ksancov /path/to/project/
```

### 로그 확인

각 스크립트는 상세한 로그를 출력합니다:
- `[INFO]`: 일반 정보
- `[SUCCESS]`: 성공 메시지
- `[WARNING]`: 경고 메시지
- `[ERROR]`: 오류 메시지

## 고급 사용법

### 커스텀 프로그램 분석

특정 프로그램의 커버리지를 분석하려면:

```bash
# Python 분석기 사용
python3 coverage_analyzer.py trace /path/to/program
python3 coverage_analyzer.py counters /path/to/program

# ksancov 직접 사용
sudo ./ksancov --trace --exec /path/to/program
sudo ./ksancov --counters --exec /path/to/program
```

### 결과 분석

커버리지 결과는 다음과 같이 분석할 수 있습니다:

1. **TRACE 모드**: 실행된 PC 주소 목록
2. **COUNTERS 모드**: 엣지별 실행 횟수
3. **통계**: 커버리지 비율 및 히트 수

### 성능 최적화

큰 프로그램의 커버리지를 측정할 때:

```bash
# 엔트리 수 제한
./ksancov --trace --entries 10000

# 특정 모듈만 분석
# (On-Demand 모드 사용)
```

