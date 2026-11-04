# 旧 Nginx 模式 1 配置

该配置在当前 LibDASICS 仓库内复现已审计的旧 Nginx 运行方式。`make PROFILE=nginx-mode1` 使用共享源码中的 `DASICS_NGINX_MODE1` 分支和 OpenSSL/PCRE 策略文件；公共头文件、ISA 与上下文实现只有一份。默认配置和 `spec-static` 保持各自行为。集成构建入口为顶层 `scr/dasics_validation/nginx_build.py`。

采用旧 stage1=1、stage2 无复制路径、GOT 元数据槽位 2、单份可信 libc、可信目标快速跳转、OpenSSL 合并域和旧专用堆。stage3 符号保留，不代表启用复制；本配置没有定义 `DASICS_COPY`。旧库二进制与 glibc 不修改。

策略文件来源列于 `SOURCE.json`。除以下接口适配，旧授权、调用、异常恢复和分配器逻辑保持原样：

- CSR/异常常量和 custom3 指令编码复用当前 `include/dasics_isa.h`；旧重复字段定义由此替代，数值不变。
- trap 使用当前完整整数/浮点/返回状态上下文；旧 fetch/startup 处理器对返回 CSR 的修改改写到保存的 `return_pc` / `freezone_return_pc`，再统一恢复。
- 保留旧 syscall 分流：启动与应用阶段都直接调用旧 ECALL 检查函数，忽略其返回值，再按当前 restore-first 接口请求代理。没有采用新版的拒绝决策或新增参数检查。
- maincall 保留旧分派及可信服务快速路径；需要保存上下文的慢路径使用当前 448 字节布局，读取旧 ELF 指针的参数改从保存的 t0 获取。`abi.c` 验证布局和旧分派表前缀。旧短返回帧也使用当前完整布局。
- `lib_call` 的唯一指令变化为旧 `0x0005108b` → 当前 `DASICS_FDICALL_JR_RA_A0` (`0x000510fb`)；不修改旧参数/栈语义。
- `umaincall_check_handler` 沿用当前独立类型名，避免与 syscall 的同名 typedef 冲突；旧分派函数签名和当前上下文结构组合在公共头文件的模式分支中。

启动宽授权上限仍为旧 `0x4000000000`，未采用新版默认的 `0x800000000000`。若真实平台的 mmap/栈布局超出旧范围，需记录实际地址后另行审查，不在本步扩大授权。

旧行为也包含：读写故障在无法换入旧软件授权后跳过故障指令；取指故障恢复返回目标；原有随机换入、长期授权启停和未修改的分配器边界/释放语义。本配置不提供新版严格拒绝或更精细的隔离保证。初始化/退出的 a0 清零保持旧入口行为。

最小动态测试链接原有 libssl/libcrypto，是因为旧 stage2 会初始化 OpenSSL 钩子并查找两库；未添加为测试绕过旧初始化的分支。PCRE 缺失时走旧 `init_pcre()` 原有返回分支。完整 Nginx 构建与运行见顶层 `docs/dasics-integration/nginx.md`。

链接本配置的程序也须定义 `DASICS_NGINX_MODE1` 并使用公共 `include` 目录；集成构建入口已经传入该宏。默认与模式 1 使用独立构建目录。
