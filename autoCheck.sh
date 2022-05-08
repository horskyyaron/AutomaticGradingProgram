#!/bin/bash

echo ---------------------------------------------------
echo DIFFERENT
echo ---------------------------------------------------
./prog ./textComparison/different/1/1.txt ./textComparison/different/1/2.txt
./prog ./textComparison/different/2/1.txt ./textComparison/different/2/2.txt
./prog ./textComparison/different/3/1.txt ./textComparison/different/3/2.txt
echo ---------------------------------------------------

echo ---------------------------------------------------
echo SIMILAR 
echo ---------------------------------------------------
./prog ./textComparison/similar/1/1.txt ./textComparison/similar/1/2.txt
./prog ./textComparison/similar/2/1.txt ./textComparison/similar/2/2.txt
echo ---------------------------------------------------

echo ---------------------------------------------------
echo IDENTICAL
echo ---------------------------------------------------
./prog ./textComparison/identical/1/1.txt ./textComparison/identical/1/2.txt
./prog ./textComparison/identical/2/1.txt ./textComparison/identical/2/2.txt
echo ---------------------------------------------------
