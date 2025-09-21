/*
 * KSANCOV 사용 예제
 * 
 * 이 예제는 /dev/ksancov 디바이스를 사용하여 커널 커버리지를 측정하는 방법을 보여줍니다.
 * 
 * 컴파일: gcc -o ksancov_example ksancov_example.c -std=c99
 * 실행: sudo ./ksancov_example
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/wait.h>

/* ksancov 헤더 파일에서 필요한 정의들 */
#define KSANCOV_PATH "/dev/ksancov"

/* ioctl 명령어들 */
#define KSANCOV_IOC_TRACE        _IOW('K', 1, size_t)
#define KSANCOV_IOC_COUNTERS     _IO('K', 2)
#define KSANCOV_IOC_STKSIZE      _IOW('K', 3, size_t)
#define KSANCOV_IOC_MAP          _IOWR('K', 8, struct ksancov_buf_desc)
#define KSANCOV_IOC_MAP_EDGEMAP  _IOWR('K', 9, struct ksancov_buf_desc)
#define KSANCOV_IOC_START        _IOW('K', 10, uintptr_t)
#define KSANCOV_IOC_NEDGES       _IOR('K', 50, size_t)

/* 매직 넘버들 */
#define KSANCOV_TRACE_MAGIC     (uint32_t)0x5AD17F5BU
#define KSANCOV_COUNTERS_MAGIC  (uint32_t)0x5AD27F6BU
#define KSANCOV_EDGEMAP_MAGIC   (uint32_t)0x5AD37F7BU
#define KSANCOV_STKSIZE_MAGIC   (uint32_t)0x5AD47F8BU

/* 커버리지 모드 */
typedef enum {
    KS_MODE_NONE,
    KS_MODE_TRACE,
    KS_MODE_COUNTERS,
    KS_MODE_STKSIZE,
    KS_MODE_MAX
} ksancov_mode_t;

/* 버퍼 설명자 */
struct ksancov_buf_desc {
    uintptr_t ptr;
    size_t sz;
};

/* 공통 헤더 */
typedef struct ksancov_header {
    uint32_t         kh_magic;
    _Atomic uint32_t kh_enabled;
} ksancov_header_t;

/* TRACE 모드 구조체 */
typedef struct ksancov_trace {
    ksancov_header_t kt_hdr;
    uint32_t         kt_maxent;
    _Atomic uint32_t kt_head;
    uint64_t         kt_entries[];
} ksancov_trace_t;

/* COUNTERS 모드 구조체 */
typedef struct ksancov_counters {
    ksancov_header_t kc_hdr;
    uint32_t         kc_nedges;
    uint8_t          kc_hits[];
} ksancov_counters_t;

/* 엣지 매핑 구조체 */
typedef struct ksancov_edgemap {
    uint32_t  ke_magic;
    uint32_t  ke_nedges;
    uintptr_t ke_addrs[];
} ksancov_edgemap_t;

/* 헬퍼 함수들 */
static int ksancov_open(void) {
    return open(KSANCOV_PATH, O_RDWR);
}

static int ksancov_map(int fd, uintptr_t *buf, size_t *sz) {
    struct ksancov_buf_desc mc = {0};
    int ret = ioctl(fd, KSANCOV_IOC_MAP, &mc);
    if (ret == -1) {
        return errno;
    }
    *buf = mc.ptr;
    if (sz) {
        *sz = mc.sz;
    }
    return 0;
}

static int ksancov_map_edgemap(int fd, uintptr_t *buf, size_t *sz) {
    struct ksancov_buf_desc mc = {0};
    int ret = ioctl(fd, KSANCOV_IOC_MAP_EDGEMAP, &mc);
    if (ret == -1) {
        return errno;
    }
    *buf = mc.ptr;
    if (sz) {
        *sz = mc.sz;
    }
    return 0;
}

static size_t ksancov_nedges(int fd) {
    size_t nedges;
    int ret = ioctl(fd, KSANCOV_IOC_NEDGES, &nedges);
    if (ret == -1) {
        return SIZE_MAX;
    }
    return nedges;
}

static int ksancov_mode_trace(int fd, size_t entries) {
    int ret = ioctl(fd, KSANCOV_IOC_TRACE, &entries);
    return (ret == -1) ? errno : 0;
}

static int ksancov_mode_counters(int fd) {
    int ret = ioctl(fd, KSANCOV_IOC_COUNTERS);
    return (ret == -1) ? errno : 0;
}

static int ksancov_thread_self(int fd) {
    uintptr_t th = 0;
    int ret = ioctl(fd, KSANCOV_IOC_START, &th);
    return (ret == -1) ? errno : 0;
}

static int ksancov_start(void *buf) {
    ksancov_header_t *hdr = (ksancov_header_t *)buf;
    atomic_store_explicit(&hdr->kh_enabled, 1, memory_order_relaxed);
    return 0;
}

static int ksancov_stop(void *buf) {
    ksancov_header_t *hdr = (ksancov_header_t *)buf;
    atomic_store_explicit(&hdr->kh_enabled, 0, memory_order_relaxed);
    return 0;
}

static size_t ksancov_trace_head(ksancov_trace_t *trace) {
    size_t maxent = trace->kt_maxent;
    size_t head = atomic_load_explicit(&trace->kt_head, memory_order_acquire);
    return head < maxent ? head : maxent;
}

static uintptr_t ksancov_trace_entry(ksancov_trace_t *trace, size_t i) {
    if (i >= trace->kt_head) {
        return 0;
    }
    return trace->kt_entries[i];
}

static uintptr_t ksancov_edge_addr(ksancov_edgemap_t *kemap, size_t idx) {
    if (idx >= kemap->ke_nedges) {
        return 0;
    }
    return kemap->ke_addrs[idx];
}

/* 예제 1: TRACE 모드 사용 */
static int example_trace_mode(void) {
    printf("\n=== TRACE 모드 예제 ===\n");
    
    int fd = ksancov_open();
    if (fd < 0) {
        perror("ksancov_open");
        return errno;
    }
    
    printf("디바이스 열기 성공: fd=%d\n", fd);
    
    // TRACE 모드 설정
    size_t max_entries = 10000;
    int ret = ksancov_mode_trace(fd, max_entries);
    if (ret != 0) {
        perror("ksancov_mode_trace");
        close(fd);
        return ret;
    }
    printf("TRACE 모드 설정 완료 (최대 %zu 엔트리)\n", max_entries);
    
    // 버퍼 매핑
    uintptr_t buf;
    size_t sz;
    ret = ksancov_map(fd, &buf, &sz);
    if (ret != 0) {
        perror("ksancov_map");
        close(fd);
        return ret;
    }
    printf("버퍼 매핑 완료: buf=%p, size=%zu\n", (void*)buf, sz);
    
    // 현재 스레드 연결
    ret = ksancov_thread_self(fd);
    if (ret != 0) {
        perror("ksancov_thread_self");
        close(fd);
        return ret;
    }
    printf("스레드 연결 완료\n");
    
    // 커버리지 수집 시작
    ksancov_start((void*)buf);
    printf("커버리지 수집 시작\n");
    
    // 측정할 코드 실행
    printf("측정 대상 코드 실행 중...\n");
    for (int i = 0; i < 1000; i++) {
        // 간단한 계산 작업
        volatile int sum = 0;
        for (int j = 0; j < 100; j++) {
            sum += j * i;
        }
        if (i % 100 == 0) {
            printf("진행률: %d%%\n", i / 10);
        }
    }
    
    // 커버리지 수집 중지
    ksancov_stop((void*)buf);
    printf("커버리지 수집 중지\n");
    
    // 결과 분석
    ksancov_trace_t *trace = (ksancov_trace_t *)buf;
    size_t head = ksancov_trace_head(trace);
    printf("수집된 PC 엔트리 수: %zu\n", head);
    
    // 처음 10개 PC 출력
    printf("처음 10개 PC 주소:\n");
    for (size_t i = 0; i < head && i < 10; i++) {
        uintptr_t pc = ksancov_trace_entry(trace, i);
        printf("  [%zu] 0x%lx\n", i, pc);
    }
    
    close(fd);
    return 0;
}

/* 예제 2: COUNTERS 모드 사용 */
static int example_counters_mode(void) {
    printf("\n=== COUNTERS 모드 예제 ===\n");
    
    int fd = ksancov_open();
    if (fd < 0) {
        perror("ksancov_open");
        return errno;
    }
    
    // 총 엣지 수 조회
    size_t nedges = ksancov_nedges(fd);
    if (nedges == SIZE_MAX) {
        perror("ksancov_nedges");
        close(fd);
        return errno;
    }
    printf("총 엣지 수: %zu\n", nedges);
    
    // COUNTERS 모드 설정
    int ret = ksancov_mode_counters(fd);
    if (ret != 0) {
        perror("ksancov_mode_counters");
        close(fd);
        return ret;
    }
    printf("COUNTERS 모드 설정 완료\n");
    
    // 버퍼 매핑
    uintptr_t buf;
    size_t sz;
    ret = ksancov_map(fd, &buf, &sz);
    if (ret != 0) {
        perror("ksancov_map");
        close(fd);
        return ret;
    }
    printf("버퍼 매핑 완료: buf=%p, size=%zu\n", (void*)buf, sz);
    
    // 엣지 매핑 매핑
    uintptr_t edgemap_buf;
    size_t edgemap_sz;
    ret = ksancov_map_edgemap(fd, &edgemap_buf, &edgemap_sz);
    if (ret != 0) {
        perror("ksancov_map_edgemap");
        close(fd);
        return ret;
    }
    printf("엣지 매핑 매핑 완료: buf=%p, size=%zu\n", (void*)edgemap_buf, edgemap_sz);
    
    // 현재 스레드 연결
    ret = ksancov_thread_self(fd);
    if (ret != 0) {
        perror("ksancov_thread_self");
        close(fd);
        return ret;
    }
    
    // 커버리지 수집 시작
    ksancov_start((void*)buf);
    printf("커버리지 수집 시작\n");
    
    // 측정할 코드 실행
    printf("측정 대상 코드 실행 중...\n");
    for (int i = 0; i < 1000; i++) {
        // 분기가 있는 코드
        if (i % 2 == 0) {
            volatile int even_sum = 0;
            for (int j = 0; j < 50; j++) {
                even_sum += j;
            }
        } else {
            volatile int odd_sum = 0;
            for (int j = 0; j < 50; j++) {
                odd_sum += j * 2;
            }
        }
    }
    
    // 커버리지 수집 중지
    ksancov_stop((void*)buf);
    printf("커버리지 수집 중지\n");
    
    // 결과 분석
    ksancov_counters_t *counters = (ksancov_counters_t *)buf;
    ksancov_edgemap_t *edgemap = (ksancov_edgemap_t *)edgemap_buf;
    
    printf("엣지별 실행 횟수 (처음 20개):\n");
    for (size_t i = 0; i < counters->kc_nedges && i < 20; i++) {
        uint8_t hits = counters->kc_hits[i];
        uintptr_t pc = ksancov_edge_addr(edgemap, i);
        if (hits > 0) {
            printf("  엣지[%zu]: PC=0x%lx, 실행횟수=%d\n", i, pc, hits);
        }
    }
    
    close(fd);
    return 0;
}

/* 예제 3: 자식 프로세스에서 커버리지 수집 */
static int example_fork_mode(void) {
    printf("\n=== FORK 모드 예제 ===\n");
    
    int fd = ksancov_open();
    if (fd < 0) {
        perror("ksancov_open");
        return errno;
    }
    
    // TRACE 모드 설정
    size_t max_entries = 5000;
    int ret = ksancov_mode_trace(fd, max_entries);
    if (ret != 0) {
        perror("ksancov_mode_trace");
        close(fd);
        return ret;
    }
    
    // 버퍼 매핑
    uintptr_t buf;
    size_t sz;
    ret = ksancov_map(fd, &buf, &sz);
    if (ret != 0) {
        perror("ksancov_map");
        close(fd);
        return ret;
    }
    
    // 자식 프로세스 생성
    pid_t pid = fork();
    if (pid == 0) {
        // 자식 프로세스
        ret = ksancov_thread_self(fd);
        if (ret != 0) {
            perror("ksancov_thread_self");
            exit(1);
        }
        
        ksancov_start((void*)buf);
        printf("자식 프로세스에서 커버리지 수집 시작\n");
        
        // 자식 프로세스에서 실행할 코드
        for (int i = 0; i < 500; i++) {
            volatile int result = i * i + i;
            if (i % 100 == 0) {
                printf("자식: %d\n", i);
            }
        }
        
        ksancov_stop((void*)buf);
        printf("자식 프로세스에서 커버리지 수집 중지\n");
        
        exit(0);
    } else {
        // 부모 프로세스
        int status;
        waitpid(pid, &status, 0);
        printf("자식 프로세스 종료 (status=%d)\n", status);
        
        // 결과 분석
        ksancov_trace_t *trace = (ksancov_trace_t *)buf;
        size_t head = ksancov_trace_head(trace);
        printf("수집된 PC 엔트리 수: %zu\n", head);
        
        printf("처음 5개 PC 주소:\n");
        for (size_t i = 0; i < head && i < 5; i++) {
            uintptr_t pc = ksancov_trace_entry(trace, i);
            printf("  [%zu] 0x%lx\n", i, pc);
        }
    }
    
    close(fd);
    return 0;
}

/* 메인 함수 */
int main(int argc, char *argv[]) {
    printf("KSANCOV 커버리지 측정 예제\n");
    printf("========================\n");
    
    if (argc > 1) {
        if (strcmp(argv[1], "trace") == 0) {
            return example_trace_mode();
        } else if (strcmp(argv[1], "counters") == 0) {
            return example_counters_mode();
        } else if (strcmp(argv[1], "fork") == 0) {
            return example_fork_mode();
        } else {
            printf("사용법: %s [trace|counters|fork]\n", argv[0]);
            return 1;
        }
    }
    
    // 모든 예제 실행
    int ret = 0;
    
    ret = example_trace_mode();
    if (ret != 0) {
        printf("TRACE 모드 예제 실패: %d\n", ret);
        return ret;
    }
    
    ret = example_counters_mode();
    if (ret != 0) {
        printf("COUNTERS 모드 예제 실패: %d\n", ret);
        return ret;
    }
    
    ret = example_fork_mode();
    if (ret != 0) {
        printf("FORK 모드 예제 실패: %d\n", ret);
        return ret;
    }
    
    printf("\n모든 예제 완료!\n");
    return 0;
}

