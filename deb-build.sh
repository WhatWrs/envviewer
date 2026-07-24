#!/bin/bash

read -p"请输入版本号:（x.x.x）:" VERSION



#复制可执行文件
cp src/build/unknown-Release/envviewer  envviwer-deb/opt/envviewer/envviewer

if [[ ! -d envviwer-deb/usr/share/icons/hicolor ]];then
    cp -r res/hicolor envviwer-deb/usr/share/icons
    cp -r res/hicolor/envviewer.png envviwer-deb/usr/share/pixmaps #以防缓存不生效
else
    echo "目录已经存在"
fi

install -m644  res/envviewer.desktop envviwer-deb/usr/share/applications/envviewer.desktop



#dpkg-deb --build   envviwer-deb envviwer_amd64_${VERSION}.deb
dpkg-deb --build --root-owner-group  envviwer-deb envviwer_amd64_${VERSION}.deb

read -p "即将安装 envviwer_amd64_${VERSION}.deb，是否继续？(y/n) " confirm
if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
    echo "安装已取消。"
    exit 0   # 或 exit 1，取决于你想怎么退出
fi

# 用户输入了 y 或 Y，执行安装
sudo dpkg -i "envviwer_amd64_${VERSION}.deb"
