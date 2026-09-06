# Aced 架构基线

日期：2026-09-06。状态：设计基线，尚非全部实现。

## 1. 结论与适用范围

采用一个 C++ 核心库、一个薄 CLI、一个 Rust 界面。核心围绕 Java 原生序列化文档组织：解析与写出共享协议模型，查询与分析只读消费模型，CLI 处理外部输入和输出协议。按职责、状态所有权和变化原因划分边界，不按函数数量拆模块。

这里的“通用”首先指不依赖目标 JAR 的原生序列化结构检查、查询、分析和受支持范围内的编辑。它不表示能够无条件解释任意自定义序列化内容、执行 Java 方法或自动确认漏洞可利用性。Hessian、Kryo、JVM 验证留待有明确需求后独立接入。

不能以自我审查证明“95% 的成功概率”。本设计通过三轮架构检查，并规定实现后的验收门槛；结论是适合当前项目的高置信度基线，资源预算、异常协议支持和编辑保真度仍需样本及测试验证。

## 2. 产品与构建边界

```text
Rust UI
  └─ 启动 aced CLI，通过版本化 JSON 交互
       └─ 调用 aced-core（aced::core）
            ├─ ByteReader：安全读取字节
            ├─ Serialization：文档、解析、后续写出与编辑
            └─ Analysis：查询、语义视图、规则与差异比较
```

只有 aced-core 静态库、aced 可执行程序两个生产构建目标；不为每个源码目录建立一个库。测试使用独立可执行程序和现有 aced-test-support INTERFACE 目标。Rust 界面位于未来的独立工程目录，保持当前 D:/Projects/Aced/aced-core 工程根目录不变。

依赖箭头表示左方使用右方：

```text
CLI → Serialization 公开入口、Analysis 公开入口
Analysis → Serialization::StreamDocument
StreamParser → StreamDocument、ByteReader
StreamWriter → StreamDocument、协议内部定义
StreamEditor → StreamDocument、共享的必要结构校验
StreamDocument → 标准库
ByteReader → 标准库
```

StreamEditor 与 StreamWriter 由调用方按“校验编辑 → 应用编辑 → 写出”编排，互不反向调用。协议内部辅助类型仅在确有多个使用方时抽取。底层不依赖 CLI、JSON、UI、分析规则或测试代码。

## 3. ByteReader 边界

保留现有 Aced::ByteReader：输入是非拥有的连续字节视图，提供读取、跳过、位置和剩余长度。只维护自身读取位置与边界不变量。

- 不加载文件，不识别 Java 魔数、类型标记或字符串编码，不打印日志。
- 越界时不推进位置；继续使用现有 std::out_of_range 契约。
- 不创建 IReader、ReaderFactory、流适配器或可插拔字节序策略。
- 解析器在明确的读取位置处理截断，不能捕获整个系统任意 out_of_range 并一律声称输入截断，以免掩盖程序错误。
- 非拥有视图的有效期由调用方负责；不持有可能因外部容器扩容失效的长期指针。

## 4. Serialization：一个领域，几个必要入口

### 4.1 解析入口与内部状态

公开 parse_stream(input, options) 一类完整操作；具体签名随最小文档模型落地。调用方不需要分步操作 read_header、read_token 或 handle 表。每次调用使用独立的内部 StreamParser 状态，不暴露可由外部重置或修改的解析阶段。

内部聚合：ByteReader、资源计数、handle 映射、当前类/对象上下文、流头校验、类型分派、块数据处理、错误位置。流头校验是私有方法或 cpp 内函数，不保留公开 stream_header.h、HeaderStatus。协议常量先留在实现文件，解析器与写出器共同使用后再提取到 src/serialization 的私有头文件。

同一个 StreamParser 可以在增长后拆成多个 cpp，继续共享一个内部状态定义；不按每个 TC_* 标记创建解析器子类，也不先建立通用插件注册器。

### 4.2 StreamDocument：唯一的结构事实来源

StreamDocument 存储 Java 原生序列化模型，不设计可表达所有序列化格式的万能对象树。

它持有：输入字节、按顺序排列的顶层记录、对象/字符串/数组/类描述等节点、节点间引用、原始字节范围及尚未解释的区域。字段值与引用构成对象图；不再复制一份独立可变图、JSON 树和语法树。

- 默认取得输入缓冲区所有权；CLI 可移动 vector 交给解析器。若接受 span，必须明确复制，不能让返回文档引用临时输入。
- 内部使用文档局部 NodeId；不把裸指针或 vector 元素地址当持久身份。节点 ID 不是协议 handle，reset 后重用 handle 不能覆盖旧节点身份。
- 顺序记录只保存顺序、范围和节点引用等必要信息，不重复整个节点内容。
- 记录本身的字节范围与它引用的对象范围分开；对象涉及多个原始区域时不能伪造连续范围。
- 外部通过只读访问查询；构建器与编辑代码维护引用有效性等不变量。采用值类型/variant 等简单结构，不使用每种节点一个虚基类。
- 文档不负责读取文件、解析 JSON、执行规则、生成报告或展示 UI。
- 原始字节属于解析时快照。编辑后的值必须与原始来源信息明确区分，不能把旧偏移伪装成新输出偏移。
- 文档局部 ID 不能当成两个独立样本之间的稳定身份；diff 需要结构匹配，不能直接比较相同数值的 ID。

### 4.3 结果与错误

在 serialization 领域内使用明确的解析结果和诊断，不建立全项目 Result<T>/ErrorManager 框架。

解析结果区分：完整、部分、失败。诊断至少有错误码、输入偏移、解释；上下文仅在确有帮助时加入。被截断、未支持布局或超出预算的数据不能标记成完整成功。调用方可以显式接受部分结果，但部分文档不得包含指向不存在节点的悬空引用。

内部可用专用解析异常简化失败退出，公开入口只转换本领域可预期错误。内存分配失败、程序错误不能统统降级成“样本格式错误”。CLI 最外层负责转成进程失败信息。

流头错误也属于解析错误，不单设 HeaderStatus。协议错误码与 CLI 数字退出码分别定义，避免接口绑死。

### 4.4 资源与未知内容

解析选项限制输入大小、深度、节点数、字符串/数组规模及总构建预算。分配前检查长度乘法、类型转换与预算；深度限制必须足够保守，必要时用显式栈。递归或循环都检查取消请求，优先使用 C++20 stop_token，不发明取消框架。

嵌套数据分析有总层数、总字节及总工作预算，不能每个嵌套样本重新获得完整额度。初期由嵌套分析操作持有预算；确有多个操作共享后再抽内部预算类型。

能够确定边界但不能解释含义的内容保留为不透明区；无法确定边界时在可靠位置停止并记录原因。不得扫描“下一个像类型标记的字节”来假装恢复解析。

Java Modified UTF-8、reset/exception 的引用表语义、自定义 writeObject/readObject、旧版 Externalizable 都要独立列入协议支持矩阵。特别是自定义写入不能仅凭类字段描述假定默认字段必然按固定位置存在；需要类约定时应声明限制。

### 4.5 后续写出与编辑

StreamWriter 消费受支持且有效的 StreamDocument 并生成字节，自行维护写出 handle；不复用解析器的可变状态，不负责文件保存。

StreamEditor 在 serialization 内部作为后续能力出现，校验并应用字符串、字段、数组、引用等修改；先验证整项编辑，失败不留下半修改状态。初期不引入命令总线、撤销框架或持久化快照系统。

原样导出直接使用原始输入；语义重建是另一种操作。已修改文档的原始快照不能作为“保存修改”的结果。对不透明内容、未知引用依赖或无法可靠重写的布局拒绝相关重建。未修改区域也不能无条件直接拼接，因为引用编号及上下文可能变化。

测试分别验证原样导出的字节一致性、受支持重建的语义一致性，不承诺任意样本编辑后字节完全一致。

## 5. Analysis：只读解释与证据

Analysis 读取 StreamDocument，提供检索、引用路径查询、已知集合等附加语义视图、规则分析和后续结构 diff。它不能反向修改 StreamParser 的控制流或重写 StreamDocument。

查询函数、规则执行和语义解释先按文件组织在同一领域，不先拆成 QueryService、SemanticManager、RuleRepository 等服务层。只有出现独立调用方与稳定契约时才升级为独立模块。

规则结果包括规则 ID/版本、风险等级、置信度、命中 NodeId、字节位置、引用路径、依据与未验证条件。类名匹配是线索，引用路径是结构证据，不能直接当成方法调用链或确认可利用性。

已知类的内容展示由 Analysis 解释；若类特定知识是“确定原始字节如何消费”的必要条件，则属于解析器私有协议处理逻辑。前者不应偷偷决定后者的边界。

初期规则使用编译期函数和普通数据。出现实际外部规则编写需求后再定义受限的数据格式，复杂行为仍由 C++ 实现。不要提前建立脚本运行时或动态插件 ABI。

查询需要索引时先按调用构建；性能数据证明重复成本明显后再缓存，并将缓存绑定到文档版本。不可在可编辑文档上无条件复用旧索引。

## 6. CLI 与 Rust

CLI 负责参数校验、文件/标准输入读取、用户选择的 Base64/Hex 转换、调用核心、JSON/文本输出、输出文件写入和退出码。数据转换最初是 CLI 内函数；第二个实际调用方出现后再提取小型 codec，不先设通用编码框架。

分析规则和对象解释仍在核心，不能为了让 CLI 看起来方便而移入命令处理代码。文件创建/覆盖失败必须与解析失败分开。

第一版命令以单次任务运行，规划 inspect、query、analyze、diff、rewrite，按能力逐个实现。Rust 通过参数数组启动 CLI，不拼 shell 字符串；同时消费 stdout/stderr，避免管道阻塞。

- stdout：结构化结果；stderr：诊断与进度。
- JSON 有 schema_version，文档节点用 ID 引用，避免展开循环。
- 64 位整数、SUID 和大偏移使用明确的无损表示；定义哪些字段是十进制字符串，不能依赖前端数字精度。
- 长字符串/数组默认摘要，显式查询展开；报告包含完整/部分状态及输入来源。
- 原始文件偏移与解码后偏移不能混淆。Base64/压缩转换后默认报告解码流偏移和变换说明，不虚构一一映射。
- 发现风险是分析成功的结果，不等于工具执行失败。
- UI 负责展示、选择、交互，不复制协议解析和风险规则。

JSON 序列化适配留在 CLI。只有确实需要第二个 C++ 调用方复用 JSON 输出时，才提取独立适配组件；StreamDocument 不含 to_json 或特定 JSON 库类型。

频繁调用导致重复解析成为测得的瓶颈时，再增加常驻请求模式及会话 ID。其会话表属于 CLI 服务模式，不属于纯解析引擎。不预建 RPC 框架、守护进程或线程池。

## 7. 测试边界

沿用 tests/CMakeLists.txt 里的 aced_add_test 与 tests/support/test_assertions.h。共享断言和构建注册，业务样本保持局部。

- ByteReader 测试只检查字节行为和越界状态。
- StreamParser 测试从公开 parse_stream 入口验证流头、内容、引用、错误和资源限额；不暴露私有函数供测试。
- StreamWriter/StreamEditor 测试验证重建和失败原子性；Analysis 测试验证规则证据；CLI 测试验证协议与退出状态。
- 测试文件可以按行为增长拆分，不要求生产文件等比例拆分。
- 同一模块多个测试实际共享时才增加模块专属 fixture；fixture 是样本或构造工具，不是测试基类。全局 support 不依赖 Java 协议。
- 同时保留手写精确样本和由 Java 生成、记录生成方式的样本；不能只用自身 StreamWriter 给自身 StreamParser 当正确性依据。
- Java 互操作测试仅使用已知测试类和可控样本，不在基础解析测试里执行任意输入。
- 模糊测试直接调用解析入口，保留触发崩溃、挂起、预算失效的输入。工具链支持时增加内存/未定义行为检查。
- 暂不为当前简单断言另造测试注册、参数化、发现机制；这些需求扩大时优先评估成熟测试框架。

运行测试前确认进程真的启动并执行断言。2026-09-06 此前测试中出现的 0xc0000135 是未解决的运行环境证据，不是通过证明。当前 stream_header.cpp 尚不完整，整工程现状也不能宣称可构建。

## 8. 渐进目录布局

以下是能力逐步实现后的布局，不要求现在创建所有文件。

```text
D:/Projects/Aced/
├── aced-core/
│   ├── CMakeLists.txt
│   ├── CMakePresets.json
│   ├── include/aced/
│   │   ├── byte_reader.h
│   │   ├── serialization/
│   │   │   ├── stream_document.h
│   │   │   ├── stream_parser.h
│   │   │   ├── stream_writer.h           [写出阶段]
│   │   │   └── stream_editor.h           [编辑阶段]
│   │   └── analysis/
│   │       ├── stream_query.h            [查询阶段]
│   │       └── stream_analyzer.h         [规则阶段]
│   ├── src/
│   │   ├── byte_reader.cpp
│   │   ├── serialization/
│   │   │   ├── stream_document.cpp
│   │   │   ├── stream_parser.cpp
│   │   │   ├── stream_writer.cpp
│   │   │   └── stream_editor.cpp
│   │   └── analysis/
│   │       ├── stream_query.cpp
│   │       ├── stream_analyzer.cpp
│   │       └── rules/             [规则数量增长后]
│   ├── cli/                      [inspect 阶段]
│   │   ├── main.cpp
│   │   └── json_output.cpp
│   ├── tests/
│   │   ├── CMakeLists.txt
│   │   ├── support/test_assertions.h
│   │   ├── byte_reader_test.cpp
│   │   ├── stream_parser_test.cpp
│   │   └── fixtures/             [有文件样本后]
│   ├── fuzz/                     [解析入口稳定后]
│   └── docs/architecture.md
└── aced-ui/                      [Rust UI 阶段]
```

遵循现有命名空间 Aced、Aced::Serialization、Aced::Test；Analysis 使用 Aced::Analysis。无需为风格统一额外迁移现有目录。

### 8.1 统一命名规则（2026-09-06 修订，替代早期简写）

业务文件使用小写 snake_case（单词以下划线分隔），按“对象 + 职责”命名。头文件和实现文件同名，测试使用相同主干加 _test。这里的 stream 指 Serialization 领域的 Java 序列化流，不是文件 I/O 流。

| 职责 | 文件主干 | 类型或入口 |
| --- | --- | --- |
| 字节读取 | byte_reader | ByteReader |
| 序列化流文档 | stream_document | StreamDocument |
| 序列化流解析 | stream_parser | parse_stream；内部 StreamParser |
| 序列化流写出 | stream_writer | 写出入口；有状态实现需要时使用 StreamWriter |
| 序列化文档编辑 | stream_editor | 编辑入口；确有会话状态时使用 StreamEditor |
| 序列化文档查询 | stream_query | 具名查询函数，不强制建立 StreamQuery 类 |
| 序列化文档分析 | stream_analyzer | 分析入口；确有状态时使用 StreamAnalyzer |
| 测试断言 | test_assertions | Aced::Test 中的断言函数 |
| JSON 输出 | json_output | CLI 内输出函数 |

不再用 parser.h、document.h、query.h 等缺少对象信息的名字。parser 是“解析器”的正确英文拼写，不使用 paise。文件命名一致不要求每个文件都封装成类；上文 StreamWriter/StreamEditor 也表示对应职责，不承诺必须存在同名公共类。

main.cpp、CMakeLists.txt、CMakePresets.json 及文档名称遵循各自惯例；不为了凑双词而改名。目录 serialization、analysis、support 是领域/分组名称，也不强行添加后缀。

本轮已将实际测试辅助头从 assertions.h 重命名为 test_assertions.h 并更新 include。stream_parser 等是规划名，目前尚无对应生产实现；现有未完成 stream_header 文件将在最小解析任务中合并替换，不能仅改文件名就把 HeaderStatus 接口伪装成完整解析器。

### 8.2 重命名后复检

1. 文件主干与职责一致：byte 读取字节，stream 处理序列化流，test 是测试设施，json 是输出格式。
2. 对应头文件、实现、测试统一主干；类型使用 PascalCase，函数使用 snake_case，避免文件/类型混用命名风格。
3. 依赖方向不变，重命名没有引入新类、包装层、构建目标或服务接口。
4. 流头校验依然留在解析器内部；没有因统一名字重新拆出公开校验模块。
5. 测试共享头仅由测试消费；实现接口不依赖它。
6. 本轮只核验命名和现有引用，不把命名通过等同于全部协议实现或测试通过。

公开头文件自包含且依赖尽量小。单一 C++ 核心内部不先采用 PImpl 来追求尚无需求的二进制 ABI 稳定，也不引入全局 Context、ServiceLocator、事件总线、自定义内存分配器或通用插件接口。

## 9. 实施顺序与完成条件

每个任务按 CMake → 接口 → 实现 → 验收一次给全，由用户实现；除非明确要求，不代写整个实现或跳多个功能。

1. 先解决测试运行环境并确认现有 ByteReader 测试实际通过。将未完成 stream_header 文件在下一任务中替换为 stream_parser，避免保留未完成目标。
2. 最小解析闭环：StreamDocument 的最小记录表示、统一 ParseResult、流头与 TC_NULL。未知后续标记明确不支持，不能静默忽略；流头本身不是完整功能模块。
3. 添加字符串、数组、类描述和对象；与引用/reset 支持配套，逐项扩充真实模型，避免为完整语法预写大量空类型。
4. 支持矩阵、截断/畸形输入、资源预算、模糊测试持续补齐；自定义序列化限制明确可见。
5. inspect CLI 与版本化 JSON，随后引入 Rust 浏览与检索 UI。
6. 查询、语义视图与证据规则；具备文档和实际需求后再做结构 diff。
7. 受支持写出与编辑、Java 互操作测试；操作失败保持原文档有效。
8. 按实际需求增加更多格式或 JVM 辅助。新格式拥有自己的模型/解析器；只有真实共享语义稳定后才抽公共报告视图，不把 Java StreamParser 泛化成所有协议的基础框架。

## 10. 三轮架构验证

### 第一轮：职责与状态所有权

| 检查 | 结论与修正 |
| --- | --- |
| 流头是否独立职责 | 否，合并进 StreamParser，删除公共 HeaderStatus 设计 |
| handle 表是否独立公共服务 | 否，属于一次解析/写出的私有状态 |
| ByteReader 是否值得保留 | 是，字节边界逻辑独立于 Java，现有单测可独立验证 |
| StreamDocument 是否过大 | 限于数据及其不变量，不加入解析、分析、输出操作 |
| 语义解释是否必须独立模块 | 目前否，放在 Analysis；解析所必需的布局知识仍归 Serialization |
| 通用断言是否值得提取 | 是，已是跨测试文件的基础能力，无业务依赖 |

### 第二轮：依赖与变化传播

| 变化场景 | 应修改范围 | 不应被牵连 |
| --- | --- | --- |
| 新增 TC_* 支持 | StreamParser、必要节点表示、对应测试 | ByteReader、UI 启动方式 |
| 新增风险规则 | Analysis 规则与测试 | StreamParser、StreamWriter |
| 修改 JSON 字段/版本 | CLI 适配与 Rust 消费者 | 核心对象身份与字节读取 |
| 新增 UI 页面 | Rust 展示/查询调用 | 协议解析实现 |
| 支持字符串编辑 | StreamEditor、StreamWriter、相关模型校验和测试 | ByteReader、规则执行入口 |
| 支持常驻查询 | CLI 会话与客户端 | parse_stream 的纯调用契约 |
| 支持另一格式 | 新格式实现与 CLI 分派 | 现有 Java 解析过程 |
| 增加测试文件 | tests 注册、局部样本 | 生产接口 |

模型变化合理地影响使用该数据的消费者，这不是零耦合；验收标准是不波及无关职责，也不出现循环依赖。

### 第三轮：反例与过度封装

| 反例/风险 | 处理方式 |
| --- | --- |
| reset 后 handle 相同，旧对象被覆盖 | 文档 ID 独立，解析 handle 表按协议重置 |
| 返回文档引用临时输入 | StreamDocument 拥有输入，内部保存偏移/ID |
| 一份模型复制成多套可变树图 | 节点为事实来源，顺序记录/索引只引用它 |
| 无法理解自定义区却继续猜解析 | 明确不透明区或停止，标注部分结果 |
| 修改后复用旧字节冒充保存 | 区分原样导出、有效重建和原始来源信息 |
| 修改后继续用旧索引 | 索引按操作构建，未来缓存显式绑定版本 |
| 多层嵌套各自重新获得预算 | 嵌套分析共享总预算 |
| 跨文件测试导致公共头全暴露 | 只测公开行为，局部 fixture 不进入生产库 |
| 类越来越多却只有单个使用方 | 不建 Manager/Factory/接口继承；按实际变化提取 |
| 目录拆很多个库增加构建耦合 | 维持一个核心目标，按实际发布需求再拆 |

三轮检查后未发现必须靠额外框架解决的边界问题。设计不以拆文件数量、类数量或虚构 95% 分数验收；用依赖审查、协议样本、失败状态、资源预算及扩展修改范围验证。特别是自定义序列化、编辑重建和跨样本 diff 仍是实现风险，不能由设计审查消除。

## 11. 官方依据

Java 的对象引用、reset、Modified UTF-8、自定义块数据与 Externalizable 限制依据：

- [Java Object Serialization Stream Protocol](https://docs.oracle.com/en/java/javase/25/docs/specs/serialization/protocol.html)
- [Java Object Input Classes](https://docs.oracle.com/en/java/javase/25/docs/specs/serialization/input.html)

官方协议证明格式行为；本文件的模块划分、构建边界和实施顺序是针对当前项目的工程设计判断。
