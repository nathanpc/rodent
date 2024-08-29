#!/bin/sh

find ./svg -type f -name "*.svg" -exec svg2icns {} \;
