# 시료 주문 접수 (Order Intake)

## Overview

반도체 시료 생산주문관리 시스템에서, 고객이 시료(Sample)를 요청하면 주문 담당자가 이를 접수하여 새로운 주문(Order)을 생성한다. 이 문서는 주문 접수(register order) 기능의 데이터 모델, 주문번호(Order ID) 생성 규칙, 그리고 TDD로 테스트를 도출할 수 있도록 구체적인 인수 조건(Acceptance Criteria, Given-When-Then)을 정의한다.

주문이 접수되는 시점의 상태는 항상 `RESERVED`(접수)이다. 이후 상태 전이(승인/거절/생산/출고 등)는 이 문서의 범위가 아니며, 관련 문서를 참조한다 (Non-Goals 참고).

## Data Model

### Order

| 필드 | 타입 | 설명 | 검증(Validation) |
|---|---|---|---|
| `orderId` | `string` | 주문을 식별하는 고유 ID. 시스템이 자동 생성하며 사용자가 직접 입력하지 않는다. | `ORD-YYYYMMDD-####` 형식 고정 (아래 "Order ID generation rule" 참고). 생성 후 불변(immutable). |
| `sampleId` | `string` | 주문 대상 시료의 ID. `docs/FEATURES/sample-management.md`에 정의된 Sample 엔티티를 참조하는 외래키(reference) 성격의 필드. | 비어 있지 않아야 함. 시스템에 이미 등록된(existing) Sample의 ID와 일치해야 함. 등록되지 않은 `sampleId`인 경우 주문 생성 거부. |
| `customerName` | `string` | 주문을 요청한 고객명. | 비어 있지 않아야 함(non-empty). 공백만으로 구성된 문자열도 비어 있는 것으로 간주(trim 후 길이 0이면 거부). |
| `quantity` | `int` (정수) | 고객이 요청한 시료 수량. | 1 이상의 양의 정수(positive integer)여야 함. 0 이하이거나 정수가 아닌 값(해당 언어/입력 계층에서 검증 가능한 범위 내)은 거부. |
| `status` | `enum` (`OrderStatus`) | 주문의 현재 상태. 전체 값: `RESERVED`, `REJECTED`, `PRODUCING`, `CONFIRMED`, `RELEASE`. | 주문 접수 시점에는 항상 `RESERVED`로 고정 생성되며, 다른 값으로 초기화할 수 없다. `REJECTED` 상태의 주문은 모니터링(현황 조회/집계) 대상에서 제외된다 (다른 기능 문서에서 다룸). |
| `createdAt` | `datetime` (timestamp) | 주문이 접수(생성)된 일시. | 시스템이 주문 생성 시점에 자동으로 채움. 사용자가 임의로 지정할 수 없음. Order ID의 날짜 부분(`YYYYMMDD`)과 동일한 날짜여야 함. |

> 참고: `OrderStatus`의 나머지 값(`REJECTED`, `PRODUCING`, `CONFIRMED`, `RELEASE`)에 대한 전이 규칙은 `docs/FEATURES/order-approval.md`, `docs/FEATURES/production-line.md` 등 별도 문서에서 다룬다. 이 문서는 `RESERVED` 상태로의 최초 생성(접수)만 다룬다.

## Order ID generation rule

### 형식

```
ORD-YYYYMMDD-####
```

- `ORD-` : 고정 접두사.
- `YYYYMMDD` : 주문이 접수된 날짜 (예: `20260416` = 2026년 4월 16일).
- `####` : 해당 날짜 내에서 주문이 접수된 순서를 나타내는 4자리 zero-padded 순번(sequence number). `0001`부터 시작.

예시: `ORD-20260416-0043` → 2026년 4월 16일에 접수된 43번째 주문.

### 순번(counter) 규칙

- 순번은 **일자별로 독립적으로 관리**된다. 날짜가 바뀌면 순번은 `0001`로 초기화된다.
- 순번은 해당 날짜에 접수(생성)된 주문 개수를 기준으로 1씩 증가한다. 즉, N번째로 성공적으로 생성된 주문은 순번 `N`을 부여받는다.
- 순번은 주문 생성이 **성공적으로 완료된 경우에만** 소비(증가)된다. 유효성 검증에 실패하여 주문이 생성되지 않은 시도는 순번을 소비하지 않는다 (예: 3번째 시도가 실패하면 다음 성공 주문도 여전히 `0003`을 받는다).
- 동시성(concurrent) 환경에서 두 개 이상의 주문이 동시에 접수 요청되더라도, 각 주문은 서로 다른 순번을 받아야 하며 동일한 `orderId`가 두 번 이상 발급되어서는 안 된다 (collision 방지). 구현은 날짜별 카운터에 대한 원자적(atomic) 증가 또는 동등한 직렬화(serialization) 메커니즘을 사용해야 한다.
- 하루 최대 순번은 `9999`(4자리 zero-padded 범위)이며, 이를 초과하는 경우의 동작은 이 문서의 범위 밖이다(별도 정책 필요 시 추후 문서화).

## Operation: 주문 예약 (register order)

고객이 원하는 시료와 수량을 지정하여 주문 담당자가 주문을 생성하는 동작. 입력 값: `sampleId`, `customerName`, `quantity`. 생성 결과: `status = RESERVED`인 새 `Order` (시스템이 `orderId`와 `createdAt`을 자동 부여).

### Acceptance Criteria

**AC-1: 정상 접수 (happy path) — RESERVED 주문 생성**

- Given 시스템에 `sampleId = "SMP-001"`로 등록된 유효한 Sample이 존재하고
- And 오늘 날짜에 아직 접수된 주문이 없을 때
- When 주문 담당자가 `sampleId = "SMP-001"`, `customerName = "홍길동"`, `quantity = 10`으로 주문 접수를 요청하면
- Then 새로운 Order가 생성되고
- And 생성된 Order의 `status`는 `RESERVED`이며
- And `orderId`는 `ORD-YYYYMMDD-0001` 형식(오늘 날짜, 순번 `0001`)을 따르고
- And `createdAt`은 현재(접수) 일시로 설정된다.

**AC-2: 동일 일자 내 순번 증가**

- Given 오늘 날짜에 이미 2건의 주문이 성공적으로 접수되어 순번이 `0002`까지 부여된 상태에서
- When 세 번째 주문 접수 요청이 성공하면
- Then 새 Order의 `orderId`는 동일한 날짜에 순번 `0003`을 사용해 생성된다 (예: `ORD-20260416-0003`).

**AC-3: 존재하지 않는 sampleId — 거부**

- Given 시스템에 `sampleId = "SMP-999"`로 등록된 Sample이 존재하지 않을 때
- When 주문 담당자가 `sampleId = "SMP-999"`로 주문 접수를 요청하면
- Then 주문 생성이 거부되고
- And "등록되지 않은 시료 ID입니다" 취지의 명확한 오류(error)가 반환되며
- And Order는 생성되지 않고, 해당 날짜의 순번도 증가하지 않는다.

**AC-4: 수량이 0 — 거부**

- Given 유효한 `sampleId`와 `customerName`이 주어졌을 때
- When `quantity = 0`으로 주문 접수를 요청하면
- Then 주문 생성이 거부되고
- And "수량은 1 이상의 양의 정수여야 합니다" 취지의 명확한 오류가 반환되며
- And Order는 생성되지 않는다.

**AC-5: 수량이 음수 — 거부**

- Given 유효한 `sampleId`와 `customerName`이 주어졌을 때
- When `quantity = -5`로 주문 접수를 요청하면
- Then 주문 생성이 거부되고
- And 수량 유효성 오류가 반환되며
- And Order는 생성되지 않는다.

**AC-6: customerName이 빈 문자열 — 거부**

- Given 유효한 `sampleId`와 `quantity`가 주어졌을 때
- When `customerName = ""`으로 주문 접수를 요청하면
- Then 주문 생성이 거부되고
- And "고객명을 입력해야 합니다" 취지의 명확한 오류가 반환되며
- And Order는 생성되지 않는다.

**AC-7: customerName이 공백 문자로만 구성 — 거부**

- Given 유효한 `sampleId`와 `quantity`가 주어졌을 때
- When `customerName = "   "` (공백만 포함)으로 주문 접수를 요청하면
- Then 주문 생성이 거부되고
- And customerName 유효성 오류가 반환되며
- And Order는 생성되지 않는다.

**AC-8: 날짜가 바뀌면 순번이 초기화됨**

- Given 어제 날짜에 마지막으로 접수된 주문의 순번이 `0007`이었고
- When 오늘 날짜로 첫 주문 접수가 성공하면
- Then 새 Order의 `orderId`는 오늘 날짜와 순번 `0001`로 생성된다 (예: 어제 `ORD-20260415-0007` 다음, 오늘 `ORD-20260416-0001`).

**AC-9: 여러 유효성 오류가 동시에 존재하는 경우**

- Given 존재하지 않는 `sampleId`와 `quantity = 0`이 동시에 주어졌을 때
- When 주문 접수를 요청하면
- Then 주문 생성이 거부되고
- And 최소 하나 이상의 명확한 오류가 반환되며 (구현체는 첫 번째로 발견된 검증 실패 또는 모든 검증 실패를 함께 보고할 수 있음 — 상세 정책은 구현 문서에서 확정)
- And Order는 생성되지 않는다.

## Non-Goals

이 문서는 주문의 **접수(생성, RESERVED 상태 진입)** 만을 다룬다. 다음 항목은 이 문서의 범위가 아니며 별도 문서에서 다룬다:

- 주문 승인/거절(`RESERVED` → `PRODUCING` 또는 `REJECTED` 전이) 및 관련 정책: `docs/FEATURES/order-approval.md`
- 생산 진행(`PRODUCING`), 생산 완료 후 출고대기(`CONFIRMED`) 전환, 출고완료(`RELEASE`) 처리 등 생산 라인 관련 동작: `docs/FEATURES/production-line.md`
- Sample 엔티티 자체의 등록/관리(주문 접수 시 참조만 함): `docs/FEATURES/sample-management.md`
- `REJECTED` 상태 주문의 모니터링 제외 처리 로직(집계/조회 화면 등): 해당 모니터링 기능 문서
