# 모니터링 (Monitoring)

## Overview

모니터링은 주문(Order)과 시료(Sample) 재고의 현재 상태를 담당자가 한눈에 파악할 수 있도록 제공하는 **읽기 전용(read-only)** 조회 기능이다.
크게 두 가지 하위 기능으로 구성된다.

1. **주문량 확인**: 현재 상태별(`RESERVED` / `CONFIRMED` / `PRODUCING` / `RELEASE`) 주문 건수를 집계한다.
2. **재고량 확인**: 각 시료별 현재 재고 수량과, 그 재고가 대기 중인 주문 수요에 비해 충분한지를 판정하여 상태(여유/부족/고갈)로 표기한다.

이 문서는 이후 GoogleTest 테스트 케이스로 변환되는 것을 전제로 하므로, 모든 동작은 Given-When-Then 형식으로 구체적으로 기술한다.

---

## 설계 결정 (Design Decisions)

### "주문대비" 재고 판정 기준 (모호한 원본 명세의 구체화)

원본 과제 명세는 "주문대비 재고 수량에 따라 여유/부족/고갈로 표기한다"고만 되어 있고, "주문대비"가 정확히 무엇을 기준으로
비교하는 것인지 정의하지 않는다. 본 문서는 다음 규칙을 **확정된 설계 결정**으로 채택한다.

- **`pendingDemand` (미출고 대기 수량) 정의**: 특정 시료에 대해 현재 `RESERVED` 또는 `PRODUCING` 상태로 남아 있는(아직 출고되지 않은)
  주문들의 수량 합계. `CONFIRMED`, `RELEASE`, `REJECTED` 상태의 주문은 `pendingDemand` 집계에 포함하지 않는다.
  - `CONFIRMED`는 승인은 되었으나 아직 생산 착수 전 단계이므로 본 시스템의 재고 압박 계산에서는 `RESERVED`/`PRODUCING`만을
    "실제로 재고 확보가 필요한 대기 수요"로 간주한다. (승인 대기/생산 대기 물량이 곧 재고를 소모할 실질적 수요)
  - `RELEASE`는 이미 출고되어 재고에서 차감이 완료된 것으로 간주하므로 대기 수요에서 제외한다.
  - `REJECTED`는 무효한 주문이므로 모든 집계(주문량 확인, 재고량 확인)에서 완전히 제외한다.
- **판정 규칙** (시료 `S`에 대해 `stock`과 `pendingDemand`를 계산한 뒤):
  1. `stock == 0` → **고갈(Depleted)**. `pendingDemand` 값과 무관하게 항상 고갈로 판정한다 (수량이 0이면 대기 수요가 없어도 실질적으로 출고 불가능하기 때문).
  2. `stock > 0` 이고 `pendingDemand == 0` → **여유(Sufficient)**. 대기 중인 주문이 없으므로 재고가 있다면 항상 여유로 판정한다.
  3. `stock > 0` 이고 `stock < pendingDemand` → **부족(Low)**. 재고가 있지만 대기 수요를 전부 충족시키기에는 모자란 상태.
  4. `stock > 0` 이고 `stock >= pendingDemand` (`pendingDemand > 0`) → **여유(Sufficient)**. 대기 수요를 모두 충족할 수 있는 상태.
- 이 규칙은 판정 우선순위가 있다: **① `stock==0` 여부를 가장 먼저 확인**하고, 그 다음에만 `pendingDemand`와의 대소 비교를 수행한다.

### 기타 결정 사항

- **`REJECTED` 주문 제외**: 원본 명세에 명시된 대로, `REJECTED` 상태의 주문은 유효한 주문이 아니므로 "주문량 확인"의 상태별 집계와
  "재고량 확인"의 `pendingDemand` 계산 양쪽 모두에서 제외한다.
- **읽기 전용(Read-only)**: 모니터링 기능은 조회만 수행하며 주문이나 시료 재고 상태를 변경하지 않는다. (자세한 내용은 Non-Goals 참조)
- **집계 대상 없음(0건)의 처리**: 특정 상태의 주문이 하나도 없거나, 특정 시료에 대한 주문이 하나도 없는 경우에도 예외를 던지지 않고
  개수 `0` 또는 `pendingDemand=0`으로 정상 처리한다.

---

## 주문량 확인 (Order Count by Status)

현재 시스템에 존재하는 주문을 상태별(`RESERVED`, `CONFIRMED`, `PRODUCING`, `RELEASE`)로 집계하여 각 상태의 건수를 보여준다.
`REJECTED` 상태의 주문은 유효하지 않은 주문으로 간주하여 집계에서 완전히 제외한다.

### Acceptance Criteria

- **[상태별 정상 집계] 각 상태의 주문 건수를 정확히 센다.**
  - Given `RESERVED` 주문 2건, `CONFIRMED` 주문 1건, `PRODUCING` 주문 3건, `RELEASE` 주문 1건이 등록되어 있을 때
  - When 주문량 확인(상태별 집계)을 조회하면
  - Then 결과는 `RESERVED=2`, `CONFIRMED=1`, `PRODUCING=3`, `RELEASE=1`이다.

- **[REJECTED 제외] REJECTED 상태 주문은 어떤 집계에도 포함되지 않는다.**
  - Given `RESERVED` 주문 1건과 `REJECTED` 주문 5건이 등록되어 있을 때
  - When 주문량 확인을 조회하면
  - Then 결과는 `RESERVED=1`이고, 집계 결과에 `REJECTED` 항목은 존재하지 않거나(또는 존재하더라도) 다른 상태의 합계에 영향을 주지 않는다.
  - 그리고 전체 유효 주문 수를 합산해도 `REJECTED` 5건은 포함되지 않는다 (즉 `RESERVED(1)`만 유효 주문으로 집계된다).

- **[특정 상태 0건] 특정 상태에 해당하는 주문이 없으면 해당 상태는 0으로 표기된다.**
  - Given `RESERVED` 주문만 3건 등록되어 있고 `CONFIRMED`, `PRODUCING`, `RELEASE` 상태 주문은 없을 때
  - When 주문량 확인을 조회하면
  - Then 결과는 `RESERVED=3`, `CONFIRMED=0`, `PRODUCING=0`, `RELEASE=0`이다. (예외를 던지지 않는다)

- **[전체 0건] 등록된 주문이 전혀 없으면 모든 상태가 0으로 표기된다.**
  - Given 시스템에 등록된 주문이 하나도 없을 때
  - When 주문량 확인을 조회하면
  - Then 결과는 `RESERVED=0`, `CONFIRMED=0`, `PRODUCING=0`, `RELEASE=0`이다.

- **[동일 시료 다중 상태 합산] 여러 시료에 걸친 주문도 상태 기준으로 합산된다.**
  - Given 시료 `"S001"`에 대한 `RESERVED` 주문 1건과 시료 `"S002"`에 대한 `RESERVED` 주문 1건이 등록되어 있을 때
  - When 주문량 확인을 조회하면
  - Then `RESERVED=2` (시료 종류와 무관하게 상태 기준으로 합산된다).

---

## 재고량 확인 (Stock Status by Sample)

등록된 각 시료별로 현재 재고 수량(`stock`)을 확인하고, 위 "설계 결정"에서 정의한 규칙에 따라 여유(Sufficient)/부족(Low)/고갈(Depleted)
상태를 함께 표기한다.

### Acceptance Criteria

- **[여유 - 대기 수요 없음] 대기 주문이 없고 재고가 0보다 크면 여유로 판정한다.**
  - Given 시료 `"S001"`의 `stock=480`이고, `"S001"`에 대해 `RESERVED`/`PRODUCING` 상태인 주문이 하나도 없을 때 (`pendingDemand=0`)
  - When 재고량 확인을 조회하면
  - Then `"S001"`의 상태는 **여유(Sufficient)**로 표기된다.

- **[부족 - 재고가 대기 수요보다 적음] 재고가 있지만 대기 수요를 채우기에 부족하면 부족으로 판정한다.**
  - Given 시료 `"S002"`의 `stock=30`이고, `"S002"`에 대해 `RESERVED` 주문(수량 100) 1건과 `PRODUCING` 주문(수량 70) 1건이 있어
    `pendingDemand=170`일 때
  - When 재고량 확인을 조회하면
  - Then `30 < 170`이므로 `"S002"`의 상태는 **부족(Low)**으로 표기된다.

- **[여유 - 대기 수요를 충족 가능] 재고가 대기 수요 이상이면 여유로 판정한다.**
  - Given 시료 `"S003"`의 `stock=200`이고, `"S003"`에 대해 `RESERVED` 주문(수량 50)과 `PRODUCING` 주문(수량 100)이 있어
    `pendingDemand=150`일 때
  - When 재고량 확인을 조회하면
  - Then `200 >= 150`이므로 `"S003"`의 상태는 **여유(Sufficient)**로 표기된다.

- **[경계값 - 재고와 대기 수요가 정확히 같음] stock == pendingDemand 이면 여유로 판정한다.**
  - Given 시료 `"S004"`의 `stock=150`이고 `pendingDemand=150`일 때
  - When 재고량 확인을 조회하면
  - Then `150 >= 150`이므로 `"S004"`의 상태는 **여유(Sufficient)**로 표기된다. (동률은 부족이 아니라 여유로 판정)

- **[고갈 - 대기 수요 유무와 무관] 재고가 정확히 0이면 대기 수요가 있든 없든 항상 고갈로 판정한다.**
  - Given 시료 `"S005"`의 `stock=0`이고, `pendingDemand=0` (대기 주문 없음)일 때
  - When 재고량 확인을 조회하면
  - Then `"S005"`의 상태는 **고갈(Depleted)**로 표기된다.
  - Given 시료 `"S006"`의 `stock=0`이고, `RESERVED` 주문(수량 50)이 있어 `pendingDemand=50`일 때
  - When 재고량 확인을 조회하면
  - Then `"S006"`의 상태 역시 **고갈(Depleted)**로 표기된다 (`stock==0` 규칙이 `pendingDemand` 비교보다 우선한다).

- **[CONFIRMED/RELEASE는 pendingDemand에서 제외] 승인되었거나 이미 출고된 주문은 부족 판정에 영향을 주지 않는다.**
  - Given 시료 `"S007"`의 `stock=10`이고, `CONFIRMED` 주문(수량 500)과 `RELEASE` 주문(수량 300)만 있으며
    `RESERVED`/`PRODUCING` 주문은 없을 때 (`pendingDemand=0`)
  - When 재고량 확인을 조회하면
  - Then `"S007"`의 상태는 **여유(Sufficient)**로 표기된다 (`CONFIRMED`, `RELEASE` 수량은 `pendingDemand`에 합산되지 않으므로).

- **[REJECTED는 pendingDemand에서 제외] 반려된 주문은 재고 판정에 어떠한 영향도 주지 않는다.**
  - Given 시료 `"S008"`의 `stock=5`이고, `REJECTED` 주문(수량 1000)만 있고 `RESERVED`/`PRODUCING` 주문은 없을 때 (`pendingDemand=0`)
  - When 재고량 확인을 조회하면
  - Then `"S008"`의 상태는 **여유(Sufficient)**로 표기된다.

---

## Edge Cases

- **[주문이 하나도 없는 시료] 등록만 되어 있고 주문 이력이 전혀 없는 시료도 정상적으로 표기된다.**
  - Given 시료 `"S009"`가 `stock=100`으로 등록되어 있고, `"S009"`에 대한 주문이 시스템에 하나도 존재하지 않을 때
  - When 재고량 확인을 조회하면
  - Then `"S009"`은 조회 결과에 포함되고, `pendingDemand=0`으로 계산되어 상태는 **여유(Sufficient)**로 표기된다. (예외를 던지지 않는다)

- **[여러 대기 주문의 합산] 같은 시료에 대한 RESERVED/PRODUCING 주문이 여러 건이면 수량이 정확히 합산된다.**
  - Given 시료 `"S010"`의 `stock=100`이고, `"S010"`에 대해 `RESERVED` 주문(수량 20), `RESERVED` 주문(수량 30),
    `PRODUCING` 주문(수량 60)이 각각 존재할 때
  - When 재고량 확인을 조회하면
  - Then `pendingDemand = 20 + 30 + 60 = 110`으로 계산되고, `100 < 110`이므로 `"S010"`의 상태는 **부족(Low)**으로 표기된다.

- **[등록된 시료가 하나도 없음] 시스템에 등록된 시료가 없으면 빈 목록을 반환한다.**
  - Given 시스템에 등록된 시료가 하나도 없을 때
  - When 재고량 확인을 조회하면
  - Then 빈 목록(size 0)이 반환된다. (예외를 던지지 않는다)

- **[다수 시료 동시 조회] 여러 시료를 한 번에 조회하면 각 시료가 독립적으로 판정된다.**
  - Given 시료 `"S011"`(`stock=0`), `"S012"`(`stock=10`, `pendingDemand=50`), `"S013"`(`stock=1000`, `pendingDemand=0`)이
    함께 등록되어 있을 때
  - When 재고량 확인을 조회하면
  - Then `"S011"`은 **고갈**, `"S012"`는 **부족**, `"S013"`은 **여유**로, 서로 영향을 주지 않고 독립적으로 표기된다.

---

## Non-Goals

- **주문/시료 상태 변경(Mutation)**: 모니터링 기능은 순수 조회(read-only) 기능이며, 주문의 상태 전이(`RESERVED → CONFIRMED` 등)나
  시료 재고의 증감을 직접 수행하지 않는다. 주문 승인/반려에 관한 규칙은 `order-approval.md`에서, 생산 라인 진행에 따른 재고 차감 및
  주문 상태 전이(`PRODUCING → RELEASE` 등)에 관한 규칙은 `production-line.md`에서 다룬다.
- **실시간 갱신(Push/Polling)**: 콘솔 애플리케이션 특성상 화면이 자동으로 갱신되는 실시간 대시보드는 범위 밖이며, 사용자가 조회를
  요청한 시점의 스냅샷(snapshot) 정보만을 제공한다.
- **알림/경고(Alerting)**: 재고가 "부족" 또는 "고갈" 상태에 진입했을 때 별도의 알림을 발송하는 기능은 본 문서의 범위가 아니다.
- **주문/재고 데이터의 영속성**: 데이터를 파일 등에 저장/복원하는 방식은 별도의 persistence 문서에서 다룬다.
- **동시성(Concurrency)**: 멀티스레드 환경에서의 동시 조회 처리에 대한 고려는 본 콘솔 애플리케이션의 범위 밖으로 간주한다.
