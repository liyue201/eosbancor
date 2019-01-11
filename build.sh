#!/bin/bash


eosiocpp -o eosbancor.wast exchange.cpp market.cpp

eosiocpp -g eosbancor.abi market.cpp
