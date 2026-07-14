# PLAN — SampleOrderSystem 구현 계획

각 Phase는 그 자체로 빌드/테스트 가능한 단위로 끊는다. Phase 완료 기준은
"해당 Phase의 GoogleTest 스위트가 전부 GREEN"이다. Phase 순서는 아래와 같이 고정하되,
막히면 순서를 조정할 수 있다 (조정 시 이 문서를 갱신).

커밋 규칙은 [CLAUDE.md](CLAUDE.md)의 "커밋 워크플로우" 절을 따른다 (Phase 완료 즉시
커밋하지 않고, 커밋 메시지 초안만 제시 후 사용자 승인을 기다린다).

## Phase 0 — 프로젝트 초기 설정

- Visual Studio 2026(v18) C++ 콘솔 프로젝트 생성 (.sln + .vcxproj)
- 디렉터리 구조: `src/model`, `src/view`, `src/controller`, `src/repository`,
  `src/production`, `src/json`, `include/`(또는 헤더를 src와 동일 폴더에 둘지 결정),
  `test/`(GoogleTest 프로젝트, 별도 vcxproj)
- GoogleTest 통합 방법 결정 및 적용 (NuGet 패키지 `Microsoft.googletest.v143.windesktop.msvcstl.static.rt-dyn`
  또는 vcpkg — 이번 환경엔 vcpkg 미설치 확인되어 NuGet 우선 검토)
- PoC1(ConsoleMVC)의 MVC 스켈레톤을 참조해 최소 골격 이식 (Sample 등록/조회 정도까지, 아직 영속성 없음)
- PoC2(DataPersistence)의 `JsonValue` 구현을 `src/json/`으로 vendoring
- `main` 프로젝트와 `test` 프로젝트가 각각 devenv/MSBuild로 빌드되는지 확인 (Hello World 수준 테스트 1개로 파이프라인 검증)

**TODO**
- [ ] .sln/.vcxproj 생성
- [ ] GoogleTest 참조 추가 및 더미 테스트 1개 통과 확인
- [ ] MVC 폴더 구조 생성 (PoC1 참조)
- [ ] JsonValue vendoring (PoC2 참조) + 파싱/직렬화 단위테스트 이식

## Phase 1 — Sample 도메인 & 영속성

대상 문서: [docs/FEATURES/sample-management.md](docs/FEATURES/sample-management.md)

- `Sample` 모델 + 검증 규칙 TDD 구현
- `SampleRepository` (JSON 파일 CRUD, PoC2 패턴 확장) TDD 구현
- 시료 등록/조회/검색 Controller-level 로직 TDD 구현 (View는 아직 콘솔 출력 붙이지 않아도 됨 — 로직만)

## Phase 2 — Order 도메인 & 접수

대상 문서: [docs/FEATURES/order-intake.md](docs/FEATURES/order-intake.md)

- `Order` 모델 + 주문번호 채번 규칙 TDD 구현
- `OrderRepository` (JSON 파일 CRUD) TDD 구현
- 주문 접수(RESERVED 생성) 로직 TDD 구현 — Sample 존재 검증은 Phase 1의 SampleRepository에 의존

## Phase 3 — 주문 승인/거절 & 생산 라인

대상 문서: [docs/FEATURES/order-approval.md](docs/FEATURES/order-approval.md),
[docs/FEATURES/production-line.md](docs/FEATURES/production-line.md)

가장 로직이 복잡한 Phase. 두 문서를 하나의 Phase로 묶은 이유: 승인 시 재고부족 분기가
곧바로 생산 큐 등록으로 이어지므로 분리 구현 시 중간 상태가 무의미해짐.

- 승인(재고충분/부족 분기), 거절 로직 TDD 구현
- `ProductionLine` (FIFO 큐, `advanceIfDue()` 경과시간 판정) TDD 구현 — 가짜 시계(주입 가능한 clock)로 실제 대기 없이 테스트
- 실생산량/총생산시간 공식 경계값 테스트 (공식 문서의 worked example 그대로 재현)

## Phase 4 — 모니터링 & 출고 처리

대상 문서: [docs/FEATURES/monitoring.md](docs/FEATURES/monitoring.md),
[docs/FEATURES/shipment.md](docs/FEATURES/shipment.md)

- 상태별 주문 수 집계 TDD 구현
- 재고 상태(여유/부족/고갈) 판정 TDD 구현
- 출고 처리(CONFIRMED → RELEASE) TDD 구현

## Phase 5 — 메인 메뉴 통합 (콘솔 View/Controller 배선)

대상 문서: [docs/FEATURES/main-menu.md](docs/FEATURES/main-menu.md)

- 지금까지 로직 레이어(Phase 1~4)에 콘솔 View/Controller를 배선
- 메뉴 라우팅은 단위테스트로 커버 (가짜 입력 소스 주입), 콘솔 출력 포맷 자체는 수동 검증
- PDF의 예시 UI 시나리오를 실제로 재현해 수동 검증 (주문 승인 재고부족 처리, FIFO 대기열,
  모니터링 표시, 출고 처리)

## Phase 6 — Dummy 데이터 연동 & 마무리

- PoC4(DummyDataGenerator)의 더미 생성 로직을 참조해 시연/테스트용 초기 데이터 생성 지원
  (메인 앱의 숨김 메뉴 또는 별도 실행 파일 — Phase 6 시작 시 결정)
- 전체 CleanCode 재검토 (계층 간 책임, 중복 제거, 네이밍)
- README.md/PRD.md/PLAN.md/CLAUDE.md 최종 정합성 확인
- 커버리지 확인 (OpenCppCoverage 사용 가능 여부 확인 후 적용)

## 진행 방식 요약

각 Phase 내부는 `.claude/skills/test-driven-development/SKILL.md`의 Red-Green-Refactor
사이클을 기능 단위로 반복한다. Phase 착수 전 해당 docs/FEATURES/*.md의 Acceptance
Criteria를 먼저 테스트 목록으로 변환한 뒤 하나씩 구현한다.
