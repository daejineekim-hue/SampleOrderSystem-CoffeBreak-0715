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

Visual Studio에서 `SampleOrderSystem.sln`을 직접 열어 빌드/실행해도 되고, 커맨드라인에서는:

```powershell
./build.ps1     # App.exe + Tests.exe 빌드 (build/x64/Debug/)
./test.ps1      # 빌드 후 Tests.exe 실행 (GoogleTest)
./build/x64/Debug/App.exe
```

## 프로젝트 구조

```
SampleOrderSystem.sln
src/Core/     # 정적 라이브러리: model/view/controller/repository/production/json
src/App/      # 콘솔 실행파일 (main.cpp), Core 참조
test/         # GoogleTest/GoogleMock 실행파일, Core 참조 (NuGet gmock 1.11.0 사용)
packages/     # NuGet 패키지(gmock) 실물 — 재현성을 위해 git에 커밋됨
```

## 상태

Phase 0(프로젝트 초기 설정) 완료 — VS 솔루션/프로젝트 3개(Core/App/Tests) 구성,
GoogleTest/GoogleMock(NuGet `gmock` 1.11.0, Tests 프로젝트에만 참조) 연동, PoC2의
JsonValue 이식 및 회귀 테스트 포팅, 빌드·테스트 파이프라인 전체 검증 완료.
Phase 1(Sample 도메인 & 영속성)부터 TDD로 구현 예정.
