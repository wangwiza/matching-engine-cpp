#include <string>

class Order {
public:
	uint32_t order_id;
	int price;
	int count;
	std::string instrument;
	bool is_sell;
	
    Order(uint32_t order_id, int price, int count, std::string instrument, bool is_sell) : order_id(order_id), price(price), count(count), instrument(instrument), is_sell(is_sell) {}
};