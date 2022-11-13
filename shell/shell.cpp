#include "include/order.hpp"
#include "include/p_insert.hpp"
#include "include/p_delete.hpp"
#include "include/p_update.hpp"
#include "include/p_query.hpp"
#include "include/config.hpp"
#include "include/order_book.hpp"
#include "include/query_result.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <stdint.h>
#include <iomanip>

/**
 * Interactive shell to use the database engine easily and quickly.
 * Includes a simple string-based query processor.
 */

// Commands
static const std::string HELP = "/help";
static const std::string EXIT = "/exit";
static const std::string SELECT = "SELECT";
static const std::string MULTIPLE = "MULTIPLE";
static const std::string INSERT = "INSERT";
static const std::string DELETE = "DELETE";
static const std::string UPDATE = "UPDATE";

// Messages
static const std::string PROMPT = "\n>>> ";
static const std::string WELC = "\nWelcome to simple interactive shell! Please type /help for info.\n";
static const std::string ERR = "Could not interpret statement. Please type again.\n";
static const std::string EXT = "Exiting...\n\n";
static const std::string HELP_MSG = (std::string) "Help Menu\n" +
                                    "---------\n\n" +
                                    "IMPORTANT: Note that all data will be stored in the storage directory (in the workspace directory for now).\n" +
                                    "IMPORTANT: Ensure all types (especially numerical) are correctly entered in the statements.\n\n" +
                                    "List of commands:\n" +
                                    "[get order book at epoch]\n" +
                                    "\tSELECT <symbol> AT <epoch>\n\n" +
                                    "[order books at multiple epochs]\n" +
                                    "\tSELECT MULTIPLE <symbol> AT <epoch1>, <epoch2>, ..\n\n" +
                                    "[insert one order into database - use engine directly for file ingestions]\n" +
                                    "\tINSERT <symbol> AT <epoch> VALUES <id> <side:BUY/SELL> <category:NEW/TRADE/CANCEL> <price> <quantity>\n\n" +
                                    "[delete order by epoch-id pair for a symbol]\n" +
                                    "\tDELETE <symbol> AT <epoch> <id>\n\n" +
                                    "[update order by epoch-id pair for a symbol]\n" +
                                    "\tUPDATE <symbol> WITH <epoch> <id> VALUES <side:BUY/SELL> <category:NEW/TRADE/CANCEL> <price> <quantity>\n\n";

void process_update(std::vector<std::string> fields, PUpdate &updater)
{
    Side side = fields[6] == BUY_STR ? BUY : SELL;

    Category cat;
    if (fields[7] == CANCEL_STR)
        cat = CANCEL;
    else if (fields[7] == TRADE_STR)
        cat = TRADE;
    else
        cat = NEW;

    Order order(fields[1],
                std::stoull(fields[3]),
                std::stoull(fields[4]),
                side,
                cat,
                std::stoull(fields[9]),
                atof(fields[8].c_str()));

    if (updater.update_order(order))

        std::cout << "Update successful!\n";
    else
        std::cout << "Update failed!\n";
}

void process_delete(std::vector<std::string> fields, PDelete &deleter)
{
    std::string symbol = fields[1];
    std::string epoch_str = fields[3];
    uint64_t epoch = std::stoull(epoch_str);
    std::string id_str = fields[4];
    uint64_t id = std::stoull(id_str);

    if (deleter.delete_order(symbol, id, epoch))
        std::cout << "Deletion successful!\n";
    else
        std::cout << "Deletion failed!\n";
}

void process_insert(std::vector<std::string> fields, PInsert &inserter)
{
    Side side = fields[6] == BUY_STR ? BUY : SELL;

    Category cat;
    if (fields[7] == CANCEL_STR)
        cat = CANCEL;
    else if (fields[7] == TRADE_STR)
        cat = TRADE;
    else
        cat = NEW;

    Order order(fields[1],
                std::stoull(fields[3]),
                std::stoull(fields[5]),
                side,
                cat,
                std::stoull(fields[9]),
                atof(fields[8].c_str()));

    if (inserter.insert(order).second)
        std::cout << "Insertion successful!\n";
    else
        std::cout << "Insertion failed!\n";
}

template <typename T>
void pretty_print(T t, const int &width, char end)
{
    std::cout << std::left << std::setw(width) << std::setfill(' ') << t << end;
}

std::string get_two_precision(double val)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << val;
    return stream.str();
}

void prettify_query_res(QueryResult &res, uint64_t epoch)
{
    std::vector<OrderEntry> buys = res.book.buy_list();
    std::vector<OrderEntry> sells = res.book.sell_list();

    std::cout << "\n----------- Query results at epoch " << epoch << " -----------\n\n";
    pretty_print("Last traded epoch:", 25, ' ');
    pretty_print(res.last_trade_epoch, 25, '\n');
    pretty_print("Last traded quantity:", 25, ' ');
    pretty_print(res.last_trade_qty, 25, '\n');
    pretty_print("Last traded price:", 25, ' ');
    pretty_print(get_two_precision(res.last_trade_price), 25, '\n');

    std::cout << "\nBuy orders in order book:\n";
    std::cout << "-------------------------\n";
    pretty_print("Quantity", 15, ' ');
    pretty_print("Price", 20, '\n');
    for (OrderEntry &entry : buys)
    {
        pretty_print(entry.qty, 15, ' ');
        pretty_print(entry.price, 20, '\n');
    }

    std::cout << "\nSell orders in order book:\n";
    std::cout << "--------------------------\n";
    pretty_print("Quantity", 15, ' ');
    pretty_print("Price", 20, '\n');
    for (OrderEntry &entry : sells)
    {
        pretty_print(entry.qty, 15, ' ');
        pretty_print(entry.price, 20, '\n');
    }
}

void process_select_many(std::vector<std::string> fields, PQuery &querier)
{
    std::string symbol = fields[2];
    for (unsigned int i = 4; i < fields.size(); i++)
    {
        std::string epoch_str = fields[i];
        uint64_t epoch = std::stoull(epoch_str);
        QueryResult res = querier.query_timestamp(epoch, symbol);
        prettify_query_res(res, epoch);
    }
}

void process_select_one(std::vector<std::string> fields, PQuery &querier)
{
    std::string symbol = fields[1];
    std::string epoch_str = fields[3];
    uint64_t epoch = std::stoull(epoch_str);
    QueryResult res = querier.query_timestamp(epoch, symbol);
    prettify_query_res(res, epoch);
}

bool process_input(std::string input, PInsert &inserter, PDelete &deleter, PUpdate &updater, PQuery &querier)
{
    std::stringstream stream(input);
    std::istream_iterator<std::string> begin(stream);
    std::istream_iterator<std::string> end;
    std::vector<std::string> fields(begin, end);

    if (fields.size() < 1)
    {
        std::cout << ERR;
        return true;
    }

    if (fields.size() >= 5 && fields[0] == SELECT && fields[1] == MULTIPLE)
    {
        process_select_many(fields, querier);
    }
    else if (fields.size() >= 4 && fields[0] == SELECT)
    {
        process_select_one(fields, querier);
    }
    else if (fields.size() >= 10 && fields[0] == INSERT)
    {
        process_insert(fields, inserter);
    }
    else if (fields.size() >= 5 && fields[0] == DELETE)
    {
        process_delete(fields, deleter);
    }
    else if (fields.size() >= 10 && fields[0] == UPDATE)
    {
        process_update(fields, updater);
    }
    else if (fields[0] == HELP)
    {
        std::cout << HELP_MSG;
    }
    else if (fields[0] == EXIT)
    {
        std::cout << EXT;
        return false;
    }
    else
    {
        std::cout << ERR;
    }

    return true;
}

void run_shell()
{
    Config *conf = new Config("storage/");
    PInsert inserter(conf);
    PDelete deleter(conf);
    PUpdate updater(conf);
    PQuery querier(conf);

    bool is_running = true;
    while (is_running)
    {
        std::cout << PROMPT;
        std::string input;
        std::getline(std::cin, input);
        is_running = process_input(input, inserter, deleter, updater, querier);
    }
}

int main()
{
    std::cout << WELC;
    run_shell();
    return 0;
}