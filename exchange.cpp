#include "exchange.hpp"
#include <string>

#define EOS_CONTRACT "eosio.token"_n
#define EOS_SYMBOL symbol("EOS", 4)
#define ADMIN "gooooooooooe"
#define FEE_ACCOUNT "gooooooooooe"

string symbol_to_string(int sym)
{
    string s;
    for (int i = 0; i < 7; ++i) {
        char c = (char)(sym & 0xff);
        if (c == 0) {
            break;
        }
        s += c;
        sym >>= 8;
    }
    return s;
}

uint128_t name_to_num(string s)
{
    uint128_t num = 0;
    for (auto i = 0; i < s.length(); i++) {
        num = (num << 8) + s[i];
    }
    return num;
}

exchange::exchange(name self, name first_receiver,
    eosio::datastream<const char*> ds)
    : contract(self, first_receiver, ds)
{
}

void exchange::transfer(name from, name to, asset quantity, string memo)
{
    require_auth(from);

    if (from == _self || to != _self) {
        return;
    }

    eosio::check(quantity.is_valid(), "invalid quantity");

    auto admin = eosio::name(ADMIN);

    if (from == admin) {
        memo_param param;
        parse_memo_param(memo, param);

        if (param.opt == OPT_CREATE_MARKET) {
            asset eos_quant(param.eos_amount, EOS_SYMBOL);
            asset token_quant(param.token_amount, quantity.symbol);
            create_market(param.market_name, eos_quant, _first_receiver, token_quant);
        } else if (param.opt == OPT_OPEN_MARKET) {
            open_market(_first_receiver, quantity.symbol, true);
        } else if (param.opt == OPT_CLOSE_MARKET) {
            open_market(_first_receiver, quantity.symbol, false);
        } else if (param.opt == OPT_UPDATE_MARKET) {
            asset eos_quant(param.eos_amount, EOS_SYMBOL);
            asset token_quant(param.token_amount, quantity.symbol);
            update_market(eos_quant, _first_receiver, token_quant);
        } else {
            // eosio::check(1, "invalid opt");
            return;
        }
    } else {
        if (_first_receiver == EOS_CONTRACT && quantity.symbol == EOS_SYMBOL) {
            buy_token(from, quantity, memo);
        } else {
            sell_token(from, _first_receiver, quantity);
        }
    }
}

void exchange::clear()
{
    require_auth(name(ADMIN));

    markets _market(_self, _self.value);
    for (auto itr = _market.begin(); itr != _market.end();) {
        itr = _market.erase(itr);
    }
}

void exchange::create_market(string market_name, asset eos_quant,
    name token_contract, asset token_quant)
{
    eosio::check(market_name.length() > 0 && market_name.length() <= 10, "invalid market_name");
    eosio::check(token_quant.amount > 0, "invalid token_quant amount");
    eosio::check(token_quant.symbol.is_valid(), "invalid token_quant symbol");
    eosio::check(token_quant.symbol != EOS_SYMBOL, "token_quant symbol cannot be EOS");
    eosio::check(eos_quant.amount > 0, "invalid eos_quant amount");
    eosio::check(eos_quant.symbol == EOS_SYMBOL, "eos_quant symbol only support EOS");

    markets _market(_self, _self.value);

    uint128_t idxkey = (uint128_t(token_contract.value) << 64) | token_quant.symbol.raw();

    auto idx = _market.template get_index<"idxkey"_n>();
    auto itr = idx.find(idxkey);

    eosio::check(itr == idx.end(), "token market already created");

    auto idxname = _market.template get_index<"idxname"_n>();

    eosio::check(idxname.find(name_to_num(market_name)) == idxname.end(), "token market already created");

    auto pk = _market.available_primary_key();

    _market.emplace(_self, [&](auto& m) {
        m.id = pk;
        m.idxkey = idxkey;
        m.market_name = market_name;
        m.supply.amount = 100000000000000ll;
        m.supply.symbol = symbol("GOD", 4);
        m.open = false;
        m.base.contract = token_contract;
        m.base.balance.amount = token_quant.amount;
        m.base.balance.symbol = token_quant.symbol;
        m.quote.contract = EOS_CONTRACT;
        m.quote.balance.amount = eos_quant.amount;
        m.quote.balance.symbol = eos_quant.symbol;
    });
}

void exchange::update_market(asset eos_quant, name token_contract,
    asset token_quant)
{
    eosio::check(token_quant.amount > 0, "invalid token_quant amount");
    eosio::check(token_quant.symbol.is_valid(), "invalid token_quant symbol");
    eosio::check(token_quant.symbol != EOS_SYMBOL, "token_quant symbol cannot be EOS");
    eosio::check(eos_quant.amount > 0, "invalid eos_quant amount");
    eosio::check(eos_quant.symbol == EOS_SYMBOL, "eos_quant symbol only support EOS");

    markets _market(_self, _self.value);
    uint128_t idxkey = (uint128_t(token_contract.value) << 64) | token_quant.symbol.raw();
    auto idx = _market.template get_index<"idxkey"_n>();
    auto itr = idx.find(idxkey);

    eosio::check(itr != idx.end(), "token market does not exist");

    _market.modify(*itr, _self, [&](auto& es) {
        es.open = 0;
        es.base.balance.amount = token_quant.amount;
        es.quote.balance.amount = eos_quant.amount;
    });
}

void exchange::open_market(name token_contract, symbol token_symbol,
    bool open)
{
    markets _market(_self, _self.value);

    uint128_t idxkey = (uint128_t(token_contract.value) << 64) | token_symbol.raw();

    auto idx = _market.template get_index<"idxkey"_n>();
    auto itr = idx.find(idxkey);

    eosio::check(itr != idx.end(), "token market not exist");

    _market.modify(*itr, _self, [&](auto& es) { es.open = open; });
}

void exchange::buy_token(name payer, const asset& eos_quant, string token_symbol_string)
{
    eosio::check(eos_quant.amount >= 1000, "at least 0.1 EOS");
    eosio::check(eos_quant.symbol == EOS_SYMBOL, "eos_quant symbol must be EOS");
    eosio::check(token_symbol_string.length() >= 3 && token_symbol_string.length() <= 8, "invalid token_symbol_string");

    markets _market(_self, _self.value);
    auto idx = _market.template get_index<"idxname"_n>();
    auto itr = idx.find(name_to_num(token_symbol_string));
    eosio::check(itr != idx.end(), "token market does not exist");
    auto market = *itr;
    eosio::check(market.open, "token market does not open");

    auto token_contract = market.base.contract;
    auto token_symbol = market.base.balance.symbol;

    asset token_out{ 1, token_symbol };

    _market.modify(*itr, _self, [&](auto& es) {
        token_out = es.convert(eos_quant, token_symbol);
    });

    eosio::check(token_out.amount > 0, "must reserve a positive amount");
    eosio::check(token_symbol == token_out.symbol, "token_out symbol error");

    int64_t fee_amount = token_out.amount * FEE_RATE;

    if (fee_amount <= 0) {
        fee_amount = 1;
    }

    token_out.amount = token_out.amount - fee_amount;

    action(permission_level{ _self, "active"_n },
        token_contract,
        "transfer"_n,
        std::make_tuple(_self, payer, token_out, std::string("receive token from eosbancor")))
        .send();

    if (fee_amount > 0) {
        asset fee{ fee_amount, token_symbol };
        action(permission_level{ _self, "active"_n },
            token_contract,
            "transfer"_n,
            std::make_tuple(_self, eosio::name(FEE_ACCOUNT), fee, std::string("receive fee from eosbancor")))
            .send();
    }
}

void exchange::sell_token(name receiver, name token_contract, asset quant)
{
    eosio::check(quant.symbol.is_valid(), "invalid token_symbol");
    eosio::check(quant.symbol != EOS_SYMBOL,
        "eos_quant symbol must not be EOS");
    eosio::check(quant.amount >= 1000000, "at lest 100 token");

    markets _market(_self, _self.value);
    uint128_t idxkey = (uint128_t(token_contract.value) << 64) | quant.symbol.raw();
    auto idx = _market.template get_index<"idxkey"_n>();
    auto itr = idx.find(idxkey);
    eosio::check(itr != idx.end(), "token market does not exist");

    auto market = *itr;
    eosio::check(market.open, "token market does not open");

    auto eos_contract = market.quote.contract;
    auto eos_symbol = market.quote.balance.symbol;

    asset eos_out{ 0, eos_symbol };
    _market.modify(*itr, _self,
        [&](auto& es) { eos_out = es.convert(quant, eos_symbol); });

    eosio::check(eos_out.amount > 0, "token amount received from selling EOS is too low");

    int64_t fee_amount = eos_out.amount * FEE_RATE;
    if (fee_amount <= 0) {
        fee_amount = 1;
    }
    eos_out.amount = eos_out.amount - fee_amount;

    action(permission_level{ _self, "active"_n },
        eos_contract,
        "transfer"_n,
        std::make_tuple(_self, receiver, eos_out, std::string("receive EOS from bancor")))
        .send();

    if (fee_amount > 0) {
        asset fee{ fee_amount, eos_symbol };
        action(permission_level{ _self, "active"_n },
            eos_contract,
            "transfer"_n,
            std::make_tuple(_self, eosio::name(FEE_ACCOUNT), fee, std::string("receive fee from eosbancor")))
            .send();
    }
}

void split(vector<string>& ret, const string& str, string sep)
{
    if (str.empty()) {
        return;
    }
    string tmp;
    string::size_type pos_begin = str.find_first_not_of(sep);
    string::size_type comma_pos = 0;

    while (pos_begin != string::npos) {
        comma_pos = str.find(sep, pos_begin);
        if (comma_pos != string::npos) {
            tmp = str.substr(pos_begin, comma_pos - pos_begin);
            pos_begin = comma_pos + sep.length();
        } else {
            tmp = str.substr(pos_begin);
            pos_begin = comma_pos;
        }

        if (!tmp.empty()) {
            ret.push_back(tmp);
            tmp.clear();
        }
    }
}

void exchange::parse_memo_param(string memo, memo_param& param)
{
    vector<string> pairs;
    split(pairs, memo, "&");
    for (int i = 0; i < pairs.size(); i++) {
        vector<string> pair;
        split(pair, pairs[i], "=");
        if (pair.size() > 1) {
            if (pair[0] == "opt") {
                param.opt = pair[1];
            } else if (pair[0] == "eos_amount") {
                param.eos_amount = std::stoull(pair[1], nullptr, 0);
            } else if (pair[0] == "token_amount") {
                param.token_amount = std::stoull(pair[1], nullptr, 0);
            } else if (pair[0] == "limit") {
                param.limit = std::stoull(pair[1], nullptr, 0);
            } else if (pair[0] == "market_name") {
                param.market_name = pair[1];
            }
        }
    }
}

extern "C" {

[[eosio::wasm_entry]] void apply(uint64_t receiver, uint64_t code,
    uint64_t action) {
    if (action == "transfer"_n.value) {
        switch (action) {
            EOSIO_DISPATCH_HELPER(exchange, (transfer))
        }
        return;
    }

    if (code == receiver) {
        switch (action) {
            EOSIO_DISPATCH_HELPER(exchange, (clear))
        }
    }
}
} // extern "C" {
