# PLAN — SampleOrderSystem 구현 계획

각 Phase는 그 자체로 빌드/테스트 가능한 단위로 끊는다. Phase 완료 기준은
"해당 Phase의 GoogleTest 스위트가 전부 GREEN"이다. Phase 순서는 아래와 같이 고정하되,
막히면 순서를 조정할 수 있다 (조정 시 이 문서를 갱신).

커밋 규칙은 [CLAUDE.md](CLAUDE.md)의 "커밋 워크플로우" 절을 따른다 (Phase 완료 즉시
커밋하지 않고, 커밋 메시지 초안만 제시 후 사용자 승인을 기다린다).

## Phase 0 — 프로젝트 초기 설정

- Visual Studio 2026(v18) C++ 콘솔 프로젝트 생성. **프로젝트는 `src/SampleOrderSystem.vcxproj`
  하나뿐**이며, Debug/Release(App)와 Test(GoogleTest) 구분은 별도 프로젝트가 아니라
  MSBuild Configuration(`Debug|x64`/`Release|x64`/`Test|x64`)으로 처리한다 (파일별
  `ExcludedFromBuild` 조건으로 App 전용/테스트 전용 소스를 구성별로 제외).
- 디렉터리 구조(모두 `src/` 하나 아래, 프로젝트 분리 없음): `src/model`, `src/view`,
  `src/controller`, `src/repository`, `src/production`, `src/json`, `src/test`
- GoogleTest/GoogleMock 통합: NuGet 패키지 `gmock` 1.11.0. `gmock.targets` import 자체를
  `Condition="'$(Configuration)'=='Test'"`로 걸어 Debug/Release 빌드에는 테스트
  프레임워크가 전혀 링크되지 않게 한다. `packages/`는 재현성을 위해 git에 커밋
  (vcpkg는 이 환경에 미설치, nuget restore 신뢰 불가).
- PoC1(ConsoleMVC)의 MVC 스켈레톤을 참조해 최소 골격 이식 (Sample 등록/조회 정도까지, 아직 영속성 없음)
- PoC2(DataPersistence)의 `JsonValue` 구현을 `src/json/`으로 vendoring
- Debug/Release/Test 세 Configuration이 모두 MSBuild로 빌드되는지 확인 (스모크 테스트로 파이프라인 검증)

**TODO**
- [x] .sln/.vcxproj 생성 (단일 프로젝트 + Debug/Release/Test 3-Configuration 구조)
- [x] GoogleTest/GoogleMock 참조 추가(NuGet `gmock` 1.11.0, Test 구성 전용) 및 스모크 테스트 통과 확인
- [x] MVC 폴더 구조 생성 (`src/{model,view,controller,repository,production}`, PoC1 참조)
- [x] JsonValue vendoring (PoC2 참조) + 파싱/직렬화 회귀 테스트 포팅 (`src/test/JsonValueTest.cpp`)
- [x] `build.ps1 -Configuration <Debug|Release|Test>`/`test.ps1` 커맨드라인 스크립트로
  세 Configuration 빌드·테스트 파이프라인 검증

## Phase 1 — Sample 도메인 & 영속성 (완료)

대상 문서: [docs/FEATURES/sample-management.md](docs/FEATURES/sample-management.md)

- [x] `Sample` 모델 + 검증 규칙 TDD 구현 (`src/model/Sample.*`, `src/test/SampleTest.cpp` 11개)
- [x] `SampleRepository` (JSON 파일 CRUD, PoC2 패턴 확장) TDD 구현
      (`src/repository/SampleRepository.*`, `src/test/SampleRepositoryTest.cpp` 13개:
      등록/중복거부/조회/재고반영/검색 6종/영속성)
- 등록/조회/검색의 실제 비즈니스 로직(검증, 중복 거부, 부분 문자열 검색)은 이미
  `SampleRepository`에 전부 구현·테스트되어 있어, 별도의 얇은 Controller는 지금 추가하지
  않는다 (View가 없는 상태에서 패스스루 Controller를 먼저 만들면 TDD 원칙에 어긋남).
  Controller/View 배선은 Phase 5(메인 메뉴 통합)에서 실제 입출력이 필요한 시점에 추가한다.

## Phase 2 — Order 도메인 & 접수 (완료)

대상 문서: [docs/FEATURES/order-intake.md](docs/FEATURES/order-intake.md)

- [x] `Order` 모델 + 주문번호 채번 규칙 TDD 구현 (`src/model/Order.*`)
      — 채번은 별도 카운터를 저장하지 않고, 같은 날짜(`ORD-YYYYMMDD-`) 접두사를 가진
      기존 주문 중 최대 순번 다음 값으로 즉석 계산 (검증 실패 시 순번 미소비가 자동 보장됨)
- [x] `OrderRepository` (JSON 파일 CRUD) TDD 구현 (`src/repository/OrderRepository.*`)
      — sampleId 존재 검증(SampleRepository 참조), quantity>0, customerName
      trim 후 비어있지 않음 검증. 시간은 `Clock`(`std::function`)으로 주입 가능해
      날짜 경계(자정 등)를 실제 대기 없이 테스트
- [x] `src/test/OrderRepositoryTest.cpp` (10개): AC-1~AC-9 전체
      (정상 접수/순번 증가/존재하지 않는 sampleId/수량 0·음수/고객명 빈값·공백/
      날짜 변경 시 순번 초기화/검증 실패 시 순번 미소비/재시작 후 영속성)

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
