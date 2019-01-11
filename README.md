# eosbancor
一个基于Bancor协议的去中心化交易所智能合约


## 清空数据库
```
cleos  transfer dddddddadmin  ddddcontract  '0.0001 EOS' 'opt=clear_db'
```

## 创建市场
```
cleos transfer dddddddadmin  ddddcontract  '100.0000 EOS' ''
cleos push action dddddddadmin transfer '["dddddddadmin", "ddddcontract", "1000.0000 USD", "" ]' -p dddddddadmin
cleos push action dddddddadmin transfer '["dddddddadmin", "ddddcontract", "0.0001 USD", "opt=create_market&eos_amount=1000000&token_amount=10000000" ]' -p dddddddadmin
```
## 打开市场
```
cleos push action dddddddadmin transfer '["dddddddadmin", "ddddcontract", "0.0001 USD", "opt=open_market" ]' -p dddddddadmin
```

## 买USD
```
cleos  transfer ddddddduser1  ddddcontract  '2.0000 EOS' 'USD'
```

## 卖 USD
```
cleos push action dddddddadmin transfer '["ddddddduser2", "ddddcontract", "1.0000 USD", "" ]' -p ddddddduser2
```

## 提取手续费
```
cleos  transfer dddddddadmin  ddddcontract  '0.0001 EOS' 'opt=take_fee&limit=1'
```
