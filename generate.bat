@echo off
git submodule update --init --recursive 2>nul
tools\premake5 %* vs2022
