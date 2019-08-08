# Eos Bancor
全球首个EOS主链上基于Bancor协议的去中心化交易于2018-09-16上线。日活跃用户量曾一度排在[DappRadar](https://dappradar.com/)前十。  
因其已完成其历史使命，大部分代币已经下架，故将其智能合约开源，前端页面只是用来展示数据的，可有可无。

交易所地址:   http://eosbancor.top ， 需翻墙，目前显示的只是个demo。  
合约账号:   [buttonbutton](https://eospark.com/account/buttonbutton) 

## Featrue
无需注册，无需充值，链上结算，实时交易。

# Installation
## prepare

### create test accounts
```
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio buttonbutton EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio gooooooooooe EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio btncontract EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio dicecontract EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio bob EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
```
### create & issue tokens
```
CONTRACTS_DIR=/home/stirling/contracts
cleos set contract eosio.token ${CONTRACTS_DIR}/eosio.contracts/eosio.token --abi eosio.token.abi -p eosio.token
cleos set contract btncontract ${CONTRACTS_DIR}/eosio.contracts/eosio.token --abi eosio.token.abi -p btncontract
cleos set contract dicecontract ${CONTRACTS_DIR}/eosio.contracts/eosio.token --abi eosio.token.abi -p dicecontract

cleos push action eosio.token create '[ "eosio.token", "1000000000.0000 EOS"]' -p eosio.token
cleos push action btncontract create '[ "btncontract", "1000000000.0000 BTN"]' -p btncontract
cleos push action dicecontract create '[ "dicecontract", "1000000000.0000 DICE"]' -p dicecontract

cleos push action eosio.token issue '[ "gooooooooooe", "100000.0000 EOS", "memo" ]' -p eosio.token
cleos push action eosio.token issue '[ "bob", "100000.0000 EOS", "memo" ]' -p eosio.token

cleos push action btncontract issue '[ "gooooooooooe", "100000.0000 BTN", "memo" ]' -p btncontract
cleos push action dicecontract issue '[ "gooooooooooe", "100000.0000 DICE", "memo" ]' -p dicecontract
```

### show account's balance
```
cleos get currency balance eosio.token gooooooooooe 
cleos get currency balance btncontract gooooooooooe 
cleos get currency balance dicecontract gooooooooooe 
cleos get currency balance eosio.token bob 
```

## build eosbancor contract

```
./build.sh
```

## deploy eosbancor contract
```
cleos set contract buttonbutton ${CONTRACTS_DIR}/eosbancor --abi eosbancor.abi -p buttonbutton
cleos set account permission buttonbutton active '{"threshold": 1,"keys": [{"key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","weight": 1}],"accounts": [{"permission":{"actor":"buttonbutton","permission":"eosio.code"},"weight":1}]}' owner -p buttonbutton
```

## test eosbancor contract
### create BTN market
```
cleos transfer gooooooooooe  buttonbutton  '100.0000 EOS' ''
cleos push action btncontract transfer '["gooooooooooe", "buttonbutton", "1000.0000 BTN", "" ]' -p gooooooooooe
cleos push action btncontract transfer '["gooooooooooe", "buttonbutton", "0.0001 BTN", "opt=create_market&market_name=BTN&eos_amount=1000000&token_amount=10000000" ]' -p gooooooooooe
```
### show markets
```
cleos get table buttonbutton buttonbutton markets
```

### open BTN market

```
cleos push action btncontract transfer '["gooooooooooe", "buttonbutton", "0.0001 BTN", "opt=open_market"]' -p gooooooooooe
```

###  buy BTN
```
cleos push action eosio.token transfer '["bob", "buttonbutton", "100.0000 EOS", "BTN"]' -p bob
```

### sell BTN
```
cleos push action btncontract transfer '["bob", "buttonbutton", "100.0000 BTN", ""]' -p bob
```

###  close BTN market
```
cleos push action btncontract transfer '["gooooooooooe", "buttonbutton", "0.0001 BTN", "opt=close_market"]' -p gooooooooooe
```
###  update BTN market
```
cleos push action btncontract transfer '["gooooooooooe", "buttonbutton", "0.0001 BTN", "opt=update_market&market_name=BTN&eos_amount=1000000&token_amount=10000000" ]' -p gooooooooooe
```

### create DICE market

```
cleos push action dicecontract transfer '["gooooooooooe", "buttonbutton", "1000.0000 DICE", "" ]' -p gooooooooooe
cleos push action dicecontract transfer '["gooooooooooe", "buttonbutton", "0.0001 DICE", "opt=create_market&market_name=DICE&eos_amount=1000000&token_amount=10000000" ]' -p gooooooooooe
cleos push action dicecontract transfer '["gooooooooooe", "buttonbutton", "0.0001 DICE", "opt=open_market"]' -p gooooooooooe
```

### buy DICE
```
cleos push action eosio.token transfer '["bob", "buttonbutton", "100.0000 EOS", "DICE"]' -p bob
```

### clear data
```
cleos push action buttonbutton clear '[]' -p gooooooooooe
```
