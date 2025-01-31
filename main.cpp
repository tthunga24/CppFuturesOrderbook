#include <cmath>
#include <map>
#include <list>
#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <iterator>
#include <cstdint>
#include <stdexcept>
#include <memory>
#include <iomanip>

using int32 = std::int32_t;
using uint32 = std::uint32_t;

enum class Side{
    Buy,
    Sell
};

enum class OrderType{
    GoodTillFill,
    FillOrKill
};

struct PriceLevel{
    float price;
    uint32 quantity;
};

class Order{
private:
    OrderType orderType_;
    Side side_;
    uint32 orderId_;
    float price_;
    uint32 quantity_;
    uint32 remaining_;
    bool counted_ = false;


public:
    Order(OrderType orderType, Side side, uint32 orderId, float price, uint32 quantity):
        orderType_ (orderType),
        side_ (side),
        orderId_ (orderId),
        price_ (price),
        quantity_ (quantity),
        remaining_ (quantity)
        
        {
            if(fmod(price_, 0.25) != 0){
                throw std::logic_error("Price is not an increment of a tick (0.25)");
            }
        }

        OrderType getOrderType() const { return orderType_; }
        Side getSide() const {return side_; }
        uint32 getOrderId() const { return orderId_; }
        float getPrice() const { return price_; }
        uint32 getQuantity() const {return quantity_;}
        uint32 getRemaining() const { return remaining_; }
        bool getCounted() const {return counted_; }
        void toggleCounted() { counted_ = true; }

        void fillOrder(uint32 quantityv){
            if (quantityv <= getRemaining()){
                remaining_ -= quantityv;
            }else {
                std::cout << "Error fill quantity more than remaining" << std::endl;
            }
        }
};



using priceLevels = std::vector<PriceLevel>;

class BookLevels{
private:
    priceLevels bids_;
    priceLevels asks_;
    
public:
    BookLevels(const priceLevels& bids, const priceLevels& asks):
        bids_ (bids),
        asks_(asks)
        {}
        
        const priceLevels& getBids() const{
            return bids_;
        }
        
        const priceLevels& getAsks() const{
            return asks_;
        }

};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

struct TradeInfo {
    uint32 orderId_;
    float price_;
    uint32 quantity_;

};

class Trade{
private:
    TradeInfo bidSide_;
    TradeInfo askSide_;

public:
    Trade(const TradeInfo& bidSide, const TradeInfo& askSide):
        bidSide_ (bidSide),
        askSide_ (askSide)
        {}

        const TradeInfo& getBidSide() const { return bidSide_; }
        const TradeInfo& getAskSide() const { return askSide_; }
};



class Orderbook{

private:

    struct OrderEntry{
        OrderPointer order_{nullptr};
        OrderPointers::iterator location_;


    };

    struct LevelStat{
        
        uint32 volume = 0;
        uint32 openBids = 0;
        uint32 openAsks = 0;
    };

    std::map<float, OrderPointers, std::greater<float>> bids_;
    std::map<float, OrderPointers, std::less<float>> asks_;
    std::unordered_map<uint32, OrderEntry> orders_;
    std::unordered_map<float, LevelStat> levelStats_;
    std::vector<Trade> trades_;
    uint32 seqIdx = 0;
    bool newestIsBuy = false;
    bool populated_ = false;
    bool canFill(Side orderSide, float price) const{
        if (orderSide == Side::Buy ){
            if (asks_.empty()){
                return false;
            }else{
                    const auto& [bestAsk, temp] = *asks_.begin();
                    return price >= bestAsk;


            }

        }else {
            if(bids_.empty()){
                return false;
            }else{
                    const auto& [bestBid, temp] = *bids_.begin();
                    return price <= bestBid; 
            }
        }
    }

    void Fill() {
        

        while (true){

            
            auto& [bidPrice, bids] = *bids_.begin();
            auto& [askPrice, asks] = *asks_.begin();
            if((bidPrice < askPrice) || (bidPrice == 0 || askPrice == 0)){
                break;
            }
            while(!(bids.empty()) && !(asks.empty())){
                
                
                OrderPointer& bid = bids.front();
                OrderPointer& ask = asks.front();
                uint32 bOrdId = bid -> getOrderId();
                uint32 aOrdId = ask -> getOrderId();
                std::string bMsg = " partially filled @ ";
                std::string aMsg = " partially filled @ ";
                float ordPrice = bidPrice;

                if(newestIsBuy){
                    ordPrice = askPrice;
                }
                uint32 minquantity = std::min(bid -> getRemaining(), ask -> getRemaining());
                
                bid -> fillOrder(minquantity);
                ask -> fillOrder(minquantity);
                
                trades_.push_back(Trade{
                    TradeInfo{ bOrdId, ordPrice, minquantity},
                    
                    TradeInfo{aOrdId, ordPrice, minquantity}
                
                }); 

                UpdateLevelStats();
                
                if (bid -> getRemaining() == 0 || bid -> getOrderType() == OrderType::FillOrKill){
                    
                    bMsg = " fully filled @ ";
                    if (bid -> getOrderType() == OrderType::FillOrKill && bid -> getRemaining() != 0){
                        bMsg = " partially filled and killed @ ";
                    }
                    bids.pop_front();
                    orders_.erase(bOrdId);
                   
                }


                
                if (ask -> getRemaining() == 0|| ask -> getOrderType() == OrderType::FillOrKill ){
                    
                    aMsg = " fully filled @ ";
                    if (ask -> getOrderType() == OrderType::FillOrKill && ask -> getRemaining() != 0){
                        aMsg = " partially filled and killed @ ";
                    }
                    asks.pop_front();
                    orders_.erase(aOrdId);
                    
                    }

                
                
                std::cout << "Buy Order# " << bOrdId << bMsg << ordPrice << " for " << minquantity << " units." << std::endl;
                std::cout << "Sell Order# " << aOrdId << aMsg << ordPrice << " for " << minquantity << " units." << std::endl;

                

            }     
              
            if (bids.empty()){
                bids_.erase(bidPrice);
            }
            if (asks.empty()){
                asks_.erase(askPrice);
            }
                
        }

        
    }

    void UpdateLevelStats(){
        
        for(int i = seqIdx; i < trades_.size(); i++){
            
            Trade& currTrade = trades_[i];
            const TradeInfo& bidSide = currTrade.getBidSide();
            const TradeInfo& askSide = currTrade.getAskSide();
            LevelStat& bidLevel = levelStats_[bidSide.price_];
            LevelStat& askLevel = levelStats_[askSide.price_];
            OrderPointer& bidOrd = orders_[bidSide.orderId_].order_;
            OrderPointer& askOrd = orders_[askSide.orderId_].order_;


            bidLevel.volume += bidSide.quantity_;
            
            
            if(bidOrd -> getOrderType() != OrderType::FillOrKill && bidOrd -> getRemaining() > 0 && !(bidOrd -> getCounted())){
                levelStats_[bidOrd -> getPrice()].openBids += bidOrd -> getRemaining();
                bidOrd -> toggleCounted();
            } else if(bidOrd -> getCounted()){
                levelStats_[bidOrd -> getPrice()].openBids -= bidSide.quantity_;
            }
            if(askOrd -> getOrderType() != OrderType::FillOrKill && askOrd -> getRemaining() > 0 && !(askOrd -> getCounted())){
                levelStats_[askOrd -> getPrice()].openAsks += askOrd -> getRemaining();
                askOrd -> toggleCounted();
            } else if(askOrd -> getCounted()){
                levelStats_[askOrd -> getPrice()].openAsks -= askSide.quantity_;
            }
            


        }

        seqIdx = trades_.size();
    }

public:

    void AddOrder(OrderPointer ordp){
        
        std::cout << "Order# " << ordp -> getOrderId() << " confirmed." << std::endl;
        if (orders_.find(ordp -> getOrderId()) != orders_.end()){
            std::cout << ordp -> getOrderId() << std::endl;
            throw std::logic_error("Order with this order number already exists.");
        }


        if (ordp -> getOrderType() == OrderType::FillOrKill && !(canFill(ordp -> getSide(), ordp -> getPrice())) ){
            std::cout << "FillorKill Order# " << ordp -> getOrderId() << " could not be filled so it was cancelled." << std::endl;
            return;
        }

        OrderPointers::iterator iter;

        if(ordp -> getSide() == Side::Buy){
            auto& bidlvl = bids_[ordp -> getPrice()];
            bidlvl.push_back(ordp);
            iter = std::next(bidlvl.begin(), bidlvl.size() - 1);
            newestIsBuy = true;

        }else{
            auto& asklvl = asks_[ordp -> getPrice()];
            asklvl.push_back(ordp);
            iter = std::next(asklvl.begin(), asklvl.size()-1);
            newestIsBuy = false;
        }

        orders_.insert({ ordp -> getOrderId(), OrderEntry{ordp, iter}});
        if(canFill(ordp -> getSide(), ordp -> getPrice())){
             
            Fill();
        }else{
            if(ordp -> getSide() == Side::Buy){
                levelStats_[ordp ->getPrice()].openBids += ordp -> getQuantity();
                ordp -> toggleCounted();
            }else {
                levelStats_[ordp -> getPrice()].openAsks += ordp -> getQuantity();
                ordp -> toggleCounted();
            }
        }
        
        

    }

    void CancelOrder(uint32 ordId){
        if (orders_.find(ordId) == orders_.end()){
            return;
        }

        const auto& [order, iter] = orders_.at(ordId);
        orders_.erase(ordId);

        if (order -> getSide() == Side::Buy){
            auto& orderslvl = bids_.at(order -> getPrice());
            orderslvl.erase(iter);
            if(orderslvl.empty()){
                bids_.erase(order -> getPrice());
            }
        }else {
            auto& orderslvl = asks_.at(order -> getPrice());
            orderslvl.erase(iter);
            if(orderslvl.empty()){
                asks_.erase(order -> getPrice());
            }
        }

    }

    void PrintDom(){
        if(bids_.empty() || asks_.empty()){
            std::cout << "Not enough orders to print a DOM" << std::endl;
            return;
        }
        auto& [bestBid, _]  = *bids_.begin();
        auto& [bestAsk, __] = *asks_.begin();
        auto& [highestPrice, ___] = *std::prev(asks_.end());
        auto& [lowestPrice, ____] = *std::prev(bids_.end()); 
        float mktPrice = bestBid + (std::round((bestAsk - bestBid))/2.0);
        std::vector<float> priceLadder;
        uint32 largerstVol = 0;
        
        for(float i = highestPrice; i >= lowestPrice; i -= 0.25){
            priceLadder.push_back(i);
        }

       

        for (auto& [_, priceLvl] : levelStats_){
            if(priceLvl.volume > largerstVol){
                largerstVol = priceLvl.volume;
            }
        }


        std::cout << std::setw(10) << std::left << " " << std::setw(20) << std::left << "Volume";
        std::cout << std::setw(15) << std::left << "Price";
        std::cout << std::setw(15) << std::left << "Bids" << "Asks" << std::endl;

        for( float i: priceLadder){
            if (i == mktPrice){
                std::cout << "\033[1;33m" << std::setw(22) << "" << std::left << "========" << mktPrice << "========" << "\033[0m" << std::endl;
            }
            LevelStat& priceLvl = levelStats_[i];
            uint32 volume = priceLvl.volume;
            uint32 barsize = 0;
            

            if (volume != 0){
                barsize = std::round((static_cast<float>(volume) / largerstVol) * 25);
            }

            std::cout << std::setw(5) << std::left << priceLvl.volume << std::setw(25 - barsize) << "";
            for(int i = 0; i < barsize; i++){
                std::cout << "\033[1;34m" << "\u2588" << "\033[0m";
            }
            std::cout << std::setw(15) << std::left << i;
            std::cout << "\033[1;32m" << std::setw(15) << std::left;
            if(priceLvl.openBids){
                std::cout << priceLvl.openBids << "\033[0m";
            }else{
                std::cout << "" << "\033[0m";
            }
            if(priceLvl.openAsks){
                std::cout << "\033[1;31m" << priceLvl.openAsks << "\033[0m";
            }
            std::cout << std::endl;


        }

    }

    void ParseMessage(std::string msg){

        std::vector<std::string> seperated;
        std::string temp;
        std::stringstream ss(msg);
        std::string f35;
        uint32 f11;
        uint16_t f21;
        uint16_t f54;
        uint32  f38;
        float f44;
        OrderType ordT;
        Side ordS;

        while (std::getline(ss, temp, '|')){
            seperated.push_back(temp);
        }

        if(seperated.size() != 16){
            std::cout << "Not a valid FIX order" << std::endl;
            return;
        }

        f35 = seperated[2].substr(3);
        f11 = std::stoi(seperated[7].substr(3));
        f21 =std::stoi(seperated[8].substr(3));
        f54 = std::stoi(seperated[10].substr(3));
        f38 = std::stoi(seperated[11].substr(3));
        f44 = std::stof(seperated[13].substr(3));
        std::cout << f21 << std::endl;
        if(f35 != "D"){
            std::cout << "This orderbook only accepts single order messages. (35)" << std::endl;
            return;
        }

        if(f21 == 1){
            ordT = OrderType::FillOrKill;
        } else if(f21 == 2){
            ordT = OrderType::GoodTillFill;
        }else{
            std::cout << "Invalid order type. (21)" << std::endl;
            return;
        }

        if(f54 == 1){
            ordS = Side::Buy;
        }else if(f54 == 2){
            ordS = Side::Sell;
        }else{
            std::cout << "Invalid order side. (54)" << std::endl;
        }

        AddOrder(std::make_shared<Order>(ordT, ordS, f11, f44, f38));

    }

    void populateOrderBook(){
        if(populated_){
            std::cout << "Ourderbook has already been populated" << std::endl;
            return;
        }
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  10000,  98.00, 30));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 20000,  98.00, 30)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  30000,  98.25, 20));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 40000,  98.25, 20)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  50000,  98.50, 40));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 60000,  98.50, 40)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  70000,  98.75, 35));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 80000,  98.75, 35)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  90000,  99.00, 50));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 100000, 99.00, 50)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  110000, 99.25, 25));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 120000, 99.25, 25)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  130000, 99.50, 45));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 140000, 99.50, 45)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  150000, 99.75, 55));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 160000, 99.75, 55)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  170000, 100.00, 60));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 180000, 100.00, 60)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  190000, 100.25, 50));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 200000, 100.25, 50)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  210000, 100.50, 40));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 220000, 100.50, 40)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  203000, 100.75, 35));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 240000, 100.75, 35)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  250000, 101.00, 30));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 260000, 101.00, 30)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  270000, 101.25, 25));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 280000, 101.25, 25)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  290000, 101.50, 20));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 300000, 101.50, 20)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  310000, 101.75, 15));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 320000, 101.75, 15)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  330000, 102.00, 10));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 340000, 102.00, 10)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  350000, 102.25, 5));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  10001,  99,  50));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  20001,  99.25, 40));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  30001,  99.50, 60));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  40001,  99.75, 70));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  50001,  100.00, 10));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  60001,  100.00, 5));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  70001,  100.25, 30));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  80001,  100.50, 20));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  90001,  100.75, 10));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 100001, 100.50, 80)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 110001, 100.25, 90)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 120001, 100.00, 80)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 130001, 100.75, 10));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 140001, 101.00, 50));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 150001, 101.25, 40));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 160001, 101.50, 60));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 170001, 101.75, 70));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 180001, 102.00, 80));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  190001, 98.75, 20));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  200001, 98.50, 30));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  210001, 98.25, 40));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  220001, 98.00, 50));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 230001, 102.25, 40));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 240001, 102.50, 30));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 250001, 102.75, 20));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 260001, 103.00, 10));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  270001, 99.00, 35));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 280001, 101.75, 25));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  290001, 99.50, 45));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 300001, 100.75, 15));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  330001, 99.75, 25));
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Sell, 340001, 99.75, 25)); 
        AddOrder(std::make_shared<Order>(OrderType::GoodTillFill, Side::Buy,  350001, 99.25, 30));
        populated_ = true;
    }
};

int main()
{
    Orderbook orderbook;
    while(true){
    std::string input;
    
    std::cout << "C++ Orderbook and Visual DOM" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "1. Submit Order" << std::endl;
    std::cout << "2. Print Visual DOM" << std::endl;
    std::cout << "3. Populate Orderbook" << std::endl;
    std::getline(std::cin, input);

    if(input == "1"){
        std::string order;
        std::cout << "Enter your order" << std::endl; 
        std::getline(std::cin, order);
        orderbook.ParseMessage(order);
    }else if(input == "2"){
        orderbook.PrintDom();
    }else if(input == "3"){
        orderbook.populateOrderBook();
    }else{
        std::cout << "Invalid Input" << std::endl;
        
    }
    
    }
    return 0;
}
