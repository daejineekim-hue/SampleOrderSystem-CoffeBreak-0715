# ADR-0001: productionStartedAt/producedTotal/productionTimeMin live in ProductionLine, not on Order

## Status

Accepted (Phase 3).

## Context

`PRD.md`의 도메인 모델 절과 `docs/FEATURES/production-line.md`의 "Data 추가 사항"은
`productionStartedAt`/`producedTotal`/`productionTimeMin`을 `Order` 엔티티의 확장
필드로 기술한다. 그러나 같은 문서의 Non-Goals는 이 필드들의 영속성(파일 저장/복원)을
명시적으로 범위 밖으로 둔다.

## Decision

이 세 필드는 `Order` 구조체에 추가하지 않고, `ProductionLine`의 내부 `QueueEntry`에만
보관한다. `ProductionLine`은 큐 조회가 필요할 때마다 `OrderRepository::findById`로
`sampleId`/`quantity` 등 나머지 정보를 조회해 조합한다.

## Consequences

- `Order.h`/`OrderRepositoryTest`/JSON 스키마가 Phase 2 이후로 변경되지 않는다
  (영속성 비목표와 자연스럽게 일치).
- 생산 큐 관련 상태는 오직 `ProductionLine` 인스턴스 생존 기간에만 존재한다 — 이는
  이미 다른 Non-Goal("영속성")과 일치하므로 앱을 재시작하면 생산 큐 상태는
  소실된다(재시작 시 진행 중이던 생산은 다시 시작되지 않음). 향후 영속성이
  필요해지면 이 필드들을 `Order`로 옮기고 JSON 스키마를 갱신해야 한다.
- `docs/FEATURES/production-line.md`의 Acceptance Criteria는 필드가 어디 저장되는지가
  아니라 동작(상태 전이/재고 반영/진행률 조회)만 검증하므로 테스트에는 영향이 없다.
