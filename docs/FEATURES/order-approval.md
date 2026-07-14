# 주문 승인/거절 (Order Approval/Rejection)

## Overview

접수된 주문(RESERVED 상태) 목록을 확인하고, 특정 주문에 대해 승인 또는 거절 처리를 수행하는 기능이다.

- **접수된 주문 목록 조회**: RESERVED 상태의 주문만 필터링하여 화면에 표시한다.
- **주문 승인**: 승인 시 시료(sample)의 현재 재고(stock)와 주문 수량(quantity)을 비교하여, 시스템이 자동으로 아래 두 가지 처리 경로 중 하나를 선택한다.
  - 재고 충분 → 즉시 `CONFIRMED`로 전환 + 재고 차감
  - 재고 부족 → 생산 라인에 등록 + `PRODUCING`으로 전환
- **주문 거절**: 승인/거절 판단에 대해 거절을 선택한 경우, 재고 변동 없이 즉시 `REJECTED`로 전환한다.

본 문서는 "승인/거절 처리 시점의 주문 상태 전이와 재고 반영"까지를 다룬다. 승인 이후 생산 큐 내부의 실생산량·생산시간 계산, 생산 완료 후 출고 처리 등은 별도 문서에서 다룬다 (아래 Non-Goals 참조).

## Preconditions

- 승인/거절 대상 주문은 **반드시 RESERVED 상태**여야 한다.
- 주문이 RESERVED 상태가 아닌 경우(예: 이미 `CONFIRMED`, `PRODUCING`, `REJECTED`, `SHIPPED` 등) 승인/거절 요청은 **잘못된 작업(invalid operation)으로 간주하여 거부**하며, 해당 주문의 상태는 변경되지 않는다.
- 승인 판단 시점의 `sample.stock` 값을 기준으로 재고 충분/부족 여부를 판정한다.

## 접수된 주문 목록 Display

### Acceptance Criteria

- **Given** 시스템에 RESERVED 상태의 주문 3건과 그 외 상태(CONFIRMED, PRODUCING, REJECTED)의 주문이 섞여 존재할 때
  **When** 접수된 주문 목록을 조회하면
  **Then** RESERVED 상태인 주문 3건만 목록에 표시된다.

- **Given** RESERVED 상태의 주문이 하나도 존재하지 않을 때
  **When** 접수된 주문 목록을 조회하면
  **Then** 빈 목록이 반환된다 (오류가 아님).

## Operation: 주문 승인

### 분기 1 — 재고 충분 (stock >= quantity)

#### Acceptance Criteria

- **Given** RESERVED 상태의 주문 O가 시료 S에 대해 quantity=10을 요청하고, S.stock=15일 때
  **When** 주문 O를 승인하면
  **Then** 주문 O의 상태는 즉시 `CONFIRMED`로 전환되고, S.stock은 5(=15-10)로 차감된다.

- **경계값(Boundary) — Given** RESERVED 상태의 주문 O가 quantity=10을 요청하고, S.stock=10일 때 (stock == quantity)
  **When** 주문 O를 승인하면
  **Then** 재고가 "충분한" 것으로 판정되어 주문 O는 `CONFIRMED`로 전환되고, S.stock은 0으로 차감된다. (`PRODUCING`으로 전환되지 않아야 한다.)

- **Given** 재고 충분 분기로 승인 처리된 직후
  **When** 주문 O의 상태와 S.stock을 조회하면
  **Then** 생산 라인에는 아무 항목도 등록되지 않는다.

### 분기 2 — 재고 부족 (stock < quantity)

#### Acceptance Criteria

- **Given** RESERVED 상태의 주문 O가 시료 S에 대해 quantity=10을 요청하고, S.stock=4일 때
  **When** 주문 O를 승인하면
  **Then** 부족분 shortage = quantity - stock = 6으로 계산되고, 주문 O는 생산 라인에 자동으로 등록되며, 주문 O의 상태는 `PRODUCING`으로 전환된다.

- **Given** S.stock=0이고 주문 O의 quantity=5일 때
  **When** 주문 O를 승인하면
  **Then** shortage = 5로 계산되고, 주문 O는 `PRODUCING`으로 전환되며 생산 큐에 등록된다.

- **Given** 재고 부족 분기로 승인 처리된 직후
  **When** S.stock을 조회하면
  **Then** S.stock은 승인 시점 값(예: 4)에서 변경되지 않는다. (재고 차감은 재고 충분 분기에서만 발생하며, 부족 분기에서는 재고를 건드리지 않는다.)

- **Note**: 생산 큐에 등록된 항목의 실제 생산 완료 시점 재고 갱신, 실생산량/생산시간 산출 공식은 `docs/FEATURES/production-line.md`에서 다룬다. 본 문서는 "승인 시점에 PRODUCING으로 전환되고 생산 큐에 등록된다"는 사실까지만 명세한다.

## Operation: 주문 거절

### Acceptance Criteria

- **Given** RESERVED 상태의 주문 O가 시료 S에 대해 quantity=10을 요청하고, S.stock=15일 때
  **When** 주문 O를 거절하면
  **Then** 주문 O의 상태는 즉시 `REJECTED`로 전환된다.

- **Given** 주문 O를 거절 처리한 직후
  **When** S.stock을 조회하면
  **Then** S.stock은 거절 이전 값(15)과 동일하다. (재고에 어떠한 영향도 주지 않는다.)

- **Given** 주문 O를 거절 처리한 직후
  **When** 생산 라인 상태를 조회하면
  **Then** 주문 O에 대한 생산 큐 등록이 존재하지 않는다.

## Edge Cases

- **Given** 이미 `CONFIRMED` 상태인 주문 O
  **When** 주문 O를 승인하려고 시도하면
  **Then** 잘못된 작업으로 처리되어 실패하며(예외/에러 반환), 주문 O의 상태와 관련 시료의 재고는 변경되지 않는다.

- **Given** 이미 `REJECTED` 상태인 주문 O
  **When** 주문 O를 승인하려고 시도하면
  **Then** 잘못된 작업으로 처리되어 실패하며, 상태는 `REJECTED`로 유지된다.

- **Given** 이미 `PRODUCING` 상태인 주문 O
  **When** 주문 O를 거절하려고 시도하면
  **Then** 잘못된 작업으로 처리되어 실패하며, 상태는 `PRODUCING`으로 유지되고 생산 큐 등록에도 변화가 없다.

- **Given** 이미 `CONFIRMED` 상태인 주문 O
  **When** 주문 O를 거절하려고 시도하면
  **Then** 잘못된 작업으로 처리되어 실패하며, 상태는 `CONFIRMED`로 유지되고 재고에도 변화가 없다.

- **Given** 존재하지 않는 주문 ID로 승인/거절을 시도할 때
  **When** 해당 요청을 처리하면
  **Then** 잘못된 요청으로 처리되어 실패하며, 어떠한 주문/재고 상태도 변경되지 않는다.

## Non-Goals

- 생산 라인 내부의 실생산량(actual production quantity) 및 생산 소요 시간 계산 공식 → `docs/FEATURES/production-line.md`에서 다룸.
- 생산 완료 이후의 출고(shipment) 처리 → `docs/FEATURES/shipment.md`에서 다룸.
