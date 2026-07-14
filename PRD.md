# PRD — 반도체 시료 생산주문관리 시스템 (SampleOrderSystem)

## 배경

가상의 반도체 회사 "S-Semi"는 다양한 반도체 시료(Sample)를 생산해 연구소·팹리스·대학
연구실 등에 납품한다. 최근 주문량 급증으로 엑셀/메모장 기반 관리의 한계(주문 처리
여부 파악 불가, 공정 완료 시점 불명확, 불필요한 추가 공정)가 드러났다. 이를 해결하기
위해 콘솔 기반의 시료 생산주문관리 시스템을 개발한다.

## 사용자 (역할)

- **고객**: 이메일 등으로 필요한 시료를 요청 (시스템 외부 행위, 범위 밖)
- **주문 담당자**: 요청에 맞게 주문서(RESERVED 주문) 작성, 출고 처리
- **생산 담당자**: 개발 시료 등록, 주문 승인/거절, 생산 라인 관리

시스템은 단일 콘솔 프로세스로 두 담당자 역할을 모두 수행할 수 있게 한다(역할별 로그인
분리는 범위 밖).

## 도메인 모델

- **Sample(시료)**: id, name, avgProcessTimeMin(평균 생산시간), yieldRate(수율),
  stock(재고). 자세한 내용은 [docs/FEATURES/sample-management.md](docs/FEATURES/sample-management.md)
- **Order(주문)**: orderId, sampleId, customerName, quantity, status, createdAt,
  (PRODUCING 상태일 때) productionStartedAt/producedTotal/productionTimeMin.
  상태: RESERVED → (REJECTED | CONFIRMED | PRODUCING → CONFIRMED) → RELEASE

## 주문 상태 흐름

```
RESERVED --거절--> REJECTED (종단 상태, 모니터링 제외)
RESERVED --승인, 재고충분--> CONFIRMED --출고--> RELEASE
RESERVED --승인, 재고부족--> PRODUCING --생산완료--> CONFIRMED --출고--> RELEASE
```

## 기능 범위 (Chapter 2 기능 명세 대응)

| # | 메뉴 | 상세 스펙 |
|---|------|-----------|
| 1 | 시료 관리 | [sample-management.md](docs/FEATURES/sample-management.md) |
| 2 | 시료 주문(접수) | [order-intake.md](docs/FEATURES/order-intake.md) |
| 3 | 주문 승인/거절 | [order-approval.md](docs/FEATURES/order-approval.md) |
| 4 | 모니터링 | [monitoring.md](docs/FEATURES/monitoring.md) |
| 5 | 생산 라인 조회 | [production-line.md](docs/FEATURES/production-line.md) |
| 6 | 출고 처리 | [shipment.md](docs/FEATURES/shipment.md) |
| — | 메인 메뉴/통합 | [main-menu.md](docs/FEATURES/main-menu.md) |

각 문서는 Given-When-Then 형태의 Acceptance Criteria를 포함하며, GoogleTest 테스트
케이스로 1:1에 가깝게 변환 가능하도록 작성한다 (TDD 전제조건).

## 비기능 요구사항 / 설계 결정

- **플랫폼**: Windows, Visual Studio 2026(내부 버전 18) C++ 프로젝트, MSVC 툴체인
  (VC/Tools/MSVC 14.51 확인됨), MSBuild + vstest 기반 빌드/테스트.
- **아키텍처**: MVC (Model/View/Controller 패키지 분리) — PoC1(ConsoleMVC)에서 검증한
  구조를 확장.
- **영속성**: JSON 파일. PoC2(DataPersistence)에서 검증한 자체 구현 `JsonValue`
  파서/직렬화기를 vendoring하여 재사용 (외부 라이브러리 의존성 없음).
- **테스트**: GoogleTest, TDD(Red-Green-Refactor)로 개발. `.claude/skills/test-driven-development/SKILL.md`
  준수.
- **생산 진행 시간 처리**: 백그라운드 스레드 없이, 조회 시점에 경과시간을 계산해
  완료 여부를 판정하는 lazy 방식 (자세한 내용은 production-line.md).
- **문서 관리**: PRD.md(본 문서, 요구사항) / PLAN.md(Phase+TODO) / CLAUDE.md(코딩·프로세스
  규칙) / docs/FEATURES/*.md(기능별 상세 스펙) / docs/adr/(주요 설계 결정 기록, 필요 시).

## 범위 밖 (Out of Scope)

- 다중 생산 라인 (스펙상 "하나의 생산 라인은 시료를 하나씩 생산"으로 단일 라인 고정)
- 사용자 인증/권한 분리
- 네트워크/DB 서버, GUI

## 제출 요건과의 연결

이 프로젝트는 개인과제 미션2(반도체 시료 생산주문관리 시스템, 저장소명
`SampleOrderSystem-KimDaejin-03086508`)에 해당하며, PoC 4종(ConsoleMVC/DataPersistence/
DataMonitor/DummyDataGenerator)에서 검증된 패턴을 재사용해 구현한다.
