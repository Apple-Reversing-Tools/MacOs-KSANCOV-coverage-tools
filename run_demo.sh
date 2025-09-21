#!/bin/bash


set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 로그 함수들
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
    echo -e "${PURPLE}[STEP]${NC} $1"
}

log_demo() {
    echo -e "${CYAN}[DEMO]${NC} $1"
}

# 현재 디렉토리 저장
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 환경 확인 함수
check_environment() {
    log_step "환경 확인 중..."
    
    # ksancov 디바이스 확인
    if [ -e "/dev/ksancov" ]; then
        log_success "/dev/ksancov 디바이스 발견"
        ls -la /dev/ksancov
    else
        log_warning "/dev/ksancov 디바이스를 찾을 수 없습니다."
        return 1
    fi
    
    # ksancov 도구 확인
    if [ -x "./ksancov" ]; then
        log_success "ksancov 도구 발견"
    else
        log_warning "ksancov 도구가 없습니다. setup.sh를 먼저 실행하세요."
        return 1
    fi
    
    # 커널 설정 확인
    log_info "커널 설정:"
    for setting in kern.kcov.od.support_enabled kern.kcov.od.logging_enabled kern.kcov.od.config; do
        value=$(sysctl -n "$setting" 2>/dev/null || echo "N/A")
        echo "  $setting: $value"
    done
    
    return 0
}

# TRACE 모드 데모
demo_trace_mode() {
    log_step "TRACE 모드 데모 실행..."
    
    if ! check_environment; then
        log_error "환경 확인 실패"
        return 1
    fi
    
    log_demo "=== TRACE 모드 테스트 ==="
    log_info "getppid() 시스템 콜의 커버리지를 측정합니다..."
    
    if sudo ./ksancov --trace --entries 1000 2>&1; then
        log_success "TRACE 모드 실행 성공"
    else
        log_warning "TRACE 모드 실행 실패 (권한 문제일 수 있음)"
    fi
    
    echo
    log_demo "=== 특정 프로그램의 TRACE 모드 테스트 ==="
    
    # ls 명령어로 테스트
    if [ -x "/bin/ls" ]; then
        log_info "/bin/ls 명령어의 커버리지 측정..."
        if sudo ./ksancov --trace --entries 500 --exec /bin/ls 2>&1; then
            log_success "ls 명령어 TRACE 모드 성공"
        else
            log_warning "ls 명령어 TRACE 모드 실패"
        fi
    fi
    
    # whoami 명령어로 테스트
    if [ -x "/usr/bin/whoami" ]; then
        log_info "/usr/bin/whoami 명령어의 커버리지 측정..."
        if sudo ./ksancov --trace --entries 500 --exec /usr/bin/whoami 2>&1; then
            log_success "whoami 명령어 TRACE 모드 성공"
        else
            log_warning "whoami 명령어 TRACE 모드 실패"
        fi
    fi
}

# COUNTERS 모드 데모
demo_counters_mode() {
    log_step "COUNTERS 모드 데모 실행..."
    
    if ! check_environment; then
        log_error "환경 확인 실패"
        return 1
    fi
    
    log_demo "=== COUNTERS 모드 테스트 ==="
    log_info "엣지별 실행 횟수를 측정합니다..."
    
    if sudo ./ksancov --counters 2>&1; then
        log_success "COUNTERS 모드 실행 성공"
    else
        log_warning "COUNTERS 모드 실행 실패"
    fi
    
    echo
    log_demo "=== 특정 프로그램의 COUNTERS 모드 테스트 ==="
    
    # date 명령어로 테스트
    if [ -x "/bin/date" ]; then
        log_info "/bin/date 명령어의 커버리지 측정..."
        if sudo ./ksancov --counters --exec /bin/date 2>&1; then
            log_success "date 명령어 COUNTERS 모드 성공"
        else
            log_warning "date 명령어 COUNTERS 모드 실패"
        fi
    fi
}

# 커스텀 테스트 프로그램 데모
demo_custom_programs() {
    log_step "커스텀 테스트 프로그램 데모 실행..."
    
    log_demo "=== simple_coverage_test 실행 ==="
    if [ -x "./simple_coverage_test" ]; then
        log_info "간단한 커버리지 테스트 프로그램 실행..."
        if sudo ./simple_coverage_test 2>&1; then
            log_success "simple_coverage_test 성공"
        else
            log_warning "simple_coverage_test 실패"
            log_info "일반 사용자 권한으로 시도..."
            ./simple_coverage_test || log_error "일반 권한으로도 실행 실패"
        fi
    else
        log_warning "simple_coverage_test가 없습니다."
    fi
    
    echo
    log_demo "=== ksancov_example 실행 ==="
    if [ -x "./ksancov_example" ]; then
        log_info "고급 예제 프로그램 실행..."
        
        # TRACE 모드 예제
        log_info "TRACE 모드 예제 실행..."
        if sudo ./ksancov_example trace 2>&1; then
            log_success "ksancov_example trace 성공"
        else
            log_warning "ksancov_example trace 실패"
        fi
        
        # COUNTERS 모드 예제
        log_info "COUNTERS 모드 예제 실행..."
        if sudo ./ksancov_example counters 2>&1; then
            log_success "ksancov_example counters 성공"
        else
            log_warning "ksancov_example counters 실패"
        fi
    else
        log_warning "ksancov_example이 없습니다."
    fi
}

# Python 분석기 데모
demo_python_analyzer() {
    log_step "Python 커버리지 분석기 데모 실행..."
    
    if [ -x "./coverage_analyzer.py" ]; then
        log_demo "=== Python 커버리지 분석기 ==="
        
        # 환경 확인
        log_info "환경 확인 실행..."
        python3 ./coverage_analyzer.py check
        
        # TRACE 모드 분석
        log_info "TRACE 모드 분석 실행..."
        python3 ./coverage_analyzer.py trace
        
        # COUNTERS 모드 분석
        log_info "COUNTERS 모드 분석 실행..."
        python3 ./coverage_analyzer.py counters
    else
        log_warning "coverage_analyzer.py가 없습니다."
    fi
}

# 전체 데모 실행
run_full_demo() {
    log_step "전체 데모 실행..."
    
    echo "=========================================="
    echo "KSANCOV 커버리지 도구 전체 데모"
    echo "=========================================="
    
    # 환경 확인
    if ! check_environment; then
        log_error "환경 확인 실패. setup.sh를 먼저 실행하세요."
        return 1
    fi
    
    echo
    
    # 각 모드별 데모 실행
    demo_trace_mode
    echo
    
    demo_counters_mode
    echo
    
    demo_custom_programs
    echo
    
    demo_python_analyzer
    echo
    
    log_success "전체 데모 완료!"
}

# 사용법 출력
show_usage() {
    echo "KSANCOV 커버리지 도구 통합 데모"
    echo "==============================="
    echo
    echo "사용법: $0 [명령]"
    echo
    echo "명령:"
    echo "  check     - 환경 확인만 수행"
    echo "  trace     - TRACE 모드 데모 실행"
    echo "  counters  - COUNTERS 모드 데모 실행"
    echo "  custom    - 커스텀 테스트 프로그램 데모 실행"
    echo "  python    - Python 분석기 데모 실행"
    echo "  full      - 전체 데모 실행 (기본값)"
    echo "  help      - 이 도움말 표시"
    echo
    echo "예시:"
    echo "  $0 check        # 환경 확인"
    echo "  $0 trace        # TRACE 모드만 테스트"
    echo "  $0 full         # 모든 기능 테스트"
    echo
    echo "주의사항:"
    echo "  • 일부 기능은 sudo 권한이 필요합니다"
    echo "  • setup.sh를 먼저 실행하여 환경을 설정하세요"
    echo "  • /dev/ksancov 디바이스가 필요합니다"
}

# 메인 실행 로직
main() {
    case "${1:-full}" in
        "check")
            check_environment
            ;;
        "trace")
            demo_trace_mode
            ;;
        "counters")
            demo_counters_mode
            ;;
        "custom")
            demo_custom_programs
            ;;
        "python")
            demo_python_analyzer
            ;;
        "full")
            run_full_demo
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        *)
            log_error "알 수 없는 명령: $1"
            echo
            show_usage
            exit 1
            ;;
    esac
}

# 스크립트 실행
main "$@"
