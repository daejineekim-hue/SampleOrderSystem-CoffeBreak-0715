# SampleOrderSystem-KimDaejin-03086508

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

- `ConsoleMVC-KimDaejin-03086508` — MVC 계층 분리 구조
- `DataPersistence-KimDaejin-03086508` — JSON 파일 CRUD (`JsonValue` 파서/직렬화기)
- `DataMonitor-KimDaejin-03086508` — 실시간 콘솔 모니터링 도구
- `DummyDataGenerator-KimDaejin-03086508` — 더미 데이터 생성 도구

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
src/model|view|controller|repository|production|json/   # 항상 컴파일되는 공용 로직
src/test/                       # GoogleTest/GoogleMock 테스트 (Test 구성에서만 컴파일)
packages/                       # NuGet 패키지(gmock) 실물 — 재현성을 위해 git에 커밋됨
```

## 상태

Phase 0(프로젝트 초기 설정) 완료 — 단일 VS 프로젝트 + Debug/Release/Test 3-구성
구조(App/테스트를 프로젝트가 아닌 Configuration으로 전환), GoogleTest/GoogleMock
(NuGet `gmock` 1.11.0, Test 구성에만 연결) 연동, PoC2의 JsonValue 이식 및 회귀
테스트 포팅, 세 구성 모두 빌드·실행 검증 완료. Phase 1(Sample 도메인 & 영속성)부터
TDD로 구현 예정.
