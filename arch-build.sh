#!/bin/bash


cp -r  res/* envviewer-arch
install -m644  envviewer.sh envviewer-arch/envviewer.sh
install -m644  envviewer/build/unknown-Release/envviewer envviewer-arch
cd envviewer-arch

tar -cvf envviewer.tar envviewer.sh envviewer envviewer.desktop hicolor
makepkg -sfi
rm -r envviewer.tar envviewer.sh envviewer envviewer.desktop hicolor
rm -r src pkg

