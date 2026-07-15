# ADR-0002: "생산라인 대기 건수" 요약 지표의 정의

## Status

Accepted (Phase 5).

## Context

`docs/FEATURES/main-menu.md`는 메인 메뉴에 "생산라인 대기 건수"를 표시하라고 요구하지만,
정확한 산출 기준은 "production-line.md/monitoring.md에서 정의한 대기 상태 기준"이라고만
적혀 있고 구체적인 공식은 없다.

## Decision

`monitoring.md`가 이미 확정한 `pendingDemand`(미출고 대기 수량) 정의, 즉 `RESERVED` 또는
`PRODUCING` 상태 주문을 "아직 재고 확보가 필요한 실질적 대기 수요"로 보는 기준을 그대로
재사용한다. "생산라인 대기 건수"는 `RESERVED` + `PRODUCING` 상태 주문의 **건수**(수량
합계가 아님)로 정의한다 (`MenuSummaryProvider::getSummary`).

## Consequences

- 별도의 새 집계 규칙을 만들지 않고 기존 monitoring.md 정의를 재사용하므로 두 화면
  (모니터링/메인 메뉴)의 기준이 어긋나지 않는다.
- "건수"와 "수량 합계"를 혼동하지 않도록 `MenuSummary.productionWaitingCount`라는
  이름으로 명시했다 (`pendingDemand`처럼 수량 합계가 아님).
