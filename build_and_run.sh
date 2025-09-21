#!/bin/bash

# XNU 커널 커버리지 데모 빌드 및 실행 스크립트

set -e  # 에러 발생시 스크립트 종료

echo "=== XNU 커널 커버리지 데모 ==="
echo

# 현재 디렉토리 확인
CURRENT_DIR=$(pwd)
XNU_TOOLS_DIR="/Users/foo/Desktop/coverage/xnu/xnu/san/tools"

echo "1. 환경 확인..."

# ksancov 디바이스 확인
if [ ! -e "/dev/ksancov" ]; then
    echo "경고: /dev/ksancov 디바이스를 찾을 수 없습니다."
    echo "커널이 CONFIG_KCOV로 빌드되어 있는지 확인하세요."
    echo
else
    echo "✓ /dev/ksancov 디바이스 발견"
    ls -la /dev/ksancov
    echo
fi

# sysctl 설정 확인
echo "2. 커널 설정 확인..."
echo "kern.kcov.od.support_enabled: $(sysctl -n kern.kcov.od.support_enabled 2>/dev/null || echo 'N/A')"
echo "kern.kcov.od.logging_enabled: $(sysctl -n kern.kcov.od.logging_enabled 2>/dev/null || echo 'N/A')"
echo "kern.kcov.od.config: $(sysctl -n kern.kcov.od.config 2>/dev/null || echo 'N/A')"
echo

echo "3. ksancov 도구 컴파일..."

# ksancov 도구 컴파일
if [ -f "$XNU_TOOLS_DIR/ksancov.c" ]; then
    echo "XNU 소스에서 ksancov 컴파일 중..."
    cd "$XNU_TOOLS_DIR"
    
    # ksancov 컴파일
    if gcc -o ksancov ksancov.c 2>/dev/null; then
        echo "✓ ksancov 컴파일 성공"
        cp ksancov "$CURRENT_DIR/"
    else
        echo "⚠ ksancov 컴파일 실패, 대체 방법 사용"
    fi
    
    cd "$CURRENT_DIR"
else
    echo "⚠ ksancov 소스를 찾을 수 없음"
fi

echo

echo "4. 데모 프로그램 컴파일..."

# 간단한 커버리지 테스트 프로그램 컴파일
if gcc -o simple_coverage_test simple_coverage_test.c 2>/dev/null; then
    echo "✓ simple_coverage_test 컴파일 성공"
else
    echo "⚠ simple_coverage_test 컴파일 실패"
fi

# 빠른 데모 프로그램 컴파일
if gcc -o quick_coverage_demo quick_coverage_demo.c 2>/dev/null; then
    echo "✓ quick_coverage_demo 컴파일 성공"
else
    echo "⚠ quick_coverage_demo 컴파일 실패"
fi

echo

echo "5. 커버리지 데모 실행..."
echo "========================================"

# 권한 확인
if [ ! -r "/dev/ksancov" ]; then
    echo "⚠ /dev/ksancov에 읽기 권한이 없습니다."
    echo "다음 명령으로 권한을 확인하세요:"
    echo "  sudo ls -la /dev/ksancov"
    echo "  sudo chmod 644 /dev/ksancov  # 필요한 경우"
    echo
fi

# ksancov 도구가 있으면 직접 실행
if [ -x "./ksancov" ]; then
    echo "=== ksancov 도구로 TRACE 모드 테스트 ==="
    echo "getppid() 시스템 콜의 커버리지를 측정합니다..."
    
    if sudo ./ksancov --trace --entries 100 2>/dev/null; then
        echo "✓ TRACE 모드 성공"
    else
        echo "⚠ TRACE 모드 실행 실패 (권한 문제일 수 있음)"
    fi
    echo
    
    echo "=== ksancov 도구로 COUNTERS 모드 테스트 ==="
    if sudo ./ksancov --counters 2>/dev/null; then
        echo "✓ COUNTERS 모드 성공"
    else
        echo "⚠ COUNTERS 모드 실행 실패"
    fi
    echo
fi

# 커스텀 데모 프로그램 실행
if [ -x "./simple_coverage_test" ]; then
    echo "=== 커스텀 커버리지 테스트 실행 ==="
    if sudo ./simple_coverage_test 2>/dev/null; then
        echo "✓ 커스텀 테스트 성공"
    else
        echo "⚠ 커스텀 테스트 실행 실패"
        echo "일반 사용자 권한으로 시도..."
        ./simple_coverage_test || echo "일반 권한으로도 실행 실패"
    fi
    echo
fi

echo "========================================"
echo "6. 사용 가능한 명령들:"
echo
echo "직접 ksancov 사용:"
echo "  sudo ./ksancov --trace --entries 1000"
echo "  sudo ./ksancov --counters"
echo
echo "특정 프로그램의 커버리지 측정:"
echo "  sudo ./ksancov --trace --exec /bin/ls"
echo "  sudo ./ksancov --counters --exec /usr/bin/whoami"
echo
echo "커스텀 테스트 프로그램:"
echo "  sudo ./simple_coverage_test"
echo
echo "=== 데모 완료 ==="
