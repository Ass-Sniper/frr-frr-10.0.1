# FRR OSPF Delegate / FSM / LSA / Zebra 分析归档

> 归档目的：汇总 OSPF 使用 delegate 的实现方式、关键数据结构关系、消息处理链路、zclient 交互流程，以及 Event/FSM/LSA/Zebra 端到端链路。
> 该文档用于后续扩展其他协议或子系统的分析文档。

---

## 1. OSPF 如何使用 delegate（route_table_delegate_t）

### 1.1 delegate 机制概览（通用）
- `route_table_delegate_t` 是路由表“委托接口”，提供 create/destroy 的函数指针。
- `route_table_init_with_delegate()` 把 delegate 挂到 table 上；后续 `route_node_new()` / `route_node_free()` 通过 delegate 的回调创建/销毁节点。

**关联源码：**
- `lib/table.h`：`route_table_delegate_t` 定义、`route_table` 持有 `delegate`。
- `lib/table.c`：`route_table_init_with_delegate()`、`route_node_new()`、`route_node_free()`。

### 1.2 OSPF 在哪里使用 delegate
- OSPF 仅在 **Area range / NSSA range** 表中使用自定义 delegate。
- `ospf_range_table_delegate` 使用默认 `route_node_create()` 创建，但销毁时调用 `ospf_range_table_node_destroy()`，释放 `node->info` 与 node 本体。

**关联源码：**
- `ospfd/ospfd.c`：`ospf_range_table_delegate`、`ospf_range_table_node_destroy()`、`ospf_area_new()` 中的 `route_table_init_with_delegate()`。

### 1.3 LSDB 未使用自定义 delegate
- `ospf_lsdb_init()` 通过 `route_table_init()` 初始化 LSA DB，使用默认 delegate。

**关联源码：**
- `ospfd/ospf_lsdb.c`。

---

## 2. OSPF delegate 关键时序图

### 2.1 PlantUML（delegate 创建/销毁）
```plantuml
@startuml
title OSPF Range Table Delegate Sequence

participant "ospf_area_new()\n(ospfd/ospfd.c)" as OSPF
participant "route_table_init_with_delegate()\n(lib/table.c)" as RT
participant "route_table (delegate)" as Table
participant "route_node_new()\n(lib/table.c)" as RN
participant "delegate->create_node\n(route_node_create)" as Create
participant "delegate->destroy_node\n(ospf_range_table_node_destroy)" as Destroy

OSPF -> RT : init ranges/nssa_ranges\nwith ospf_range_table_delegate
RT -> Table : table->delegate = ospf_range_table_delegate

== Node create path ==
RN -> Table : route_node_new(table)
RN -> Create : delegate->create_node()

== Node destroy path ==
RN -> Table : route_node_free(table,node)
RN -> Destroy : delegate->destroy_node()

@enduml
```

### 2.2 Mermaid（delegate 创建/销毁）
```mermaid
sequenceDiagram
    participant OSPF as ospf_area_new()<br>(ospfd/ospfd.c)
    participant RT as route_table_init_with_delegate()<br>(lib/table.c)
    participant Table as route_table (delegate)
    participant RN as route_node_new()/route_node_free()<br>(lib/table.c)
    participant CreateCallback as create_node callback<br>(route_node_create)
    participant DestroyCallback as destroy_node callback<br>(ospf_range_table_node_destroy)

    OSPF->>RT: init ranges / nssa_ranges
    RT->>Table: table.delegate = ospf_range_table_delegate

    Note over RN,Table: Node create path
    RN->>Table: route_node_new(table)
    Table->>CreateCallback: delegate.create_node()

    Note over RN,Table: Node destroy path
    RN->>Table: route_node_free(table, node)
    Table->>DestroyCallback: delegate.destroy_node()
```

---

## 3. OSPF delegate 全景图（event / FSM / LSA / zebra）

```mermaid
flowchart TB
    EventLoop["Event loop (frrevent)"] --> ISM_EVT["ISM/NSM Event Schedule/Execute<br>(ospf_ism.h / ospf_nsm.h)"]
    ISM_EVT --> ISM_FSM["ISM FSM<br>(ospf_ism.c)"]
    ISM_EVT --> NSM_FSM["NSM FSM<br>(ospf_nsm.c)"]

    ISM_FSM --> LSA_ORIG["LSA Origination/Flush<br>(ospf_ism.c)"]
    NSM_FSM --> LSA_ORIG

    LSA_ORIG --> LSDB["LSDB<br>(ospf_lsdb.c)"]
    LSDB --> SPF["SPF / Route computation<br>(ospf_lsa.c)<br>ospf_spf_calculate_schedule"]
    SPF --> Zebra["Zebra API (zclient)<br>(ospf_zebra.c)"]

    Delegate["route_table_delegate_t<br>(lib/table.h)"] --> RangeTable["OSPF range/nssa range<br>(ospf_range_table_delegate)"]
    RangeTable --> OSPF_Area["ospf_area_new()<br>(ospfd/ospfd.c)"]

    Zebra --> OSPF_Area
```
> ISM: Interface State Machine
>
> NSM: Neighbor State Machine

---

## 4. OSPF 关键数据结构与类图

### 4.1 关键结构（摘要）
- **ospf**：全局 OSPF 实例，持有 `areas`、`oiflist`、AS 级 LSDB 等。
- **ospf_area**：area 上下文，持有 area 级 LSDB、ranges/nssa_ranges。
- **ospf_interface**：接口上下文，持有邻居表、self network LSA。
- **ospf_neighbor**：邻居状态，持有重传/摘要/请求三类 LSDB。
- **ospf_lsa**：LSA 实例，关联 area / lsdb / interface。
- **ospf_lsdb**：每类 LSA 一张 route_table。

### 4.2 PlantUML 类图
```plantuml
@startuml
skinparam classAttributeIconSize 0

class ospf {
  +struct list *areas
  +struct ospf_area *backbone
  +struct list *oiflist
  +struct ospf_lsdb *lsdb
  +struct route_table *networks
}

class ospf_area {
  +struct ospf *ospf
  +struct list *oiflist
  +struct route_table *ranges
  +struct route_table *nssa_ranges
  +struct ospf_lsdb *lsdb
  +struct ospf_lsa *router_lsa_self
}

class ospf_interface {
  +struct ospf *ospf
  +struct ospf_area *area
  +struct route_table *nbrs
  +struct ospf_neighbor *nbr_self
  +struct ospf_lsa *network_lsa_self
}

class ospf_neighbor {
  +struct ospf_interface *oi
  +struct ospf_lsdb ls_rxmt
  +struct ospf_lsdb db_sum
  +struct ospf_lsdb ls_req
  +struct ospf_lsa *ls_req_last
}

class ospf_lsa {
  +struct ospf_area *area
  +struct ospf_lsdb *lsdb
  +struct ospf_interface *oi
}

class ospf_lsdb {
  +type[OSPF_MAX_LSA].db : route_table*
  +unsigned long total
}

ospf "1" o-- "*" ospf_area : areas
ospf "1" o-- "*" ospf_interface : oiflist
ospf "1" --> "1" ospf_lsdb : as-external lsdb
ospf_area "*" --> "1" ospf : parent

ospf_area "1" o-- "*" ospf_interface : oiflist
ospf_area "1" --> "1" ospf_lsdb : area lsdb

ospf_interface "*" --> "1" ospf : parent
ospf_interface "*" --> "1" ospf_area : area
ospf_interface "1" o-- "*" ospf_neighbor : nbrs
ospf_interface "1" --> "1" ospf_neighbor : nbr_self

ospf_neighbor "*" --> "1" ospf_interface : oi
ospf_neighbor "*" --> "*" ospf_lsdb : ls_rxmt/db_sum/ls_req

ospf_lsa "*" --> "0..1" ospf_area : area
ospf_lsa "*" --> "0..1" ospf_lsdb : lsdb
ospf_lsa "*" --> "0..1" ospf_interface : oi (Type-9 Opaque)

ospf_lsdb "1" o-- "*" route_table : per-LSA-type db
@enduml
```

---

## 5. OSPFd 接收并处理每种消息的完整时序图

涉及消息：Hello / DB-Desc / LS-Req / LS-Upd / LS-Ack。入口点在 `ospf_read()` / `ospf_read_helper()`。

```plantuml
@startuml
title OSPFd packet recv/dispatch (all message types)

participant "frrevent::event_add_read\n(ospf_packet.c::ospf_read)" as EventLoop
participant "ospf_packet.c::ospf_read" as Read
participant "ospf_packet.c::ospf_read_helper" as Helper
participant "ospf_packet.c::ospf_recv_packet" as Recv
participant "ospf_packet.c::ospf_packet_examin" as Examine
participant "ospf_packet.c::ospf_if_lookup_recv_if" as LookupIF
participant "ospf_packet.c::ospf_verify_header" as Verify

participant "ospf_packet.c::ospf_hello" as Hello
participant "ospf_packet.c::ospf_db_desc" as DBD
participant "ospf_packet.c::ospf_ls_req" as LSReq
participant "ospf_packet.c::ospf_ls_upd" as LSUpd
participant "ospf_packet.c::ospf_ls_ack" as LSAck

EventLoop -> Read : event_add_read(...) -> ospf_read()
Read -> Helper : ospf_read_helper(ospf)
Helper -> Recv : ospf_recv_packet(ospf, fd, &ifp, ibuf)
Helper -> Examine : ospf_packet_examin(ospfh, len)
Helper -> LookupIF : ospf_if_lookup_recv_if(ospf, ip_src, ifp)
Helper -> Verify : ospf_verify_header(ibuf, oi, iph, ospfh)

alt OSPF_MSG_HELLO
  Helper -> Hello : ospf_hello(iph, ospfh, ibuf, oi, length)
else OSPF_MSG_DB_DESC
  Helper -> DBD : ospf_db_desc(iph, ospfh, ibuf, oi, length)
else OSPF_MSG_LS_REQ
  Helper -> LSReq : ospf_ls_req(iph, ospfh, ibuf, oi, length)
else OSPF_MSG_LS_UPD
  Helper -> LSUpd : ospf_ls_upd(ospf, iph, ospfh, ibuf, oi, length)
else OSPF_MSG_LS_ACK
  Helper -> LSAck : ospf_ls_ack(iph, ospfh, ibuf, oi, length)
end

@enduml
```

---

## 6. OSPF 通过 zclient 处理 router-id 与接口地址变更的全流程时序图

```plantuml
@startuml
title OSPF <-> zebra (zclient callbacks)

participant "zebra (ZAPI)" as Zebra
participant "ospfd/ospf_zebra.c::ospf_router_id_update_zebra" as RidUpdate
participant "ospfd/ospf_zebra.c::ospf_interface_address_add" as IfAddrAdd
participant "ospfd/ospf_zebra.c::ospf_interface_address_delete" as IfAddrDel
participant "ospfd/ospfd.c::ospf_router_id_update" as OspfRidUpdate
participant "ospfd/ospf_interface.c::ospf_if_update" as IfUpdate
participant "ospfd/ospf_interface.c::ospf_if_interface" as IfInterface
participant "ospfd/ospf_interface.c::ospf_if_free" as IfFree

alt Router-ID update
  Zebra -> RidUpdate : ZAPI router-id update
  RidUpdate -> OspfRidUpdate : ospf_router_id_update(ospf)
else Interface address add
  Zebra -> IfAddrAdd : ZAPI interface address add
  IfAddrAdd -> IfUpdate : ospf_if_update(ospf, ifp)
  IfAddrAdd -> IfInterface : ospf_if_interface(ifp)
else Interface address delete
  Zebra -> IfAddrDel : ZAPI interface address delete
  IfAddrDel -> IfFree : ospf_if_free(oi)
  IfAddrDel -> IfInterface : ospf_if_interface(ifp)
end

@enduml
```

---

## 7. Event / FSM / LSA / Zebra 链路时序图

```plantuml
@startuml
title Event -> FSM -> LSA -> LSDB -> SPF -> Zebra

participant "frrevent::event_add_event\n(ospf_ism.h/ospf_nsm.h)" as EventLoop
participant "ospfd/ospf_ism.c::ospf_ism_event" as ISMEvent
participant "ospfd/ospf_nsm.c::ospf_nsm_event" as NSMEvent
participant "ospfd/ospf_ism.c::ospf_router_lsa_update_area" as RouterLSAUpd
participant "ospfd/ospf_ism.c::ospf_network_lsa_update" as NetworkLSAUpd
participant "ospfd/ospf_lsdb.c::ospf_lsdb_add" as LSDBAdd
participant "ospfd/ospf_lsa.c::ospf_*_lsa_install" as LSAInstall
participant "ospfd/ospf_lsa.c::ospf_spf_calculate_schedule" as SPFSched
participant "ospfd/ospf_zebra.c::zclient callbacks" as ZebraCB

EventLoop -> ISMEvent : OSPF_ISM_EVENT_* triggers
EventLoop -> NSMEvent : OSPF_NSM_EVENT_* triggers
ISMEvent -> RouterLSAUpd : schedule/router LSA update
ISMEvent -> NetworkLSAUpd : schedule/network LSA update
RouterLSAUpd -> LSDBAdd : LSA add/update in LSDB
NetworkLSAUpd -> LSDBAdd : LSA add/update in LSDB
LSDBAdd -> LSAInstall : LSA install path
LSAInstall -> SPFSched : ospf_spf_calculate_schedule(...)

SPFSched -> ZebraCB : routes/if state influence\n(via zclient callbacks)

@enduml
```

---

## 8. Zebra 名称与作用（基于仓库文档）

### 8.1 Zebra 的作用
- **zebra 是 IP 路由管理进程**，负责内核路由表更新、接口信息查询与路由重分发。
- OSPF（`ospfd`）必须通过 zebra 获取接口信息与路由器 ID；因此 zebra 必须先于 ospfd 启动。

**关联文档：**
- `doc/user/zebra.rst`
- `doc/user/ospfd.rst`

### 8.2 名称由来说明
- FRR 仓库文档中**未包含“zebra 名称由来”说明**；该问题需要外部资料才能回答。

---

## 9. 参考文件索引（便于进一步扩展）
- `lib/table.h`, `lib/table.c`
- `ospfd/ospfd.c`, `ospfd/ospfd.h`
- `ospfd/ospf_packet.c`
- `ospfd/ospf_ism.c`, `ospfd/ospf_nsm.c`
- `ospfd/ospf_lsa.c`, `ospfd/ospf_lsdb.c`
- `ospfd/ospf_zebra.c`
- `doc/user/zebra.rst`, `doc/user/ospfd.rst`

---

## 10. 后续可扩展方向
- 按协议分区：`docs/analysis/ospf/`、`docs/analysis/bgp/`、`docs/analysis/isis/`
- 按主题分区：`docs/analysis/fsm/`、`docs/analysis/lsdb/`、`docs/analysis/zebra/`
