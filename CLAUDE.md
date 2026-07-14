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

- **MVC 계층 분리** (PoC1 ConsoleMVC 참조):
  - `model/`: 데이터 구조 + 순수 비즈니스 로직. 콘솔 I/O 금지.
  - `view/`: 콘솔 입출력만 담당. 비즈니스 로직(계산/검증) 금지.
  - `controller/`: view의 입력을 model/repository/service에 위임하고 결과를 view로 전달.
  - `repository/`: JSON 파일 CRUD (PoC2 DataPersistence 참조). model은 repository를 몰라야 함(의존 방향: controller → repository → model).
- **JSON 처리**: 외부 라이브러리 없이 PoC2에서 검증한 `JsonValue` 파서/직렬화기를 그대로
  vendoring해서 사용한다. 스키마 변경 시 `src/json/`만 수정하면 되도록 격리한다.
- **생산 라인 시간 처리**: 백그라운드 스레드 금지. `ProductionLine::advanceIfDue()` 처럼
  조회 시점에 경과시간을 계산하는 lazy 방식으로 구현한다 (테스트에서 실제 시간 대기 없이
  가짜 시계를 주입할 수 있어야 함 — production-line.md 참고).

## 빌드/테스트 환경

- Visual Studio 2026 (내부 버전 18), MSVC 14.51, C++17 이상
- GoogleTest 기반 단위테스트, `test/` 프로젝트에서 관리
- CMake/Make는 사용하지 않는다 (VS 프로젝트 파일 + MSBuild/vstest로 빌드·실행)

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
