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


sudo dpkg -i envviwer_amd64_${VERSION}.deb
