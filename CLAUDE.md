# CLAUDE.md

이 저장소는 반도체 시료 생산주문관리 시스템 개인과제의 **미션2 (본 프로젝트)** 이다.
PoC 4종(ConsoleMVC/DataPersistence/DataMonitor/DummyDataGenerator, 별도 저장소)에서
검증한 패턴을 재사용해 [PRD.md](PRD.md)의 요구사항을 구현한다. Phase 진행 순서와 각
Phase의 범위는 [PLAN.md](PLAN.md)를 따르고, 기능별 상세 스펙(테스트 케이스의 근거)은
[docs/FEATURES/*.md](docs/FEATURES/)에 있다.

## 필독 문서 우선순위

1. 지금 작업 중인 Phase의 `docs/FEATURES/*.md` (Acceptance Criteria = 테스트 목록)
2. `.claude/skills/test-driven-development/SKILL.md` (모든 구현은 이 절차를 따른다)
3. `PLAN.md` (현재 Phase 범위, 다음 Phase)
4. `PRD.md` (전체 도메인/설계 결정의 배경)

## 개발 방식: TDD 필수

**모든 프로덕션 코드는 실패하는 테스트 없이 작성하지 않는다.** 자세한 절차는
`.claude/skills/test-driven-development/SKILL.md` 참고. 요약:

1. RED: 실패하는 GoogleTest 케이스 작성 (docs/FEATURES의 Given-When-Then 1개 = 테스트 1개 기준)
2. RED 확인: 반드시 실행해서 "의도한 이유로" 실패하는지 확인
3. GREEN: 통과시키는 최소 코드만 작성 (과잉 설계 금지)
4. GREEN 확인: 해당 테스트 + 전체 스위트 통과 확인
5. REFACTOR: 테스트가 계속 GREEN인 상태에서 중복 제거/네이밍 개선

테스트 없이 먼저 구현 코드를 작성했다면 삭제하고 처음부터 다시 시작한다 (예외는 사용자
승인 필요).

## 아키텍처 규칙

- **MVC 계층 분리** (PoC1 ConsoleMVC 참조), 전부 `src/` 아래 폴더로 구분 (프로젝트 분리 아님):
  - `src/model/`: 데이터 구조 + 순수 비즈니스 로직. 콘솔 I/O 금지.
  - `src/view/`: 콘솔 입출력만 담당. 비즈니스 로직(계산/검증) 금지.
  - `src/controller/`: view의 입력을 model/repository/service에 위임하고 결과를 view로 전달.
  - `src/repository/`: JSON 파일 CRUD (PoC2 DataPersistence 참조). model은 repository를 몰라야 함(의존 방향: controller → repository → model).
  - `src/production/`: 생산 라인(FIFO 큐, lazy 완료 판정) 도메인 로직.
  - `src/generator/`: 더미(demo) 데이터 생성 로직 (PoC4 DummyDataGenerator 참조).
  - `src/test/`: GoogleTest/GoogleMock 테스트 (Test 구성에서만 컴파일됨, 아래 참고).
- **JSON 처리**: 외부 라이브러리 없이 PoC2에서 검증한 `JsonValue` 파서/직렬화기를 그대로
  vendoring해서 사용한다. 스키마 변경 시 `src/json/`만 수정하면 되도록 격리한다.
- **생산 라인 시간 처리**: 백그라운드 스레드 금지. `ProductionLine::advanceIfDue()` 처럼
  조회 시점에 경과시간을 계산하는 lazy 방식으로 구현한다 (테스트에서 실제 시간 대기 없이
  가짜 시계를 주입할 수 있어야 함 — production-line.md 참고).

## 빌드/테스트 환경

- Visual Studio 2026 (내부 버전 18), MSVC 14.51, C++17 이상 (`/std:c++20`로 컴파일)
- **단일 프로젝트 + 3가지 Configuration** (`src/SampleOrderSystem.vcxproj` 하나뿐, Core/App/Tests를
  별도 프로젝트로 나누지 않는다):
  - `Debug|x64`, `Release|x64`: `src/main.cpp`를 빌드/실행하는 실제 App. `src/test/*`는
    이 구성에서 `ExcludedFromBuild`로 컴파일에서 빠진다.
  - `Test|x64`: `src/main.cpp`를 제외하고 `src/test/*.cpp` + gmock을 빌드해 테스트
    실행파일을 만든다. Visual Studio 상단 툴바에서 구성(Configuration)을 `Test`로
    바꾸면 F5로 테스트를 실행할 수 있다.
  - `src/model|view|controller|repository|production|generator|json/`의 소스는 세 구성 모두에서
    항상 컴파일된다 (App과 Test가 같은 비즈니스 로직을 공유).
- 단위테스트는 GoogleTest/GoogleMock, NuGet 패키지 `gmock` 1.11.0으로 제공받는다
  (`src/test/packages.config`). vcpkg는 이 환경에 설치되어 있지 않아 사용하지 않는다.
  패키지 파일 일체(`packages/`)는 재현성을 위해 git에 커밋한다 (gitignore하지 않음) —
  이 환경에서 `nuget.exe` CLI restore가 보장되지 않기 때문.
- `gmock.targets` import 자체가 `Condition="'$(Configuration)'=='Test'"`로 걸려 있어,
  Debug/Release(실제 배포되는 App)는 테스트 프레임워크를 전혀 링크하지 않는다.
- 새 소스 파일을 추가할 때 `.vcxproj`의 `ExcludedFromBuild` 패턴을 반드시 지킨다:
  App 전용 파일은 Test에서 제외, 테스트 파일은 Debug/Release에서 제외.
- CMake/Make는 사용하지 않는다 (VS 프로젝트 파일 + MSBuild/vstest로 빌드·실행).
  `build.ps1 -Configuration <Debug|Release|Test>` / `test.ps1`(Test 구성 빌드 후 즉시 실행)로
  커맨드라인 빌드·테스트 가능.

## 시스템 테스트 (system-test.ps1)

GoogleTest(`test.ps1`)는 로직 계층(model/repository/production/controller의 라우팅)만
검증하며, 실제로 컴파일된 `SampleOrderSystem.exe`를 콘솔 입력으로 구동해 전체 플로우가
맞물려 동작하는지는 검증하지 않는다. 이를 보완하는 것이 `system-test.ps1`이다.

- Release 구성으로 빌드한 뒤, stdin에 미리 정의된 입력 시퀀스를 파이프로 흘려 넣어
  실제 exe를 구동하고, stdout과 `data/*.json` 최종 상태를 검증한다.
- 시나리오는 시나리오별로 `data/samples.json`/`data/orders.json`을 초기화해 격리한다
  (main.cpp가 이 경로를 하드코딩하므로 순차 실행만 지원, 병렬 실행 없음).
- 실제 생산 완료(advanceIfDue 경과시간 판정)까지는 기다리지 않는다 — 승인 직후의
  PRODUCING 전이와 화면에 표시되는 생산예정량/생산시간 수치만 검증하고, 실제 완료
  전이는 `ProductionLineTest`(가짜 시계)가 이미 커버한다.
- 새 기능을 추가하면(특히 메뉴 흐름이나 화면 출력에 영향을 주는 변경) 관련 시나리오를
  `system-test.ps1`에 추가하거나 기존 시나리오의 기대 출력을 갱신한다.
- 실행: `./system-test.ps1` (기본 Release; `-Configuration Debug`로 전환 가능)

## 커밋 워크플로우 (중요 — 매번 이 규칙을 따를 것, 반복해서 지시할 필요 없음)

Phase(또는 그 하위 작업 단위)를 완료해도 **바로 git commit 하지 않는다.**

1. 작업이 끝나면 `git status`/`git diff`로 변경사항을 정리해 **커밋 메시지 초안만** 사용자에게
   제시한다. 실제 `git commit`은 실행하지 않는다.
2. 사용자가 검토 후 "확인 내용 메시지"와 함께 커밋을 지시하면, 그 시점에만 커밋을 실행한다.
3. 커밋을 실행할 때는 사용자가 준 확인 내용 메시지를 아래 형식으로 커밋 메시지 하단에 추가한다:

   ```
   Reviewed-by: daejin.kim (<사용자가 준 확인 내용 메시지>)
   ```

4. 이 규칙은 이 프로젝트의 모든 Phase/커밋에 동일하게 적용되며, 사용자가 매번 다시
   설명할 필요가 없다.

## 문서 관리

- `PRD.md`: 요구사항 (배경, 도메인 모델, 기능 범위, 설계 결정)
- `PLAN.md`: Phase별 계획과 TODO. Phase 순서가 바뀌면 이 문서를 갱신한다.
- `docs/FEATURES/*.md`: 기능별 상세 스펙 + Acceptance Criteria (테스트 케이스 근거)
- `docs/adr/`: 스펙에 없어서 직접 내린 설계 결정(ADR)을 기록 (예: 모니터링의 "여유/부족"
  판정 기준처럼 스펙이 모호해 구체화가 필요했던 경우)
- 문서와 실제 구현이 어긋나면 코드가 아니라 문서를 갱신해 항상 최신 상태로 유지한다.
