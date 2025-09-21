/*
 * 간단한 커널 커버리지 측정 예제
 * 기본적인 시스템 콜들을 실행하여 커버리지를 수집합니다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

// ksancov 헤더 파일의 내용을 포함
#include <stdint.h>
#include <stdatomic.h>
#include <sys/ioccom.h>
#include <strings.h>
#include <assert.h>

#define KSANCOV_PATH "/dev/ksancov"

/* ioctl 명령들 */
#define KSANCOV_IOC_TRACE        _IOW('K', 1, size_t)
#define KSANCOV_IOC_COUNTERS     _IO('K', 2)
#define KSANCOV_IOC_MAP          _IOWR('K', 8, struct ksancov_buf_desc)
#define KSANCOV_IOC_MAP_EDGEMAP  _IOWR('K', 9, struct ksancov_buf_desc)
#define KSANCOV_IOC_START        _IOW('K', 10, uintptr_t)
#define KSANCOV_IOC_NEDGES       _IOR('K', 50, size_t)

/* 매직 넘버들 */
#define KSANCOV_TRACE_MAGIC     (uint32_t)0x5AD17F5BU
#define KSANCOV_COUNTERS_MAGIC  (uint32_t)0x5AD27F6BU
#define KSANCOV_EDGEMAP_MAGIC   (uint32_t)0x5AD37F7BU

/* 구조체들 */
struct ksancov_buf_desc {
    uintptr_t ptr;
    size_t sz;
};

typedef struct ksancov_header {
    uint32_t         kh_magic;
    _Atomic uint32_t kh_enabled;
} ksancov_header_t;

typedef struct ksancov_trace {
    ksancov_header_t kt_hdr;
    uint32_t         kt_maxent;
    _Atomic uint32_t kt_head;
    uint64_t         kt_entries[];
} ksancov_trace_t;

typedef struct ksancov_counters {
    ksancov_header_t kc_hdr;
    uint32_t         kc_nedges;
    uint8_t          kc_hits[];
} ksancov_counters_t;

typedef struct ksancov_edgemap {
    uint32_t  ke_magic;
    uint32_t  ke_nedges;
    uintptr_t ke_addrs[];
} ksancov_edgemap_t;

/* 헬퍼 함수들 */
static int ksancov_open(void) {
    return open(KSANCOV_PATH, 0);
}

static int ksancov_map(int fd, uintptr_t *buf, size_t *sz) {
    struct ksancov_buf_desc mc = {0};
    int ret = ioctl(fd, KSANCOV_IOC_MAP, &mc);
    if (ret == -1) return errno;
    
    *buf = mc.ptr;
    if (sz) *sz = mc.sz;
    return 0;
}

static int ksancov_map_edgemap(int fd, uintptr_t *buf, size_t *sz) {
    struct ksancov_buf_desc mc = {0};
    int ret = ioctl(fd, KSANCOV_IOC_MAP_EDGEMAP, &mc);
    if (ret == -1) return errno;
    
    *buf = mc.ptr;
    if (sz) *sz = mc.sz;
    return 0;
}

static int ksancov_mode_trace(int fd, size_t entries) {
    return ioctl(fd, KSANCOV_IOC_TRACE, &entries) == -1 ? errno : 0;
}

static int ksancov_mode_counters(int fd) {
    return ioctl(fd, KSANCOV_IOC_COUNTERS) == -1 ? errno : 0;
}

static int ksancov_thread_self(int fd) {
    uintptr_t th = 0;
    return ioctl(fd, KSANCOV_IOC_START, &th) == -1 ? errno : 0;
}

static void ksancov_start(void *buf) {
    ksancov_header_t *hdr = (ksancov_header_t *)buf;
    atomic_store_explicit(&hdr->kh_enabled, 1, memory_order_relaxed);
}

static void ksancov_stop(void *buf) {
    ksancov_header_t *hdr = (ksancov_header_t *)buf;
    atomic_store_explicit(&hdr->kh_enabled, 0, memory_order_relaxed);
}

static void ksancov_reset_trace(ksancov_trace_t *trace) {
    atomic_store_explicit(&trace->kt_head, 0, memory_order_relaxed);
}

static void ksancov_reset_counters(ksancov_counters_t *counters) {
    bzero(counters->kc_hits, counters->kc_nedges);
}

/* 테스트용 시스템 콜들을 실행하는 함수 */
static void perform_test_operations(void) {
    printf("=== 커버리지 측정을 위한 테스트 작업 시작 ===\n");
    
    // 1. 파일 시스템 관련 시스템 콜들
    printf("1. 파일 시스템 작업...\n");
    int fd = open("/tmp/kcov_test.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "Hello, kernel coverage!\n", 24);
        fsync(fd);
        close(fd);
        unlink("/tmp/kcov_test.txt");
    }
    
    // 2. 프로세스 관련 시스템 콜들
    printf("2. 프로세스 정보 조회...\n");
    pid_t pid = getpid();
    pid_t ppid = getppid();
    uid_t uid = getuid();
    gid_t gid = getgid();
    printf("   PID: %d, PPID: %d, UID: %d, GID: %d\n", pid, ppid, uid, gid);
    
    // 3. 메모리 관련 시스템 콜들
    printf("3. 메모리 할당/해제...\n");
    void *mem = malloc(4096);
    if (mem) {
        memset(mem, 0x42, 4096);
        free(mem);
    }
    
    // 4. 시간 관련 시스템 콜들
    printf("4. 시간 관련 작업...\n");
    time_t t = time(NULL);
    printf("   현재 시간: %ld\n", t);
    
    // 5. 간단한 네트워크 소켓 작업
    printf("5. 소켓 생성/해제...\n");
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        close(sock);
    }
    
    printf("=== 테스트 작업 완료 ===\n");
}

/* TRACE 모드로 커버리지 측정 */
static void test_trace_mode(void) {
    printf("\n========== TRACE 모드 테스트 ==========\n");
    
    int fd = ksancov_open();
    if (fd < 0) {
        perror("ksancov_open 실패");
        return;
    }
    printf("ksancov 디바이스 열기 성공 (fd: %d)\n", fd);
    
    // TRACE 모드 설정 (64K 엔트리)
    size_t max_entries = 64 * 1024;
    int ret = ksancov_mode_trace(fd, max_entries);
    if (ret) {
        printf("TRACE 모드 설정 실패: %s\n", strerror(ret));
        close(fd);
        return;
    }
    
    // 커버리지 버퍼 매핑
    uintptr_t buf_addr;
    size_t buf_size;
    ret = ksancov_map(fd, &buf_addr, &buf_size);
    if (ret) {
        printf("버퍼 매핑 실패: %s\n", strerror(ret));
        close(fd);
        return;
    }
    
    ksancov_trace_t *trace = (ksancov_trace_t *)buf_addr;
    printf("매핑 성공: 0x%lx + %zu 바이트, 최대 엔트리: %u\n", 
           buf_addr, buf_size, trace->kt_maxent);
    
    // 에지맵 매핑
    uintptr_t edgemap_addr;
    ret = ksancov_map_edgemap(fd, &edgemap_addr, NULL);
    if (ret) {
        printf("에지맵 매핑 실패: %s\n", strerror(ret));
    } else {
        ksancov_edgemap_t *edgemap = (ksancov_edgemap_t *)edgemap_addr;
        printf("에지맵 매핑 성공, 총 에지 수: %u\n", edgemap->ke_nedges);
    }
    
    // 현재 스레드에 커버리지 연결
    ret = ksancov_thread_self(fd);
    if (ret) {
        printf("스레드 설정 실패: %s\n", strerror(ret));
        close(fd);
        return;
    }
    
    // 커버리지 측정 시작
    ksancov_reset_trace(trace);
    ksancov_start(trace);
    printf("커버리지 측정 시작...\n");
    
    // 테스트 작업 실행
    perform_test_operations();
    
    // 커버리지 측정 중지
    ksancov_stop(trace);
    printf("커버리지 측정 중지\n");
    
    // 결과 출력
    uint32_t head = atomic_load_explicit(&trace->kt_head, memory_order_acquire);
    head = head < trace->kt_maxent ? head : trace->kt_maxent;
    
    printf("\n=== TRACE 모드 결과 ===\n");
    printf("수집된 PC 엔트리 수: %u\n", head);
    
    if (head > 0) {
        printf("처음 10개 PC 주소:\n");
        for (uint32_t i = 0; i < head && i < 10; i++) {
            printf("  [%u] 0x%lx\n", i, (uintptr_t)trace->kt_entries[i]);
        }
        
        if (head > 10) {
            printf("  ... (총 %u개 더)\n", head - 10);
        }
    } else {
        printf("수집된 커버리지 데이터가 없습니다.\n");
    }
    
    close(fd);
}

/* COUNTERS 모드로 커버리지 측정 */
static void test_counters_mode(void) {
    printf("\n========== COUNTERS 모드 테스트 ==========\n");
    
    int fd = ksancov_open();
    if (fd < 0) {
        perror("ksancov_open 실패");
        return;
    }
    
    // COUNTERS 모드 설정
    int ret = ksancov_mode_counters(fd);
    if (ret) {
        printf("COUNTERS 모드 설정 실패: %s\n", strerror(ret));
        close(fd);
        return;
    }
    
    // 커버리지 버퍼 매핑
    uintptr_t buf_addr;
    size_t buf_size;
    ret = ksancov_map(fd, &buf_addr, &buf_size);
    if (ret) {
        printf("버퍼 매핑 실패: %s\n", strerror(ret));
        close(fd);
        return;
    }
    
    ksancov_counters_t *counters = (ksancov_counters_t *)buf_addr;
    printf("매핑 성공: 0x%lx + %zu 바이트, 총 에지 수: %u\n", 
           buf_addr, buf_size, counters->kc_nedges);
    
    // 에지맵 매핑
    uintptr_t edgemap_addr;
    ret = ksancov_map_edgemap(fd, &edgemap_addr, NULL);
    ksancov_edgemap_t *edgemap = NULL;
    if (ret == 0) {
        edgemap = (ksancov_edgemap_t *)edgemap_addr;
    }
    
    // 현재 스레드에 커버리지 연결
    ret = ksancov_thread_self(fd);
    if (ret) {
        printf("스레드 설정 실패: %s\n", strerror(ret));
        close(fd);
        return;
    }
    
    // 커버리지 측정 시작
    ksancov_reset_counters(counters);
    ksancov_start(counters);
    printf("커버리지 측정 시작...\n");
    
    // 테스트 작업 실행
    perform_test_operations();
    
    // 커버리지 측정 중지
    ksancov_stop(counters);
    printf("커버리지 측정 중지\n");
    
    // 결과 분석 및 출력
    printf("\n=== COUNTERS 모드 결과 ===\n");
    uint32_t hit_edges = 0;
    uint32_t total_hits = 0;
    
    for (uint32_t i = 0; i < counters->kc_nedges; i++) {
        if (counters->kc_hits[i] > 0) {
            hit_edges++;
            total_hits += counters->kc_hits[i];
        }
    }
    
    printf("총 에지 수: %u\n", counters->kc_nedges);
    printf("히트된 에지 수: %u (%.2f%%)\n", 
           hit_edges, (float)hit_edges / counters->kc_nedges * 100.0f);
    printf("총 히트 수: %u\n", total_hits);
    
    if (hit_edges > 0) {
        printf("\n히트된 에지들 (처음 10개):\n");
        uint32_t shown = 0;
        for (uint32_t i = 0; i < counters->kc_nedges && shown < 10; i++) {
            if (counters->kc_hits[i] > 0) {
                uintptr_t addr = edgemap ? edgemap->ke_addrs[i] : 0;
                printf("  에지 %u: %u회 히트", i, counters->kc_hits[i]);
                if (addr) printf(" (주소: 0x%lx)", addr);
                printf("\n");
                shown++;
            }
        }
    }
    
    close(fd);
}

int main(int argc, char *argv[]) {
    printf("XNU 커널 커버리지 측정 데모\n");
    printf("============================\n");
    
    // 커버리지 디바이스가 존재하는지 확인
    if (access(KSANCOV_PATH, F_OK) != 0) {
        printf("오류: %s 디바이스를 찾을 수 없습니다.\n", KSANCOV_PATH);
        printf("커널이 CONFIG_KCOV로 빌드되었고 ksancov가 활성화되어 있는지 확인하세요.\n");
        return 1;
    }
    
    printf("ksancov 디바이스 발견: %s\n", KSANCOV_PATH);
    
    // TRACE 모드 테스트
    test_trace_mode();
    
    // COUNTERS 모드 테스트  
    test_counters_mode();
    
    printf("\n============================\n");
    printf("커버리지 측정 완료!\n");
    
    return 0;
}
