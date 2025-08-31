# Telegram 验证机器人

这是一个用于 Telegram 群组的验证机器人，可以防止机器人和垃圾信息。

## 功能特性

- 新用户加入群组时自动限制权限
- 通过私聊进行算术题验证
- 验证成功后自动恢复用户权限
- 超时自动踢出未验证用户

## 编译和运行

### 前置要求

- CMake 3.15+
- C++17 编译器
- TgBot C++ 库
- Boost 库

### 编译步骤

1. 安装依赖：
```bash
sudo apt update
sudo apt install -y libboost-all-dev
```

2. 编译项目：
```bash
mkdir build
cd build
cmake ..
make
```

### 配置和运行

1. 在 `src/main.cpp` 中替换 `BOT_TOKEN` 为你的机器人令牌
2. 运行程序：
```bash
./VerificationBot
```

## 使用说明

1. 将机器人添加到群组并给予管理员权限
2. 新用户加入时会自动收到验证消息
3. 用户点击按钮进入私聊完成算术题验证
4. 验证成功后自动恢复群组权限

## 注意事项

- 确保机器人有足够的管理员权限
- 验证超时时间默认为5分钟
- 需要替换代码中的机器人令牌才能正常使用