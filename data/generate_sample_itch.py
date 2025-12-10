#!/usr/bin/env python3
import struct
import random

def generate_add_order(timestamp, order_ref, shares, stock, price, buy_sell):
    msg_type = b'A'
    stock_locate = struct.pack('>H', 1)
    tracking_num = struct.pack('>H', 0)
    ts_bytes = struct.pack('>Q', timestamp)[-6:]
    order_ref_bytes = struct.pack('>Q', order_ref)
    buy_sell_byte = buy_sell.encode('ascii')
    shares_bytes = struct.pack('>I', shares)
    stock_bytes = stock.ljust(8).encode('ascii')[:8]
    price_bytes = struct.pack('>I', price)
    
    return msg_type + stock_locate + tracking_num + ts_bytes + \
           order_ref_bytes + buy_sell_byte + shares_bytes + \
           stock_bytes + price_bytes

def main():
    stocks = ['AAPL', 'MSFT', 'GOOGL', 'TSLA', 'AMZN']
    
    with open('data/sample_itch.bin', 'wb') as f:
        for i in range(50000):
            stock = random.choice(stocks)
            timestamp = 34200000000000 + i * 1000
            order_ref = 1000000 + i
            shares = random.randint(100, 10000)
            price = random.randint(1000000, 2000000)
            buy_sell = random.choice(['B', 'S'])
            
            msg = generate_add_order(timestamp, order_ref, shares, 
                                     stock, price, buy_sell)
            f.write(msg)
    
    print(f"Generated 50,000 ITCH messages")

if __name__ == '__main__':
    main()
