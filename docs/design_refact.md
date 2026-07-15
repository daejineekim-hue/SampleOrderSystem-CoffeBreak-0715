# Phase 8 — 아키텍처 리팩토링 계획

Phase 0~7에서 기능 요구사항은 모두 구현되었으나, TDD로 기능을 하나씩 쌓는 과정에서
생긴 구조적 부채가 있다. 이 문서는 그 중 우선순위가 높은 4개 항목을 정리하고, 각
항목의 리팩토링 전/후 설계를 기록한다. **동작은 바꾸지 않고 구조만 바꾼다** —
기존 GoogleTest/`system-test.ps1`이 계속 GREEN이어야 한다.

## 항목 1+2: `OrderRepository`의 책임 분리 + `ProductionLine` 양방향 결합 제거

두 항목은 서로 얽혀 있어 하나의 설계로 함께 해결한다.

### 문제

- `OrderRepository`가 JSON CRUD(`findAll`/`findById`/영속화)와 주문 상태 전이
  비즈니스 로직(`approve`/`reject`/`release`/`completeProduction`)을 동시에 갖고
  있어 "repository"라는 이름과 실제 책임이 어긋난다.
- `OrderRepository::approve()`는 `production::ProductionLine&`을 파라미터로 받고,
  `ProductionLine`은 완료 시 `orderRepository_.completeProduction(...)`을 콜백으로
  호출한다. 전방 선언으로 컴파일은 되지만 두 클래스가 서로를 알아야 하는 구조다.

### 해결 방향

`OrderRepository`에 **순수 저장소 연산 하나**를 추가해 상태 전이 관련 메서드를
전부 대체한다:

```cpp
// 검증 없이 상태만 바꾸고 저장한다 (SampleRepository::updateStock과 동일한 성격).
const model::Order& updateStatus(const std::string& orderId, model::OrderStatus newStatus);
```

`approve`/`reject`/`release`/`completeProduction`은 `OrderRepository`에서 완전히
제거하고, 아래 두 곳으로 나눠 옮긴다.

**`src/service/OrderLifecycleService.h/cpp` (신규)** — 사용자가 트리거하는 전이
(승인/거절/출고)만 담당:

```cpp
class OrderLifecycleService {
public:
    OrderLifecycleService(repository::OrderRepository&, repository::SampleRepository&);

    const model::Order& approve(const std::string& orderId, production::ProductionLine&);
    const model::Order& reject(const std::string& orderId);
    const model::Order& release(const std::string& orderId);
};
```

`approve()`만 `productionLine.enqueue(orderId)`를 호출한다 — **단방향** 의존
(Service → ProductionLine)이며 콜백은 없다.

**`ProductionLine`가 완료 처리를 직접 수행** (production-line.md Formula #3은 원래
이 문서 소유였다 — `OrderRepository::completeProduction`에 있던 것이 오히려
잘못된 위치였음): `advanceIfDue()`가 큐 선두 완료를 감지하면, 이미 알고 있는
`producedTotal`과 `orderRepository_.findById(...)`로 읽은 `quantity`/`sampleId`를
이용해 `sampleRepository_.updateStock(...)` + `orderRepository_.updateStatus(...)`를
**직접** 호출한다. `OrderRepository`에 콜백할 필요가 없어진다.

### 결과 의존 그래프

```
OrderLifecycleService --> OrderRepository (updateStatus, findById)
OrderLifecycleService --> SampleRepository (findAll, updateStock)
OrderLifecycleService --> ProductionLine   (enqueue만, 단방향)

ProductionLine --> OrderRepository  (findById, updateStatus)
ProductionLine --> SampleRepository (findAll, updateStock)

OrderRepository --> SampleRepository (registerOrder의 sampleId 존재 검증, 기존과 동일)
```

`OrderRepository`와 `ProductionLine`은 더 이상 서로를 모른다 (순환 없음).
`OrderRepository.h`에서 `production::ProductionLine` 전방 선언도 제거된다.

### 영향받는 파일

- `src/repository/OrderRepository.h/.cpp`: `approve`/`reject`/`release`/
  `completeProduction` 제거, `updateStatus` 추가
- `src/service/OrderLifecycleService.h/.cpp` (신규)
- `src/production/ProductionLine.cpp`: 완료 처리 로직을 `updateStatus`+`updateStock`
  직접 호출로 변경
- `src/controller/OrderApprovalController.*`, `src/controller/ShipmentController.*`:
  `OrderRepository` 대신 `OrderLifecycleService`를 주입받아 사용
- `src/main.cpp`: `OrderLifecycleService` 구성 및 배선
- 테스트: `src/test/OrderApprovalTest.cpp`, `src/test/ShipmentTest.cpp`,
  `src/test/ProductionLineTest.cpp`(`approveIntoProduction` 헬퍼) — 호출부만
  `OrderLifecycleService`로 교체, 기대값(Acceptance Criteria)은 변경 없음

## 항목 3: 콘솔 서브메뉴 루프 중복 제거

### 문제

`SampleManagementController`/`OrderIntakeController`/`OrderApprovalController`/
`ShipmentController` 4곳에 아래 골격이 거의 그대로 복제되어 있다:

```cpp
while (true) {
    std::cout << "...메뉴...\n선택 > ";
    int choice = -1;
    if (!(std::cin >> choice)) { std::cin.clear(); choice = -1; }
    skipToNextLine();
    if (choice == 0) return;
    switch (choice) { ... default: std::cout << "[오류] 유효하지 않은 선택입니다.\n"; }
}
```

### 해결 방향

공용 베이스 클래스 `src/controller/ConsoleSubmenuController.h/.cpp` (신규)를
도입한다. `run()`을 템플릿 메서드로 구현하고, 하위 클래스는 메뉴 출력과 선택
처리만 구현한다.

```cpp
class ConsoleSubmenuController : public SubController {
public:
    void run() final;  // 루프 골격 (메뉴 표시 -> 입력 -> 0이면 리턴 -> handle 위임)

protected:
    virtual void showPrompt() = 0;      // "선택 > "로 끝나는 메뉴 출력
    virtual bool handle(int choice) = 0; // 처리했으면 true, 모르는 선택이면 false
};
```

4개 컨트롤러가 이를 상속해 `showPrompt()`/`handle()`만 구현하도록 바꾼다.
`MonitoringController`/`ProductionLineController`/`DummyDataController`는 메뉴
루프가 없는 1회성 화면이라 대상에서 제외한다.

### 영향받는 파일

- `src/controller/ConsoleSubmenuController.h/.cpp` (신규)
- `src/controller/SampleManagementController.*`, `OrderIntakeController.*`,
  `OrderApprovalController.*`, `ShipmentController.*`: 베이스 클래스 상속으로 변경

콘솔 입출력은 GoogleTest 대상이 아니므로(main-menu.md Testability note), 이
리팩토링의 회귀 확인은 `system-test.ps1` 재실행으로 한다.

## 항목 4: `MenuSummaryProvider`의 `Monitoring` 로직 중복 제거

### 문제

`MenuSummaryProvider::getSummary()`가 RESERVED/PRODUCING 주문 수를 직접 루프 돌며
세는데, 이미 동일한 정의(`docs/adr/0002`)를 `Monitoring::countByStatus`가 갖고
있다. 지금은 값이 같지만 나중에 monitoring.md 정의가 바뀌면 두 곳을 따로 고쳐야
하는 드리프트 위험이 있다.

### 해결 방향

```cpp
auto counts = model::Monitoring::countByStatus(orders);
summary.productionWaitingCount = counts.reserved + counts.producing;
```

### 영향받는 파일

- `src/controller/MenuSummaryProvider.cpp`

## 검증 방법

1. 각 항목을 순서대로(1+2 -> 3 -> 4) 적용하고, 매 항목 후 `./test.ps1`
   (GoogleTest)로 회귀를 확인한다.
2. 항목 3(콘솔 루프 통합) 이후에는 `./system-test.ps1`도 재실행해 실제 콘솔
   플로우가 그대로인지 확인한다.
3. 전체 완료 후 `./test.ps1` + `./system-test.ps1` 최종 재확인.
