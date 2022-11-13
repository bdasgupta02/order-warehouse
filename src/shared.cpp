#include "shared.hpp"
#include "include/config.hpp"
#include <type_traits>
#include <utility>
#include <vector>
#include <string>

void remove_string_end(int times, std::string &source)
{
	for (int i = 0; i < times; i++)
		source.pop_back();
}

uint64_t generate_epoch_window(Config *conf, uint64_t epoch)
{
	return (epoch / conf->epoch_window) * conf->epoch_window;
}

std::pair<std::string, uint64_t> generate_filename(Config *conf, uint64_t epoch, std::string symbol)
{
	uint64_t timestamp_from = generate_epoch_window(conf, epoch);
	std::string filename = conf->data_dir;
	filename.append(symbol + "/");
	filename.append(std::to_string(timestamp_from));
	filename.append(".dat");
	return {filename, timestamp_from};
}

void write_header(std::ofstream &fout, Header &header)
{
	fout.write((char *)&header, sizeof(Header));
}

void write_base_book(std::ofstream &fout, OrderBook &book)
{
	std::vector<OrderEntry> buys = book.buy_list();
	std::vector<OrderEntry> sells = book.sell_list();

	for (OrderEntry &entry : buys)
	{
		fout.write((char *)&entry, sizeof(OrderEntry));
	}

	for (OrderEntry &entry : sells)
	{
		fout.write((char *)&entry, sizeof(OrderEntry));
	}
}