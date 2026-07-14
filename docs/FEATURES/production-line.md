# 생산 라인 (Production Line)

## Overview

생산 라인(Production Line)은 주문(Order)이 승인되었으나 재고(stock)가 주문 수량(quantity)에 미치지 못할 때,
부족분(shortage)만큼 시료를 생산하는 도메인이다. 생산에는 시료의 **수율(yieldRate)** 과, 실제 생산 현장에서 발생하는
**추가 손실/오차를 반영하는 보정 계수 `0.9`** 가 적용되어, 요청한 부족분보다 더 많은 양을 생산해야 한다.

생산 라인은 **단일 라인(single production line)** 으로 구성되며, 한 번에 **하나의 주문만 실제로 생산(producing) 중**일 수 있다.
나머지 주문들은 생산 큐(production queue)에서 **FIFO(선입선출)** 방식으로 대기한다.

본 시스템은 콘솔 애플리케이션이며 백그라운드 스레드/타이머를 사용하지 않는다. 따라서 생산 완료 판정은
**조회 시점에 지연 계산(lazy evaluation)** 하는 방식(`advanceIfDue()`)으로 설계한다. 이 설계 결정은 개발 계획서에서
이미 확정된 사항이며, 본 문서는 이를 전제로 인수 기준을 기술한다.

이 문서는 이후 GoogleTest 테스트 케이스로 변환되는 것을 전제로 하므로, 모든 동작은 Given-When-Then 형식으로
구체적인 수치 예제와 함께 기술한다.

---

## Formulas

### 1. 실 생산량 (producedTotal)

```
producedTotal = ceil( shortage / (yieldRate * 0.9) )
```

- `shortage` = `quantity - stock` (주문 수량 - 생산 착수 시점의 재고). 항상 0보다 커야 한다 (Edge Cases 참고).
- `yieldRate * 0.9`: 수율에 생산 현장의 오차 보정 계수(0.9)를 곱한 **실효 수율(effective yield)**.
- 결과는 정수 단위 생산 개수이므로 올림(ceiling) 처리한다.

### 2. 총 생산 시간 (productionTimeMin)

```
productionTimeMin = avgProcessTimeMin * producedTotal
```

- `avgProcessTimeMin`: 해당 시료의 평균 생산시간(분), `Sample` 엔티티에 정의됨 (`sample-management.md` 참고).
- 생산 개수가 늘어난 만큼 총 생산 시간도 비례해서 늘어난다.

### 3. 생산 완료 시 재고/상태 반영

```
order.status: PRODUCING -> CONFIRMED
sample.stock = sample.stock + producedTotal   // 생산분 입고
sample.stock = sample.stock - order.quantity  // 주문 수량만큼 출고
```

즉 최종적으로 `stock`은 `producedTotal - order.quantity` 만큼 순증가(net increase)한다.
(`producedTotal >= shortage = quantity - stock_당시` 이므로 결과 재고는 0 이상이 보장된다.)

### 공식 예제 (Worked Example) — 전 과정 계산

과제 공식 예제 문서에서 제시된 값:

| 항목 | 값 |
|---|---|
| 주문 수량 (`quantity`) | 80 |
| 생산 착수 시점 재고 (`stock`) | 30 |
| 부족분 (`shortage = quantity - stock`) | `80 - 30 = 50` |
| 수율 (`yieldRate`) | 0.92 |
| 오차 보정 계수 | 0.9 |

계산 과정:

1. 실효 수율 = `yieldRate * 0.9` = `0.92 * 0.9` = `0.828`
2. 미보정 필요량 = `shortage / 실효수율` = `50 / 0.828` = `60.3864734...`
3. `producedTotal = ceil(60.3864734...)` = **`61`** (ea)
4. `productionTimeMin = avgProcessTimeMin * 61` (예: `avgProcessTimeMin = 20`이면 `20 * 61 = 1220`분)
5. 생산 완료 후 재고: `stock = 30 + 61 - 80 = 11`
   - 순증가 = `producedTotal - quantity = 61 - 80 = -19`(재고 관점에서는 `30 -> 11`로 계산 일치 확인:
     `30(기존) + 61(입고) - 80(출고) = 11`)

이 예제는 GoogleTest 테스트 케이스 `ProductionLineFormulaTest.WorkedExample_Shortage50_Yield092` 등으로
그대로 옮겨 검증한다.

### 보조 예제 (나머지 없이 정확히 나누어떨어지는 경우 — 올림 경계 검증)

| 항목 | 값 |
|---|---|
| 부족분 (`shortage`) | 81 |
| 수율 (`yieldRate`) | 0.9 |

1. 실효 수율 = `0.9 * 0.9 = 0.81`
2. `81 / 0.81 = 100.0` (오차 없이 정확히 나누어떨어짐)
3. `producedTotal = ceil(100.0) = 100` — **정수로 정확히 나누어떨어질 때 불필요하게 101로 올림되지 않아야 함**
   (부동소수점 오차로 인해 `100.00000001` 같은 값이 발생해 `101`로 잘못 올림되지 않도록 구현 시 주의.
   테스트에서 이 경계값을 명시적으로 검증한다.)

---

## Data 추가 사항 (Order 확장 필드)

기존 `Order` 엔티티(주문 관리 문서에서 정의)에 아래 필드를 추가한다.

| 필드 | 타입 | 설명 | 비고 |
|---|---|---|---|
| `productionStartedAt` | `time_point` (예: `std::chrono::system_clock::time_point`) | 해당 주문이 생산 큐에서 **실제로 생산 시작**된 시각 | 큐에 들어간 시각이 아니라, **자신의 차례가 되어 생산이 개시된 시각**. FIFO 진행에 따라 갱신될 수 있음 |
| `producedTotal` | `int` | 이번 생산으로 만들어질 실 생산량 | `shortage`, `yieldRate` 확정 시점(생산 개시 시)에 계산되어 고정됨 |
| `productionTimeMin` | `double`(또는 `int`, 분 단위) | 이번 생산에 필요한 총 생산 시간(분) | `avgProcessTimeMin * producedTotal`로 계산되어 고정됨 |

`producedTotal`, `productionTimeMin`은 주문이 `PRODUCING` 상태로 전환되며 생산 큐의 선두(head)가 되어 실제 생산이
개시되는 시점에 1회 계산되어 저장된다. 이후 값은 불변(immutable)이며, 생산 완료 판정 시 재계산하지 않는다.

---

## FIFO 큐 시맨틱 (단일 생산 라인)

- 생산 큐(production queue)는 재고 부족으로 `PRODUCING` 상태가 된 주문들을 담는 대기열이다.
- 생산 라인은 **하나**이므로, 큐의 **선두(head) 주문 단 하나만** 실제로 "생산 중(active producing)"이며
  `productionStartedAt`, `producedTotal`, `productionTimeMin`이 계산되어 진행 중이다.
- 큐의 나머지 주문들은 "대기(waiting)" 상태로, 아직 `productionStartedAt`이 설정되지 않았거나(또는 개시 전 placeholder)
  실제 생산이 시작되지 않은 것으로 취급한다.
- 선두 주문이 완료(`CONFIRMED`로 전환)되면, 큐에서 제거되고 **다음 대기 주문이 새로운 선두**가 되며,
  그 주문의 `productionStartedAt`은 **완료가 감지된 바로 그 시각(now)** 으로 설정된다. (원래 큐에 들어간 시각이 아님)
- 정렬 기준은 순수 FIFO: 큐에 들어간 순서(주문 생성/승인 순서) 그대로 처리하며, 우선순위/재정렬은 없다.

---

## `advanceIfDue()` 동작

`advanceIfDue()`는 생산 상태를 조회하거나 다른 메뉴에서 생산 상태를 참조할 때마다 호출되는 공용 함수로,
"경과 시간이 총 생산 시간을 넘겼는지"를 판정하여 필요 시 상태 전이를 수행한다.

판정식: 큐 선두 주문에 대해 `elapsed = now - productionStartedAt` 을 계산하고,
- `elapsed < productionTimeMin` 이면 아직 생산 중 (변화 없음)
- `elapsed >= productionTimeMin` 이면 생산 완료 처리 수행

완료 처리 시 수행 작업(순서대로):

1. 해당 주문 상태를 `PRODUCING -> CONFIRMED`로 전환
2. 시료 재고에 `producedTotal`을 더하고, 이어서 `quantity`만큼 차감 (`Formulas` 3번 참고)
3. 해당 주문을 생산 큐에서 제거
4. 큐에 다음 대기 주문이 있다면, 그 주문을 새 선두로 승격하고 `productionStartedAt = now`로 설정,
   `shortage`/`producedTotal`/`productionTimeMin`을 이 시점의 재고 기준으로 계산하여 고정
   (직전 주문 처리로 재고가 변동되었으므로, 다음 주문의 `shortage`는 갱신된 재고 기준으로 재계산됨)
5. 만약 완료 후 새로 승격된 주문도 이미 완료 조건을 만족한다면(예: 조회가 한참 뒤에 이루어진 경우),
   동일한 판정을 **연쇄적으로(cascade)** 반복 적용하여 큐를 계속 전진시킨다.

### Acceptance Criteria

- **[아직 미완료 — PRODUCING 유지, 진행률 표시]**
  - Given 주문 `"O001"`이 생산 큐 선두이며 `productionStartedAt = T0`, `productionTimeMin = 100`(분)로 생산 중일 때
  - When 현재 시각이 `T0 + 40분`인 시점에 생산 현황을 조회하면(`advanceIfDue()` 내부 호출)
  - Then 주문 `"O001"`의 상태는 여전히 `PRODUCING`이며, 진행률은 `40 / 100 = 40%`로 표시된다.

- **[정확히 경계 시각 — 완료 처리]**
  - Given 주문 `"O001"`의 `productionTimeMin = 100`이고 `productionStartedAt = T0`일 때
  - When 현재 시각이 정확히 `T0 + 100분`인 시점에 조회하면
  - Then `elapsed(100) >= productionTimeMin(100)` 이므로 완료로 판정되고 `CONFIRMED`로 전환된다.

- **[완료 — 상태 전환 및 재고 반영]**
  - Given 주문 `"O001"`(`quantity=80`, 생산 개시 시점 재고 `stock=30`, `yieldRate=0.92`, `producedTotal=61`)이
    `productionTimeMin`을 초과한 상태일 때
  - When 조회를 통해 `advanceIfDue()`가 호출되면
  - Then `"O001"`의 상태는 `CONFIRMED`로 전환되고, 시료 재고는 `30 + 61 - 80 = 11`로 갱신된다.

- **[완료 — 다음 대기 주문 승격 및 시작 시각 재설정]**
  - Given 생산 큐가 `["O001"(선두, 생산 시간 초과됨), "O002"(대기)]` 순서일 때
  - When 조회 시점 `now`에 `advanceIfDue()`가 호출되어 `"O001"`이 완료 처리되면
  - Then `"O002"`가 새로운 선두가 되고, `"O002".productionStartedAt = now`로 설정되며,
    `"O002"`의 `shortage`/`producedTotal`/`productionTimeMin`은 `"O001"` 처리로 갱신된 재고를 기준으로
    새로 계산되어 확정된다. (`"O002"`가 큐에 들어간 원래 시각이 아니라 `now` 기준으로 카운트가 시작됨)

- **[연쇄 완료 — cascade]**
  - Given 생산 큐가 `["O001", "O002"]`이고, `"O001"`의 생산 시간이 이미 오래 전에 초과되었으며,
    `"O002"`가 이제 막 시작되었다고 계산해도 `productionTimeMin`이 매우 짧아 즉시 완료 조건을 만족하는 경우
  - When 한 번의 조회로 `advanceIfDue()`가 호출되면
  - Then `"O001"`, `"O002"` 모두 같은 호출 내에서 순차적으로 `CONFIRMED`로 전환되고, 각 단계의 재고 반영이
    누적 반영된 최종 재고 상태로 조회 결과가 나온다. (한 번의 조회에서 여러 건의 완료가 연쇄적으로 처리될 수 있음)

- **[큐가 비어있음]**
  - Given 생산 큐에 대기/생산 중인 주문이 하나도 없을 때
  - When `advanceIfDue()`가 호출되면
  - Then 아무 상태 변화도 일으키지 않고 정상 반환한다. (예외를 던지지 않는다)

---

## 생산 현황 조회 (Display Production Status)

현재 생산 큐의 선두(실제 생산 중)인 주문에 대한 정보를 표시한다.

### Acceptance Criteria

- **[생산 중 정보 표시]**
  - Given 주문 `"O001"`이 생산 큐 선두로 생산 중이며 `productionTimeMin = 100`, 경과 시간 `40분`일 때
  - When 생산 현황을 조회하면
  - Then 결과에는 최소한 주문 ID(`"O001"`), 대상 시료, 주문 수량, `producedTotal`, 진행률(`40%`),
    남은 시간(`60분`)이 포함되어 표시된다.

- **[생산 중인 주문 없음]**
  - Given 생산 큐가 비어 있을 때
  - When 생산 현황을 조회하면
  - Then "현재 생산 중인 주문 없음"에 준하는 결과(빈 결과 또는 명시적 표시)를 반환하며 예외를 던지지 않는다.

- **[조회 시 지연 완료 판정 선반영]**
  - Given 선두 주문의 생산 시간이 이미 초과되어 있는 상태(아직 조회되지 않아 상태가 `PRODUCING`으로 남아있음)일 때
  - When 생산 현황을 조회하면
  - Then 조회 내부에서 `advanceIfDue()`가 먼저 호출되어 해당 주문은 `CONFIRMED`로 반영된 뒤, 조회 결과에는
    (완료되어 큐에서 빠진 뒤의) 새로운 선두 주문의 생산 현황이 표시된다. (즉 조회 결과는 항상 최신 상태 기준)

---

## 대기 주문 확인 (Display Waiting Queue)

생산 큐에서 선두를 제외한 대기 목록을 FIFO 순서 그대로 출력한다.

### Acceptance Criteria

- **[대기 목록 순서 — FIFO]**
  - Given 생산 큐에 `"O001"`(선두), `"O002"`, `"O003"` 순서로 들어있을 때
  - When 대기 주문 목록을 조회하면
  - Then `"O002"`, `"O003"` 순서(들어온 순서 그대로)로 반환되고, 선두인 `"O001"`은 대기 목록이 아닌
    생산 현황 조회에서 표시된다 (대기 목록에는 포함되지 않음).

- **[대기 주문 없음]**
  - Given 생산 큐에 주문이 하나(선두)만 있거나 아예 없을 때
  - When 대기 주문 목록을 조회하면
  - Then 빈 목록이 반환된다. (예외를 던지지 않는다)

- **[대기 목록 조회도 최신 상태 반영]**
  - Given 선두 주문의 생산이 이미 완료 조건을 만족한 상태일 때
  - When 대기 주문 목록을 조회하면
  - Then 조회 내부에서 `advanceIfDue()`가 먼저 적용되어, 완료된 주문은 큐에서 빠지고 승격된 새 선두를 제외한
    나머지가 대기 목록으로 반환된다.

---

## Edge Cases

- **[shortage = 0 은 발생하지 않아야 하는 불변식]** 주문 승인(order-approval) 로직은 재고가 주문 수량을 충족하는 경우
  생산 라인으로 라우팅하지 않고 즉시 확정(재고 차감만으로 충족) 처리하도록 설계되어 있다. 따라서 생산 라인에 진입하는
  주문은 항상 `shortage = quantity - stock > 0`이어야 한다. 이는 생산 라인 진입 시점에 **불변식(invariant)** 으로
  단언(assert)하며, `shortage <= 0`인 주문이 생산 큐에 들어오는 것은 상위(주문 승인) 로직의 버그로 간주한다.
  (본 문서의 테스트에서는 `shortage <= 0`으로 생산 진입을 시도하는 상황을 방어적으로 검증하되, 정상 플로우에서는
  발생하지 않음을 전제로 한다.)
- **[yieldRate > 0 보장]** `producedTotal` 계산의 분모(`yieldRate * 0.9`)가 0이 되어 0으로 나누는 문제가 발생하지
  않도록, `yieldRate`는 시료 등록 시 이미 `0 < yieldRate <= 1`로 검증되어 있다 (`sample-management.md` "유효하지
  않은 수율 거부" 참고). 생산 라인 로직 자체에서는 이 값이 항상 0보다 크다고 가정하고 별도 재검증은 생략하되,
  방어적 차원에서 `yieldRate <= 0`인 경우 예외를 던지는 것을 허용한다(도달 불가능해야 하는 경로).
- **[올림 처리의 부동소수점 경계]** `shortage / (yieldRate * 0.9)`가 수학적으로 정수(예: `100.0`)인 경우, 부동소수점
  연산 오차로 `100.00000000001`처럼 계산되어 `ceil` 결과가 `101`로 잘못 올림되지 않도록 구현 시 허용 오차(epsilon)
  비교 또는 정수 연산으로의 재구성을 고려한다. (`보조 예제` 참고)
- **[생산 큐에 동시에 여러 주문이 대기]** 여러 주문이 동시에 부족분이 발생해도, 생산 라인은 단일 라인이므로 한 번에
  하나씩만 처리하며 나머지는 FIFO로 순차 대기한다.

---

## Non-Goals

- **주문 승인/거부 로직 자체**: 어떤 조건에서 주문이 `PRODUCING`으로 라우팅되는지(재고 부족 판정 등)의 세부 규칙은
  주문 관리(Order Management) 문서에서 다루며, 본 문서는 이미 `PRODUCING`으로 진입한 이후의 생산 처리만 다룬다.
- **실시간/백그라운드 처리**: 실제 벽시계 스레드나 타이머를 이용한 비동기 완료 알림은 다루지 않는다. 본 시스템은
  조회 시점 지연 계산(`advanceIfDue()`) 방식만을 채택한다.
- **다중 생산 라인/병렬 생산**: 생산 라인을 여러 개 두어 동시에 여러 주문을 생산하는 구조는 본 과제 범위 밖이다.
- **영속성(Persistence)**: `productionStartedAt` 등 생산 관련 필드의 파일 저장/복원 방식은 별도의 persistence 문서에서
  다룬다.
