# SampleOrderSystem-CoffeBreak-0715

반도체 시료 생산주문관리 시스템 개인과제의 **본 프로젝트(미션2)**.

가상의 반도체 회사 "S-Semi"의 시료(Sample) 주문 접수 → 승인/거절 → (필요 시) 생산 →
출고 흐름을 콘솔에서 관리하는 애플리케이션이다. Visual Studio C++ 프로젝트, MVC
아키텍처, JSON 파일 영속성, GoogleTest 기반 TDD로 개발한다.

## 문서

- [PRD.md](PRD.md) — 요구사항 (배경, 도메인 모델, 기능 범위, 설계 결정)
- [PLAN.md](PLAN.md) — Phase별 구현 계획
- [CLAUDE.md](CLAUDE.md) — 코딩/프로세스 규칙 (TDD 절차, 커밋 워크플로우 포함)
- [docs/FEATURES/](docs/FEATURES/) — 기능별 상세 스펙 (Acceptance Criteria)

## 관련 PoC 저장소

이 프로젝트는 아래 4개 PoC에서 검증한 패턴을 재사용한다.

- `ConsoleMVC-CoffeBreak-0715` — MVC 계층 분리 구조
- `DataPersistence-CoffeBreak-0715` — JSON 파일 CRUD (`JsonValue` 파서/직렬화기)
- `DataMonitor-CoffeBreak-0715` — 실시간 콘솔 모니터링 도구
- `DummyDataGenerator-CoffeBreak-0715` — 더미 데이터 생성 도구

## 빌드 및 실행

프로젝트는 **하나**(`src/SampleOrderSystem.vcxproj`)이고, Configuration으로 App/테스트를
구분한다. Visual Studio에서 `SampleOrderSystem.sln`을 열고 상단 툴바에서 구성을
`Debug`/`Release`(App 실행) 또는 `Test`(GoogleTest 실행)로 바꿔 F5하면 된다.

커맨드라인에서는:

```powershell
./build.ps1 -Configuration Debug    # App 빌드 (build/x64/Debug/SampleOrderSystem.exe)
./build.ps1 -Configuration Test     # 테스트만 빌드 (build/x64/Test/SampleOrderSystem.exe)
./test.ps1                          # Test 구성 빌드 + 즉시 실행
```

## 프로젝트 구조

```
SampleOrderSystem.sln
src/SampleOrderSystem.vcxproj   # 프로젝트 1개
src/main.cpp                    # App 진입점 (Debug/Release에서만 컴파일)
src/model|view|controller|repository|production|generator|json/   # 항상 컴파일되는 공용 로직
src/test/                       # GoogleTest/GoogleMock 테스트 (Test 구성에서만 컴파일)
packages/                       # NuGet 패키지(gmock) 실물 — 재현성을 위해 git에 커밋됨
```

## 상태

Phase 0~6 전체 완료 (PLAN.md 참고). 시료 등록/조회/검색, 주문 접수, 승인/거절(재고
충분 시 즉시 확정, 부족 시 단일 FIFO 생산 라인 등록 및 지연 완료 판정), 모니터링
(상태별 주문 집계, 재고 여유/부족/고갈 판정), 출고 처리, 콘솔 메인 메뉴 라우팅,
더미 데이터 생성(숨김 메뉴)까지 전 기능이 GoogleTest 기반 TDD로 구현되어 있다
(비즈니스 로직 전 계층 테스트 커버, 콘솔 I/O 자체는 `docs/FEATURES/main-menu.md`의
Testability note에 따라 수동 검증). `SampleOrderSystem.exe`를 직접 실행해 전체
플로우(등록 → 주문 → 승인/생산 → 모니터링 → 출고)를 확인할 수 있다.
