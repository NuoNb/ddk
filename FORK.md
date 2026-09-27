# 本 Fork 说明

上游：[Ylarod/ddk](https://github.com/Ylarod/ddk)。本 fork 在其基础上做了三件事：

1. **镜像产线自主化**：镜像链（builder → toolchain → ddk/ddk-min）各级 `FROM` 参数化，
   默认推送到本仓库 ghcr 命名空间；`release.yml` 补齐 GHCR 登录与 `packages:write`
   （上游原版无登录无法在 GitHub 出包），并修复 LFS 必须进 `prebuilts` 子模块拉取
   （clang+rust）的问题，附 tarball 体积自检；镜像 tag 支持 `-日期` 后缀供消费方钉版本。
2. **镜像内置加载回归工具**：ddk 镜像追加 `qemu-system-arm` 与
   `/test/{run.sh,init,build-initramfs.py}`，配合镜像自带内核（kdir 的
   `arch/arm64/boot/Image`），实现"拉镜像即可测"。
3. **提供可复制的 CI 模板**：[templates/qemu-load-test.yml](templates/qemu-load-test.yml)。

## 模块仓库接入加载回归（两分钟）

1. 复制 `templates/qemu-load-test.yml` 到你的模块仓库 `.github/workflows/`；
2. Actions 手动触发（填 `kmi` 与镜像 tag），或把 `on:` 改成 push 自动跑；
3. 结果三态：`PASS: MODULE LOADED OK` / 内核崩溃（CI 日志含完整 panic 栈）/
   被加载器拒绝（CI 日志含 errno 与 version magic 行）。

## 手动 / 本地使用

```bash
docker run --rm -v "$PWD":/ws -w /ws ghcr.io/nuonb/ddk:android16-6.12-20260927 \
  make KDIR=/opt/ddk/kdir/android16-6.12 && \
docker run --rm -v "$PWD":/ws -w /ws ghcr.io/nuonb/ddk:android16-6.12-20260927 \
  /test/run.sh hack.ko android16-6.12
```

## 出新镜像

1. Sync fork（同步上游 mapping/工具链更新）；
2. 手动触发 `release.yml`（`kmi_filter` 可只重出单个 KMI，留空全量）；
3. 新 tag 形如 `<kmi>-<UTC日期>`（如 `android16-6.12-20260927`）。

镜像以公开包发布（公开仓库自动继承公开），消费方匿名拉取即可，无需凭据。
