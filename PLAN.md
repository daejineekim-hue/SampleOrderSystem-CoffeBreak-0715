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

## Phase 3 — 주문 승인/거절 & 생산 라인 (완료)

대상 문서: [docs/FEATURES/order-approval.md](docs/FEATURES/order-approval.md),
[docs/FEATURES/production-line.md](docs/FEATURES/production-line.md)

가장 로직이 복잡한 Phase. 두 문서를 하나의 Phase로 묶은 이유: 승인 시 재고부족 분기가
곧바로 생산 큐 등록으로 이어지므로 분리 구현 시 중간 상태가 무의미해짐.

- [x] 승인(재고충분/부족 분기), 거절 로직 TDD 구현 — `OrderRepository::approve/reject`
      (`src/test/OrderApprovalTest.cpp` 16개: 재고충분/경계값/재고부족/거절/이미
      확정·거절·생산중인 주문에 대한 재승인·재거절 거부/존재하지 않는 주문ID)
- [x] `ProductionLine` (FIFO 큐, `advanceIfDue()` 경과시간 판정) TDD 구현
      (`src/production/ProductionLine.*`, `src/test/ProductionLineTest.cpp` 14개) —
      가짜 시계(주입 가능한 clock)로 실제 대기 없이 테스트. 큐는 주문 id만 보관하고
      매 조회 시 `OrderRepository::findById`로 최신 정보를 조회 (Order 확장 필드를
      Order 모델에 추가하지 않고 ProductionLine 내부 QueueEntry에 캡슐화)
- [x] 실생산량/총생산시간 공식 경계값 테스트 (공식 문서의 worked example 그대로 재현) —
      `WorkedExample_Shortage50_Yield092`(61개/1220분), `ExactDivision_DoesNotOverRound`
      (부동소수점 올림 오차 방지, 100개로 정확히 나누어떨어짐)

## Phase 4 — 모니터링 & 출고 처리 (완료)

대상 문서: [docs/FEATURES/monitoring.md](docs/FEATURES/monitoring.md),
[docs/FEATURES/shipment.md](docs/FEATURES/shipment.md)

- [x] 상태별 주문 수 집계 TDD 구현 — `Monitoring::countByStatus`
      (`src/model/Monitoring.*`, `src/test/MonitoringTest.cpp` 중 5개: 정상 집계/
      REJECTED 제외/특정 상태 0건/전체 0건/여러 시료 합산). 순수 함수로 구현하여
      Order/Sample 목록만 받고 저장소 의존 없음 (model 계층 규칙 준수)
- [x] 재고 상태(여유/부족/고갈) 판정 TDD 구현 — `Monitoring::stockStatus`
      (`src/test/MonitoringTest.cpp` 나머지 12개: 여유/부족/경계값(동률=여유)/
      고갈 우선순위/CONFIRMED·RELEASE·REJECTED 제외/여러 대기주문 합산/
      시료 없음/여러 시료 독립 판정). monitoring.md의 "설계 결정" 절이 정의한
      pendingDemand 규칙을 그대로 구현
- [x] 출고 처리(CONFIRMED → RELEASE) TDD 구현 — `OrderRepository::release`/
      `findShippable` (`src/test/ShipmentTest.cpp` 13개: 정상 전이/독립성/
      재고 재차감 없음 확인/RESERVED·PRODUCING·RELEASE·REJECTED 상태에서 거부/
      존재하지 않는 주문 ID/CONFIRMED만 필터링하는 목록 조회)

## Phase 5 — 메인 메뉴 통합 (콘솔 View/Controller 배선) (완료)

대상 문서: [docs/FEATURES/main-menu.md](docs/FEATURES/main-menu.md)

- [x] 지금까지 로직 레이어(Phase 1~4)에 콘솔 View/Controller를 배선 —
      `SampleManagementController`/`OrderIntakeController`/`OrderApprovalController`/
      `MonitoringController`/`ProductionLineController`/`ShipmentController` 6종 신설,
      각각 해당 Repository/Model만 호출 (자체 비즈니스 로직 없음)
- [x] 메뉴 라우팅은 단위테스트로 커버 (가짜 입력 소스 주입) —
      `MainMenuController` (`src/test/MainMenuControllerTest.cpp` 8개: 1~6 라우팅/
      범위 밖 숫자·비숫자 입력 오류 후 재입력/정상 복구/`0` 종료/하위 기능 후 메인
      메뉴 복귀/요약 정보 매 진입 시 조회). 콘솔 출력 포맷 자체는 수동 검증 범위
      (`ConsoleInputSource`/`ConsoleOutputSink`, `src/view/ConsoleMenuIO.*`)
- [x] PDF의 예시 UI 시나리오를 실제로 재현해 수동 검증 — 시료 등록 → 주문 접수 →
      승인(재고충분 즉시 CONFIRMED/재고부족 PRODUCING 생산큐 등록, 실생산량·생산시간
      공식 실측 확인) → 모니터링(주문량/재고상태) → 생산라인 조회 → 출고 처리,
      잘못된 메뉴 입력(`99`, `abc`) 후 정상 복구까지 `SampleOrderSystem.exe`
      직접 실행으로 확인

## Phase 6 — Dummy 데이터 연동 & 마무리 (완료)

- [x] PoC4(DummyDataGenerator)의 더미 생성 로직을 참조해 시연/테스트용 초기 데이터 생성
      지원 — `src/generator/DummyDataGenerator.*` (`src/test/DummyDataGeneratorTest.cpp`
      6개: 요청 개수 생성/필드 유효성/영속화/기존 데이터 미덮어쓰기/ID 채번 연속성/
      0건 요청). **메인 앱의 숨김 메뉴**로 결정(메뉴 번호 `9`, `MainMenuController`의
      화면 출력 목록에는 노출되지 않지만 라우팅은 정상 동작 — `Choice9_...` 테스트로
      커버). ID 접두사는 실제 앱 데이터(`SMP-...`)와 구분되도록 `DUMMY-###`로 채번
- [x] 전체 CleanCode 재검토 — 5개 컨트롤러에 중복되어 있던 `skipToNextLine()` 콘솔
      입력 헬퍼를 `src/view/ConsoleMenuIO.h`의 공용 함수로 추출
- [x] README.md/PRD.md/PLAN.md/CLAUDE.md 최종 정합성 확인 — README의 저장소명/PoC
      참조/상태 절을 최신화(옛 `-KimDaejin-03086508` 명명 → `-CoffeBreak-0715`), PRD의
      Order 도메인 모델 설명을 실제 구현(생산 관련 필드는 `ProductionLine`이 보관)에
      맞게 정정하고 [docs/adr/0001](docs/adr/0001-production-fields-live-in-productionline-not-order.md)
      / [docs/adr/0002](docs/adr/0002-production-waiting-count-definition.md)로
      근거를 남김, CLAUDE.md 폴더 목록에 `production`/`generator` 반영
- [x] 커버리지 확인 (OpenCppCoverage 사용 가능 여부 확인 후 적용) — 설치 확인 후
      Test 구성 실행 결과로 리포트 생성(로컬 산출물, `.gitignore`에 `coverage_report/`
      추가). 모델/저장소/생산라인/모니터링 로직은 100% 근접, 전체 라인 커버리지는
      약 68% — 나머지는 main-menu.md의 Testability note에 따라 단위테스트 범위 밖인
      콘솔 I/O 컨트롤러(시료/주문/승인/출고 화면)에 해당하며 Phase 5에서 실제 실행
      시나리오로 수동 검증됨

## Phase 7 — 콘솔 시스템 테스트 (완료)

Phase 0~6의 원래 계획에는 없었으나, GoogleTest는 로직 계층(model/repository/
production/controller 라우팅)만 검증하고 실제 컴파일된 `SampleOrderSystem.exe`를
콘솔 입력으로 구동한 전체 플로우 검증은 Phase 5에서 사람이 수동으로 한 번 확인한
게 전부였다. 이를 반복 가능한 자동 테스트로 만들기 위해 사용자 요청으로 추가된 단계.

- [x] `system-test.ps1` 신설 — Release 빌드 후 실제 `SampleOrderSystem.exe`를 stdin
      파이프로 구동하고 stdout/`data/*.json` 최종 상태를 검증하는 6개 시나리오:
      정상 플로우(재고충분 승인→출고, 재고 불변 확인), 재고부족(생산라인 라우팅,
      실생산량/생산시간 공식 실측값 확인), 주문 거절(모니터링 집계 제외 확인),
      잘못된 메인 메뉴 입력 후 정상 복구, 숨김 더미 데이터 메뉴(9번), 즉시 종료(0).
      시나리오마다 `data/samples.json`/`data/orders.json`을 초기화해 격리(순차 실행).
      의도적으로 기대값을 깨뜨려 실패 감지가 되는지 확인 후 원복.
- [x] `test.ps1`(GoogleTest 113개) 회귀 없음 재확인
- [x] README.md "빌드 및 실행"/"상태" 절, CLAUDE.md에 "시스템 테스트" 절 추가해
      `test.ps1`(로직 계층)과 `system-test.ps1`(실행 파일 전체 플로우)의 역할 구분 명시
- [x] `.claude/skills/system-test/SKILL.md` 등록 — 콘솔 입출력에 영향을 주는 변경
      후에는 이 스킬을 참고해 `system-test.ps1` 시나리오를 갱신/추가하도록 안내

## Phase 8 — 아키텍처 리팩토링 (완료)

Phase 0~7로 기능 요구사항은 모두 구현되었으나, TDD로 기능을 하나씩 쌓는 과정에서
생긴 구조적 부채가 있어 사용자 요청으로 추가된 리팩토링 단계. 세부 설계는
[docs/design_refact.md](docs/design_refact.md) 참고. **동작은 바꾸지 않고 구조만
변경** — 매 항목 후 GoogleTest/system-test.ps1 회귀 확인.

- [x] **항목 1+2: `OrderRepository` 책임 분리 + `ProductionLine` 양방향 결합 제거** —
      `OrderRepository`에서 `approve`/`reject`/`release`/`completeProduction`을
      제거하고 검증 없는 `updateStatus`만 남김(CRUD 전용화). 사용자 트리거 전이는
      신설 `src/service/OrderLifecycleService`(approve/reject/release)로 이동.
      생산 완료 시점의 재고/상태 반영(Formula #3)은 `ProductionLine`이
      `OrderRepository::updateStatus`/`SampleRepository::updateStock`을 직접 호출하는
      방식으로 변경해, `OrderRepository`가 더 이상 `ProductionLine`을 몰라도 되게 함
      (전방 선언 제거, 순환 의존 해소). `OrderApprovalTest`/`ShipmentTest`/
      `ProductionLineTest`를 새 호출부에 맞게 갱신, 기존 Acceptance Criteria 불변.
- [x] **항목 3: 콘솔 서브메뉴 루프 중복 제거** — `ConsoleSubmenuController` 베이스
      클래스 신설(`run()`을 템플릿 메서드로 구현), `SampleManagementController`/
      `OrderIntakeController`/`OrderApprovalController`/`ShipmentController` 4곳이
      상속해 `showPrompt()`/`handle()`만 구현하도록 변경. `system-test.ps1`로
      실제 콘솔 플로우 회귀 없음 확인.
- [x] **항목 4: `MenuSummaryProvider`의 `Monitoring` 로직 중복 제거** —
      `productionWaitingCount` 계산을 자체 루프 대신
      `Monitoring::countByStatus(orders).reserved + .producing`으로 교체해
      docs/adr/0002 정의와의 드리프트 위험 제거.
- [x] 매 항목 적용 후 `test.ps1`(GoogleTest 113개), 항목 3 이후 `system-test.ps1`
      (6개 시나리오)까지 재실행해 최종 회귀 없음 확인.

## Phase 9 — 버그 수정: 메인 메뉴 입력 씹힘 (완료)

Phase 8 리팩토링 직후 실제 exe로 생산라인 완료를 데모하다가 발견한 버그를 수정한
단계. 메인 메뉴에서 `[4] 모니터링`/`[5] 생산라인 조회`를 연 뒤 바로 다음 실제
입력이 씹히는 문제가 있었다. `MainMenuController`는 `getline()`으로 메뉴 번호를
읽어 개행을 이미 소비하는데, `MonitoringController`/`ProductionLineController`가
화면 끝에서 "계속하려면 Enter를 누르세요..." + `skipToNextLine()`을 호출하면
남은 개행이 없어 사용자의 다음 실제 입력 한 줄을 그대로 삼켜버렸다
(`DummyDataController`는 자기 자신의 `cin >> count` 뒤에 오는 개행을 치우는
것이므로 정상). `system-test.ps1`의 기존 시나리오들은 우연히 넣어둔 여분의
`"0"`이 희생양으로 먹히는 바람에 이 버그를 못 잡고 있었다.

- [x] `system-test.ps1`에 시나리오 7 추가(`"5"` 다음 `"4"`를 바로 입력해 두 화면이
      모두 정상 표시되는지 확인)로 RED 재현 후, 두 컨트롤러의 불필요한 트레일링
      `skipToNextLine()` 호출을 제거해 GREEN 확인. 기존 시나리오 1~3에 있던, 이
      버그에 의존해 우연히 통과하던 여분의 `"0"`도 함께 제거.
- [x] `test.ps1`(GoogleTest 113개) + `system-test.ps1`(7개 시나리오) 전체 재확인.

## Phase 10 — 버그 수정: 숫자 입력 필드 실패 가드 누락 (완료)

Phase 9와 같은 관점(콘솔 배선은 GoogleTest로 못 잡는다)으로 `src/controller/`
전체의 `cin >>` 사용처를 재조사해 발견한 버그. 메뉴 번호(choice)를 읽는 4곳
(`ConsoleSubmenuController`/`OrderApprovalController`/`DummyDataController`)은
전부 `if (!(cin >> x)) { cin.clear(); x = 기본값; }` 가드가 있는데,
`SampleManagementController::registerSample()`의 평균생산시간/수율/재고 3곳과
`OrderIntakeController::registerOrder()`의 수량 1곳, 총 4개 숫자 필드에는 이
가드가 없었다.

비숫자 입력(예: "abc")이 들어오면 해당 `cin >>` 추출이 실패해 스트림이 fail
상태가 되고, 뒤이은 필드 읽기들도 조용히 스킵되어 기본값(0)으로 등록이
시도되다가 검증에서 거부되는 것까지는 괜찮지만, 실패를 유발한 입력 잔여분이
버퍼에 남아있다가 이후 메뉴 탐색(뒤로가기/종료 등)을 엉뚱하게 소비해 가짜
"유효하지 않은 선택입니다"/"유효하지 않은 메뉴 번호입니다" 오류가 끼어드는
현상을 확인했다 (완전한 무한루프까지는 아니었으나 명백한 오동작).

- [x] `system-test.ps1`에 시나리오 8 추가(시료 등록 중 평균생산시간에 "abc" 입력
      후 정상적으로 뒤로가기/종료까지 이어지는지 확인)로 RED 재현 후,
      `SampleManagementController`/`OrderIntakeController`의 4개 숫자 필드에
      기존과 동일한 가드 패턴(`cin.clear()` + 검증에서 자연히 거부되는 안전한
      기본값)을 적용해 GREEN 확인.
- [x] `test.ps1`(GoogleTest 113개) + `system-test.ps1`(8개 시나리오) 전체 재확인.

## Phase 11 — 테스트 실행 전용 서브에이전트 도입 (완료)

지금까지는 코드 변경 후 검증이 필요할 때 Git Bash 창을 여러 개 띄워 Claude Code를
동시에 여러 개 돌리는 식(멀티 스레드/창)으로 대응해왔다. 이를 대체해, 이 저장소
전용 서브에이전트 `test-runner`를 도입해 "코드 변경 → 두 테스트 스위트 실행 → 결과
보고"를 에이전트 위임으로 처리할 수 있게 했다.

- [x] `.claude/agents/test-runner.md` 신설 — `test.ps1`(GoogleTest, 로직 계층)과
      `system-test.ps1`(콘솔 시스템 테스트, 실행 파일 전체 플로우) **둘 다** 실행하고
      pass/fail을 정리해 보고하는 전용 에이전트. Phase 9/10에서 GoogleTest 단독으로는
      못 잡는 버그가 실제로 있었다는 점을 근거로, 두 스위트를 모두 돌리도록 명시.
      본인은 프로덕션 코드를 고치지 않고 결과 보고만 하도록 역할 한정(커밋/문서 수정도
      금지).
- [x] 새 세션에서부터 `test-runner`로 위임 가능(에이전트 정의는 세션 시작 시 로드되므로
      현재 세션에는 즉시 반영되지 않음을 확인).

각 Phase 내부는 `.claude/skills/test-driven-development/SKILL.md`의 Red-Green-Refactor
사이클을 기능 단위로 반복한다. Phase 착수 전 해당 docs/FEATURES/*.md의 Acceptance
Criteria를 먼저 테스트 목록으로 변환한 뒤 하나씩 구현한다.
