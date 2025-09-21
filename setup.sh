#!/bin/bash

set -e

echo "=========================================="
echo "KSANCOV 커버리지 도구 환경 설정"
echo "=========================================="

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# 현재 디렉토리 저장
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

log_info "작업 디렉토리: $SCRIPT_DIR"

# 1. 시스템 환경 확인
echo
log_info "1. 시스템 환경 확인 중..."

# macOS 버전 확인
if [[ "$OSTYPE" == "darwin"* ]]; then
    MACOS_VERSION=$(sw_vers -productVersion)
    log_success "macOS 버전: $MACOS_VERSION"
else
    log_error "이 도구는 macOS에서만 동작합니다."
    exit 1
fi

# Xcode Command Line Tools 확인
if xcode-select -p &>/dev/null; then
    XCODE_PATH=$(xcode-select -p)
    log_success "Xcode Command Line Tools: $XCODE_PATH"
else
    log_warning "Xcode Command Line Tools가 설치되지 않았습니다."
    log_info "설치를 진행합니다..."
    xcode-select --install
    log_info "설치 완료 후 다시 실행해주세요."
    exit 0
fi

# 2. 커널 설정 확인
echo
log_info "2. 커널 설정 확인 중..."

# /dev/ksancov 디바이스 확인
if [ -e "/dev/ksancov" ]; then
    log_success "/dev/ksancov 디바이스 발견"
    ls -la /dev/ksancov
else
    log_warning "/dev/ksancov 디바이스를 찾을 수 없습니다."
    log_info "커널이 CONFIG_KSANCOV로 빌드되어 있는지 확인하세요."
fi

# 커널 설정 확인
log_info "커널 커버리지 설정:"
for setting in kern.kcov.od.support_enabled kern.kcov.od.logging_enabled kern.kcov.od.config; do
    value=$(sysctl -n "$setting" 2>/dev/null || echo "N/A")
    if [ "$value" != "N/A" ]; then
        log_success "$setting: $value"
    else
        log_warning "$setting: 설정되지 않음"
    fi
done

# 3. 필요한 도구들 설치
echo
log_info "3. 필요한 도구들 확인 중..."

# gcc 확인
if command -v gcc &> /dev/null; then
    GCC_VERSION=$(gcc --version | head -n1)
    log_success "GCC: $GCC_VERSION"
else
    log_error "GCC가 설치되지 않았습니다."
    exit 1
fi

# Python3 확인
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    log_success "Python3: $PYTHON_VERSION"
else
    log_error "Python3가 설치되지 않았습니다."
    exit 1
fi

# 4. ksancov 도구 빌드
echo
log_info "4. ksancov 도구 빌드 중..."

# XNU 소스에서 ksancov 컴파일 시도
XNU_PATHS=(
    "/usr/local/src/xnu/xnu/san/tools"
    "/opt/xnu/xnu/san/tools"
    "/usr/src/xnu/xnu/san/tools"
    "$HOME/xnu/xnu/san/tools"
    "/Users/foo/Desktop/coverage/xnu/xnu/san/tools"
)

KSANCOV_BUILT=false

for xnu_path in "${XNU_PATHS[@]}"; do
    if [ -f "$xnu_path/ksancov.c" ]; then
        log_info "XNU 소스 발견: $xnu_path"
        cd "$xnu_path"
        
        if gcc -o ksancov ksancov.c 2>/dev/null; then
            log_success "ksancov 컴파일 성공"
            cp ksancov "$SCRIPT_DIR/"
            KSANCOV_BUILT=true
            break
        else
            log_warning "ksancov 컴파일 실패: $xnu_path"
        fi
    fi
done

cd "$SCRIPT_DIR"

# ksancov가 없으면 에러
if [ ! -f "ksancov" ] && [ "$KSANCOV_BUILT" = false ]; then
    log_error "ksancov 도구를 찾을 수 없습니다."
    log_error "XNU 소스 코드에서 ksancov.c를 찾아서 수동으로 빌드하거나,"
    log_error "기존 ksancov 바이너리를 이 디렉토리에 복사해주세요."
    log_error ""
    log_error "수동 빌드 방법:"
    log_error "  cd /path/to/xnu/xnu/san/tools"
    log_error "  gcc -o ksancov ksancov.c"
    log_error "  cp ksancov /path/to/this/directory/"
    exit 1
fi

# 5. 테스트 프로그램들 컴파일
echo
log_info "5. 테스트 프로그램들 컴파일 중..."

# simple_coverage_test 컴파일
if [ -f "simple_coverage_test.c" ]; then
    if gcc -o simple_coverage_test simple_coverage_test.c; then
        log_success "simple_coverage_test 컴파일 성공"
    else
        log_warning "simple_coverage_test 컴파일 실패"
    fi
fi

# ksancov_example 컴파일
if [ -f "ksancov_example.c" ]; then
    if gcc -o ksancov_example ksancov_example.c; then
        log_success "ksancov_example 컴파일 성공"
    else
        log_warning "ksancov_example 컴파일 실패"
    fi
fi

# 6. 권한 설정
echo
log_info "6. 권한 설정 중..."

# 실행 권한 부여
chmod +x ksancov 2>/dev/null || true
chmod +x simple_coverage_test 2>/dev/null || true
chmod +x ksancov_example 2>/dev/null || true
chmod +x coverage_analyzer.py 2>/dev/null || true
chmod +x build_and_run.sh 2>/dev/null || true

log_success "실행 권한 설정 완료"

# 7. 환경 설정 완료
echo
log_info "7. 환경 설정 완료!"

echo "=========================================="
log_success "KSANCOV 환경 설정이 완료되었습니다!"
echo "=========================================="

echo
log_info "사용 가능한 도구들:"
echo "  • ./ksancov - 커버리지 측정 도구"
echo "  • ./simple_coverage_test - 간단한 커버리지 테스트"
echo "  • ./ksancov_example - 고급 예제 프로그램"
echo "  • ./coverage_analyzer.py - 커버리지 분석기"
echo "  • ./run_demo.sh - 통합 데모 실행"

echo
log_info "빠른 시작:"
echo "  ./run_demo.sh check    # 환경 확인"
echo "  ./run_demo.sh trace    # TRACE 모드 데모"
echo "  ./run_demo.sh counters # COUNTERS 모드 데모"
echo "  ./run_demo.sh full     # 전체 테스트"

echo
log_warning "주의사항:"
echo "  • 커버리지 측정은 sudo 권한이 필요할 수 있습니다"
echo "  • /dev/ksancov 디바이스가 없으면 커널을 다시 빌드해야 합니다"
echo "  • 일부 기능은 특정 macOS 버전에서만 동작할 수 있습니다"

echo
log_success "설정 완료! 이제 커버리지 측정을 시작할 수 있습니다."
