#!/usr/bin/env bash
set -e

VENV="$HOME/venv/frr-doc"

echo "[*] Recreating FRR doc venv at $VENV"

# 1. 删除旧环境（如果存在）
if [ -d "$VENV" ]; then
    echo "[*] Removing existing venv"
    rm -rf "$VENV"
fi

# 2. 创建新 venv
python3 -m venv "$VENV"

# 3. 激活
source "$VENV/bin/activate"

# 4. 升级 pip（保持适中，不追最新）
pip install --upgrade "pip<24"

# 5. 安装【FRR 10.0.1 developer 文档】已验证可用组合
pip install \
    sphinx==3.5.4 \
    jinja2==3.0.3 \
    docutils==0.16 \
    sphinx-rtd-theme==0.5.2 \
    pygments \
    babel \
    snowballstemmer \
    imagesize \
    requests

echo
echo "[OK] FRR doc venv ready"
echo "Activate with: source $VENV/bin/activate"
echo "Build with:    make -C doc/developer html"

