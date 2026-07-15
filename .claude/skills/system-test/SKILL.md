---
name: system-test
description: Run/extend the console system test (system-test.ps1) that drives the real compiled SampleOrderSystem.exe end-to-end via scripted stdin, for changes GoogleTest can't catch on its own
---

# 콘솔 시스템 테스트 (system-test.ps1)

## 언제 쓰나

`.claude/skills/test-driven-development/SKILL.md`(GoogleTest, Red-Green-Refactor)는
model/repository/production/controller 라우팅 등 **로직 계층**을 검증한다. 이 스킬은
그 위 레이어, 즉 **실제로 컴파일된 `SampleOrderSystem.exe`를 콘솔 입력으로 구동했을 때
전체 플로우가 맞물려 정상 동작하는지**를 검증한다. 다음과 같은 변경 뒤에는 반드시 이
스킬을 참고해 `system-test.ps1`을 실행/갱신한다:

- 메인 메뉴 라우팅, 각 기능 컨트롤러의 콘솔 입출력 문구/흐름 변경
- 여러 컨트롤러/repository가 얽히는 엔드투엔드 플로우(승인→생산→모니터링→출고 등) 변경
- `main.cpp`의 의존성 배선(생성자 주입 순서 등) 변경

단위 로직 변경만 있고 콘솔 플로우/출력에 영향이 없다면(예: 순수 계산 공식 수정)
GoogleTest만으로 충분하며 이 스킬은 건너뛰어도 된다.

## 실행

```powershell
./system-test.ps1                       # Release 빌드 + 6개 시나리오 실행
./system-test.ps1 -Configuration Debug  # Debug 빌드로 실행
```

실패 시나리오가 있으면 exit code 1과 함께 실패 목록을 출력한다. `test.ps1`과 마찬가지로
커밋 전 게이트로 사용한다.

## 동작 원리

- `Invoke-Scenario`가 시나리오별로 `data/samples.json`/`data/orders.json`을 삭제해
  격리한 뒤, 입력 라인 배열을 stdin으로 파이프해 exe를 구동하고 stdout 전체를
  캡처한다 (`main.cpp`가 데이터 경로를 하드코딩하므로 순차 실행만 가능, 병렬 불가).
- `Assert-Contains`(stdout 부분 문자열 검사)와 `Assert-True`(JSON 파일 상태 등 임의
  조건 검사)로 검증하고, 실패를 `$script:failures` 배열에 누적한다.
- 재고부족(PRODUCING) 케이스는 실제 생산 완료(advanceIfDue 경과시간 판정)까지
  기다리지 않는다 — 승인 직후 전이와 화면에 표시되는 생산예정량/생산시간 수치만
  확인한다. 실제 완료 전이는 `src/test/ProductionLineTest.cpp`가 가짜 시계로 이미
  커버하므로 여기서 중복 검증하지 않는다.
- 주문번호는 `"ORD-" + (오늘 날짜 yyyyMMdd) + "-0001"`로 예측한다 — 시나리오마다
  데이터 파일을 초기화하므로 항상 그날의 첫 주문이 되어 결정적이다.

## 새 시나리오 추가하기

1. `system-test.ps1`에 `Invoke-Scenario`로 새 입력 시퀀스 블록을 추가한다 (기존
   시나리오들을 템플릿으로 삼는다 — 메뉴 번호 흐름은 각 컨트롤러의 `run()` 구현 참고).
2. `Assert-Contains`/`Assert-True`로 기대 결과(stdout 문구, JSON 파일 상태)를 검증한다.
3. 기대값을 일부러 깨서 실패가 감지되는지 1회 확인한 뒤 원복한다 (RED 확인에 해당).
4. `./system-test.ps1`과 `./test.ps1`을 모두 실행해 전체 통과를 확인한다.
