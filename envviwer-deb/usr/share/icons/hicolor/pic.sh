#!/bin/bash

for size in 16  32  64  128 256 512 1024; do
    mkdir -p  ${size}x${size}/app
    convert envviewer.png -resize ${size}x${size} ${size}x${size}/app/envviewer.png
done
