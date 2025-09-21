#!/usr/bin/env python3
"""
XNU 커널 커버리지 분석기

이 스크립트는 ksancov로 수집된 커버리지 데이터를 분석하고 시각화합니다.
"""

import os
import sys
import subprocess
import json
import time
from collections import defaultdict, Counter

class KernelCoverageAnalyzer:
    def __init__(self):
        self.ksancov_path = "./ksancov"
        self.results = {}
        
    def check_environment(self):
        """커버리지 측정 환경을 확인합니다."""
        print("=== 환경 확인 ===")
        
        # ksancov 디바이스 확인
        if os.path.exists("/dev/ksancov"):
            print("✓ /dev/ksancov 디바이스 발견")
            try:
                stat = os.stat("/dev/ksancov")
                print(f"  권한: {oct(stat.st_mode)[-3:]}")
            except:
                print("  권한 확인 실패")
        else:
            print("✗ /dev/ksancov 디바이스 없음")
            return False
            
        # ksancov 도구 확인
        if os.path.exists(self.ksancov_path):
            print("✓ ksancov 도구 발견")
        else:
            print("✗ ksancov 도구 없음")
            return False
            
        # 커널 설정 확인
        try:
            result = subprocess.run(["sysctl", "kern.kcov.od.support_enabled"], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                value = result.stdout.strip().split(": ")[1]
                print(f"✓ kern.kcov.od.support_enabled: {value}")
            else:
                print("? 커널 커버리지 설정 확인 불가")
        except:
            print("? sysctl 명령 실행 실패")
            
        print()
        return True
        
    def run_coverage_test(self, mode, program=None, entries=1000):
        """커버리지 테스트를 실행합니다."""
        print(f"=== {mode.upper()} 모드 커버리지 측정 ===")
        
        cmd = ["sudo", self.ksancov_path]
        
        if mode == "trace":
            cmd.extend(["--trace", "--entries", str(entries)])
        elif mode == "counters":
            cmd.append("--counters")
        elif mode == "stksize":
            cmd.extend(["--stksize", "--entries", str(entries)])
            
        if program:
            cmd.extend(["--exec", program])
            
        print(f"실행 명령: {' '.join(cmd)}")
        
        try:
            start_time = time.time()
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            end_time = time.time()
            
            self.results[mode] = {
                'returncode': result.returncode,
                'stdout': result.stdout,
                'stderr': result.stderr,
                'duration': end_time - start_time,
                'program': program
            }
            
            if result.returncode == 0:
                print("✓ 성공")
            else:
                print(f"✗ 실패 (코드: {result.returncode})")
                
            return result.returncode == 0
            
        except subprocess.TimeoutExpired:
            print("✗ 타임아웃")
            return False
        except Exception as e:
            print(f"✗ 실행 오류: {e}")
            return False
            
    def analyze_trace_results(self, mode):
        """TRACE 모드 결과를 분석합니다."""
        if mode not in self.results:
            return
            
        result = self.results[mode]
        stderr_lines = result['stderr'].split('\n')
        
        print(f"\n=== {mode.upper()} 모드 분석 결과 ===")
        print(f"실행 시간: {result['duration']:.2f}초")
        print(f"대상 프로그램: {result['program'] or 'getppid() 시스템콜'}")
        
        # stderr에서 정보 추출
        nedges = None
        maxpcs = None
        head = None
        addresses = []
        
        for line in stderr_lines:
            if "nedges (edgemap)" in line:
                nedges = line.split("=")[1].strip()
            elif "maxpcs" in line:
                maxpcs = line.split("=")[1].strip()
            elif "head" in line:
                head = line.split("=")[1].strip()
            elif line.startswith("0x"):
                addr = line.strip()
                if addr:
                    addresses.append(addr)
                    
        if nedges:
            print(f"총 에지 수: {nedges}")
        if maxpcs:
            print(f"최대 PC 수: {maxpcs}")
        if head:
            print(f"수집된 PC 수: {head}")
            
        if addresses:
            print(f"수집된 주소 수: {len(addresses)}")
            print("처음 5개 주소:")
            for i, addr in enumerate(addresses[:5]):
                print(f"  {i+1}. {addr}")
                
        # 커버리지 통계
        if head and head.isdigit():
            coverage_count = int(head)
            if coverage_count > 0:
                print(f"\n커버리지 수집 성공! {coverage_count}개의 실행 지점이 기록되었습니다.")
            else:
                print("\n커버리지 데이터가 수집되지 않았습니다.")
        
    def analyze_counters_results(self):
        """COUNTERS 모드 결과를 분석합니다."""
        if 'counters' not in self.results:
            return
            
        result = self.results['counters']
        stderr_lines = result['stderr'].split('\n')
        
        print(f"\n=== COUNTERS 모드 분석 결과 ===")
        print(f"실행 시간: {result['duration']:.2f}초")
        print(f"대상 프로그램: {result['program'] or 'getppid() 시스템콜'}")
        
        # stderr에서 정보 추출
        nedges = None
        hit_count = 0
        hit_addresses = []
        
        for line in stderr_lines:
            if "nedges" in line and "edgemap" in line:
                nedges = line.split("=")[1].strip()
            elif "hits [idx" in line:
                hit_count += 1
                hit_addresses.append(line.strip())
                
        if nedges:
            print(f"총 에지 수: {nedges}")
            
        print(f"히트된 에지 수: {hit_count}")
        
        if hit_addresses:
            print("히트된 에지들:")
            for addr in hit_addresses[:10]:  # 처음 10개만
                print(f"  {addr}")
            if len(hit_addresses) > 10:
                print(f"  ... (총 {len(hit_addresses)}개)")
                
        # 커버리지 비율 계산
        if nedges and nedges.isdigit() and hit_count > 0:
            total_edges = int(nedges)
            coverage_ratio = (hit_count / total_edges) * 100
            print(f"\n커버리지 비율: {coverage_ratio:.2f}% ({hit_count}/{total_edges})")
        
    def generate_report(self):
        """전체 분석 보고서를 생성합니다."""
        print("\n" + "="*60)
        print("커널 커버리지 분석 보고서")
        print("="*60)
        
        for mode in ['trace', 'counters', 'stksize']:
            if mode in self.results:
                result = self.results[mode]
                print(f"\n{mode.upper()} 모드:")
                print(f"  상태: {'성공' if result['returncode'] == 0 else '실패'}")
                print(f"  실행 시간: {result['duration']:.2f}초")
                
                if mode in ['trace', 'stksize']:
                    self.analyze_trace_results(mode)
                elif mode == 'counters':
                    self.analyze_counters_results()
                    
        print("\n" + "="*60)
        
    def run_comprehensive_test(self):
        """포괄적인 커버리지 테스트를 실행합니다."""
        if not self.check_environment():
            print("환경 설정이 올바르지 않습니다.")
            return False
            
        # 기본 테스트들
        tests = [
            ("trace", None, 1000),
            ("counters", None, None),
        ]
        
        # 특정 프로그램들에 대한 테스트
        test_programs = ["/bin/ls", "/usr/bin/whoami", "/bin/date"]
        
        for program in test_programs:
            if os.path.exists(program):
                tests.append(("trace", program, 500))
                tests.append(("counters", program, None))
                
        success_count = 0
        total_count = len(tests)
        
        for mode, program, entries in tests:
            test_name = f"{mode}_{program.replace('/', '_') if program else 'syscall'}"
            if self.run_coverage_test(mode, program, entries or 1000):
                success_count += 1
            print()
            
        print(f"테스트 완료: {success_count}/{total_count} 성공")
        
        # 결과 분석
        self.generate_report()
        
        return success_count > 0

def main():
    analyzer = KernelCoverageAnalyzer()
    
    if len(sys.argv) > 1:
        command = sys.argv[1]
        
        if command == "check":
            analyzer.check_environment()
        elif command == "trace":
            program = sys.argv[2] if len(sys.argv) > 2 else None
            analyzer.check_environment()
            analyzer.run_coverage_test("trace", program)
            analyzer.analyze_trace_results("trace")
        elif command == "counters":
            program = sys.argv[2] if len(sys.argv) > 2 else None
            analyzer.check_environment()
            analyzer.run_coverage_test("counters", program)
            analyzer.analyze_counters_results()
        elif command == "full":
            analyzer.run_comprehensive_test()
        else:
            print("사용법:")
            print("  python3 coverage_analyzer.py check")
            print("  python3 coverage_analyzer.py trace [program]")
            print("  python3 coverage_analyzer.py counters [program]")
            print("  python3 coverage_analyzer.py full")
    else:
        # 기본 실행: 포괄적인 테스트
        analyzer.run_comprehensive_test()

if __name__ == "__main__":
    main()
