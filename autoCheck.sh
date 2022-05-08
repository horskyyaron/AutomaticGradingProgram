#!/bin/bash

echo ---------------------------------------------------
echo DIFFERENT
echo ---------------------------------------------------
./comp.out ./textComparison/different/1/1.txt ./textComparison/different/1/2.txt
./comp.out ./textComparison/different/2/1.txt ./textComparison/different/2/2.txt
./comp.out ./textComparison/different/3/1.txt ./textComparison/different/3/2.txt
echo ---------------------------------------------------

echo ---------------------------------------------------
echo SIMILAR 
echo ---------------------------------------------------
./comp.out ./textComparison/similar/1/1.txt ./textComparison/similar/1/2.txt
./comp.out ./textComparison/similar/2/1.txt ./textComparison/similar/2/2.txt
echo ---------------------------------------------------

echo ---------------------------------------------------
echo IDENTICAL
echo ---------------------------------------------------
./comp.out ./textComparison/identical/1/1.txt ./textComparison/identical/1/2.txt
./comp.out ./textComparison/identical/2/1.txt ./textComparison/identical/2/2.txt
echo ---------------------------------------------------
