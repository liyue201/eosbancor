#include "exchange.hpp"
#include "market.hpp"
#include <iostream>
#include <string>

namespace dex {

#define EOS_CONTRACT N(eosio.token)
#define EOS_SYMBOL S(4, EOS)
#define ADMIN "gooooooooooe"

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

exchange::exchange(account_name self)
    : contract(self)
{
}

void exchange::clear_db()
{
    markets _market(_self, _self);
    for (auto itr = _market.begin(); itr != _market.end();) {
        itr = _market.erase(itr);
    }

    fees _fee(_self, _self);
    for (auto itr = _fee.begin(); itr != _fee.end();) {
        itr = _fee.erase(itr);
    }
}

void exchange::create_market(string market_name, asset eos_quant, account_name token_contract, asset token_quant)
{
    eosio_assert(market_name.length() > 0 && market_name.length() <= 10, "invalid market_name");
    eosio_assert(token_quant.amount > 0, "invalid token_quant amount");
    eosio_assert(token_quant.symbol.is_valid(), "invalid token_quant symbol");
    eosio_assert(token_quant.symbol != EOS_SYMBOL, "token_quant symbol cannot be EOS");
    eosio_assert(eos_quant.amount > 0, "invalid eos_quant amount");
    eosio_assert(eos_quant.symbol == EOS_SYMBOL, "eos_quant symbol only support EOS");

    markets _market(_self, _self);

    uint128_t idxkey = (uint128_t(token_contract) << 64) | token_quant.symbol.value;

    auto idx = _market.template get_index<N(idxkey)>();
    auto itr = idx.find(idxkey);

    eosio_assert(itr == idx.end(), "token market already created");

    auto idxname = _market.template get_index<N(idxname)>();

    eosio_assert(idxname.find(name_to_num(market_name)) == idxname.end(), "token market already created");

    auto pk = _market.available_primary_key();

    _market.emplace(_self, [&](auto& m) {
        m.id = pk;
        m.idxkey = idxkey;
        m.name = market_name;
        m.supply.amount = 100000000000000ll;
        m.supply.symbol = S(0, GOD);
        m.open = false;
        m.base.contract = token_contract;
        m.base.balance.amount = token_quant.amount;
        m.base.balance.symbol = token_quant.symbol;
        m.quote.contract = N(eosio.token);
        m.quote.balance.amount = eos_quant.amount;
        m.quote.balance.symbol = eos_quant.symbol;
    });
}

void exchange::update_market(asset eos_quant, account_name token_contract, asset token_quant)
{
    eosio_assert(token_quant.amount > 0, "invalid token_quant amount");
    eosio_assert(token_quant.symbol.is_valid(), "invalid token_quant symbol");
    eosio_assert(token_quant.symbol != EOS_SYMBOL, "token_quant symbol cannot be EOS");
    eosio_assert(eos_quant.amount > 0, "invalid eos_quant amount");
    eosio_assert(eos_quant.symbol == EOS_SYMBOL, "eos_quant symbol only support EOS");

    markets _market(_self, _self);
    uint128_t idxkey = (uint128_t(token_contract) << 64) | token_quant.symbol.value;
    auto idx = _market.template get_index<N(idxkey)>();
    auto itr = idx.find(idxkey);

    eosio_assert(itr != idx.end(), "token market does not exist");

    _market.modify(*itr, 0, [&](auto& es) {
        es.open = 0;
        es.base.balance.amount = token_quant.amount;
        es.quote.balance.amount = eos_quant.amount;
    });
}

void exchange::open_market(account_name token_contract, uint64_t token_symbol, bool open)
{
    markets _market(_self, _self);

    uint128_t idxkey = (uint128_t(token_contract) << 64) | token_symbol;

    auto idx = _market.template get_index<N(idxkey)>();
    auto itr = idx.find(idxkey);

    eosio_assert(itr != idx.end(), "token market not exist");

    _market.modify(*itr, 0, [&](auto& es) {
        es.open = open;
    });
}

void exchange::buy_token(account_name payer, const asset& eos_quant, string token_symbol_string)
{
    eosio_assert(eos_quant.amount >= 1000, "at least 0.1 oes");
    eosio_assert(eos_quant.symbol == EOS_SYMBOL, "eos_quant symbol must be EOS");
    eosio_assert(token_symbol_string.length() >= 3 && token_symbol_string.length() <= 8, "invalid token_symbol_string");

    markets _market(_self, _self);
    auto idx = _market.template get_index<N(idxname)>();
    auto itr = idx.find(name_to_num(token_symbol_string));
    eosio_assert(itr != idx.end(), "token market does not exist");
    auto market = *itr;
    eosio_assert(market.open, "token market does not open");

    uint64_t token_contract = market.base.contract;
    uint64_t token_symbol = market.base.balance.symbol;

    asset token_out{ 1, token_symbol };

    _market.modify(*itr, 0, [&](auto& es) {
        token_out = es.convert(eos_quant, token_symbol);
    });

    eosio_assert(token_out.amount > 0, "must reserve a positive amount");
    eosio_assert(token_symbol == token_out.symbol, "token_out symbol error");

    int64_t fee_amount = token_out.amount * FEE_RATE;

    if (fee_amount <= 0) {
        fee_amount = 1;
    }

    token_out.amount = token_out.amount - fee_amount;

    action(permission_level{ _self, N(active) },
        token_contract, N(transfer),
        std::make_tuple(_self, payer, token_out, std::string("receive token from bancor")))
        .send();

    if (fee_amount > 0) {
        add_fee(fee_amount, token_contract, token_symbol);
    }
}

void exchange::sell_token(account_name receiver, account_name token_contract, asset quant)
{
    eosio_assert(quant.symbol.is_valid(), "invalid token_symbol");
    eosio_assert(quant.symbol != S(4, EOS), "eos_quant symbol must not be EOS");
    eosio_assert(quant.amount >= 1000000, "at lest 100 token");

    markets _market(_self, _self);
    uint128_t idxkey = (uint128_t(token_contract) << 64) | quant.symbol.value;
    auto idx = _market.template get_index<N(idxkey)>();
    auto itr = idx.find(idxkey);
    eosio_assert(itr != idx.end(), "token market does not exist");

    auto market = *itr;
    eosio_assert(market.open, "token market does not open");

    uint64_t eos_contract = market.quote.contract;
    uint64_t eos_symbol = market.quote.balance.symbol;

    asset eos_out{ 0, eos_symbol };
    _market.modify(*itr, 0, [&](auto& es) {
        eos_out = es.convert(quant, eos_symbol);
    });

    eosio_assert(eos_out.amount > 0, "token amount received from selling EOS is too low");

    int64_t fee_amount = eos_out.amount * FEE_RATE;
    if (fee_amount <= 0) {
        fee_amount = 1;
    }
    eos_out.amount = eos_out.amount - fee_amount;

    action(permission_level{ _self, N(active) },
        eos_contract, N(transfer),
        std::make_tuple(_self, receiver, eos_out, std::string("receive EOS from bancor")))
        .send();

    if (fee_amount > 0) {
        add_fee(fee_amount, eos_contract, eos_symbol);
    }
}

void exchange::add_fee(uint64_t amount, uint64_t token_contract, uint64_t token_symbol)
{
    fees _fee(_self, _self);
    uint128_t token_id = (uint128_t(token_contract) << 64) | token_symbol;
    auto idx = _fee.template get_index<N(token_id)>();
    auto itr = idx.find(token_id);
    if (itr == idx.end()) {
        auto pk = _fee.available_primary_key();
        _fee.emplace(_self, [&](auto& fee) {
            fee.id = pk;
            fee.token_contract = token_contract;
            fee.token_symbol = token_symbol;
            fee.amount = amount;
        });
    } else {
        _fee.modify(*itr, 0, [&](auto& fee) {
            fee.amount = fee.amount + amount;
        });
    }
}

void exchange::take_fee(account_name account, int limit)
{
    eosio_assert(limit > 0, "limit invalid");
    fees _fee(_self, _self);
    int count = 0;
    for (auto itr = _fee.begin(); itr != _fee.end() && count < limit; itr++) {
        if (itr->amount > 0) {
            action(
                permission_level{ _self, N(active) },
                itr->token_contract, N(transfer),
                std::make_tuple(_self, account, asset{ int64_t(itr->amount), itr->token_symbol }, std::string("receive token from exchange")))
                .send();

            _fee.modify(*itr, 0, [&](auto& fee) {
                fee.amount = 0;
            });
            count++;
        }
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

void exchange::on(const currency::transfer& t, account_name code)
{
    require_auth(t.from);

    if (t.from == _self || t.to != _self) {
        return;
    }

    eosio_assert(t.quantity.is_valid(), "invalid quantity");

    account_name admin = string_to_name(ADMIN);
    if (t.from == admin) {
        memo_param param;
        parse_memo_param(t.memo, param);

        if (param.opt == "clear_db") {
            clear_db();
            return;
        }

        if (param.opt == OPT_CREATE_MARKET) {
            asset eos_quant(param.eos_amount, EOS_SYMBOL);
            asset token_quant(param.token_amount, t.quantity.symbol);
            create_market(param.market_name, eos_quant, code, token_quant);
        } else if (param.opt == OPT_OPEN_MARKET) {
            open_market(code, t.quantity.symbol, true);
        } else if (param.opt == OPT_CLOSE_MARKET) {
            open_market(code, t.quantity.symbol, false);
        } else if (param.opt == OPT_TAKE_FEE) {
            take_fee(admin, param.limit);
        } else if (param.opt == OPT_UPDATE_MARKET) {
            asset eos_quant(param.eos_amount, EOS_SYMBOL);
            asset token_quant(param.token_amount, t.quantity.symbol);
            update_market(eos_quant, code, token_quant);
        } else {
            // eosio_assert(1, "invalid opt");
            return;
        }
    } else {
        if (code == EOS_CONTRACT && t.quantity.symbol == EOS_SYMBOL) {
            buy_token(t.from, t.quantity, t.memo);
        } else {
            sell_token(t.from, code, t.quantity);
        }
    }
}

void exchange::apply(account_name code, account_name action)
{
    if (action == N(transfer)) {
        on(unpack_action_data<currency::transfer>(), code);
        return;
    }
}
} // namespace dex

extern "C" {
[[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    dex::exchange ex(receiver);
    ex.apply(code, action);
    eosio_exit(0);
}
}
