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

(Phase 0 완료 후 작성 예정: Visual Studio 솔루션 열기 / MSBuild 명령 / 테스트 실행 명령)

## 상태

현재 준비 단계 — 문서(PRD/PLAN/CLAUDE.md/FEATURES) 작성 완료, Phase 0(프로젝트 초기
설정)부터 순차 구현 예정.
