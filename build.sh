#!/bin/bash

eosio-cpp -abigen -I . market.cpp  exchange.cpp  -o eosbancor.wasm
