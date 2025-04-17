# LiteLdacsSDK
[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

LiteLdacs开发所需依赖，适用于libldcauc以及ldacs-combine项目

---

## 1. 依赖

1. 安装libyaml、libevent、uthash、libcjson
```shell
sudo apt update && sudo apt upgrade
sudo apt install libyaml-dev libevent-dev uthash-dev libcjson-dev
```
2. 拉取并安装base64
```shell
git clone https://github.com/aklomp/base64.git
cd base64 && mkdir build && cd build
cmake ..
make -j12 && sudo make install
sudo ldconfig
```
---
## 2. 安装

安装本工具
```shell
git clone https://github.com/liteldacs/liteldacssdk.git
cd liteldacssdk && mkdir build && cd build
cmake ..
make -j12 && sudo make install
```
---

## 作者

中国民航大学新航行系统研究所


