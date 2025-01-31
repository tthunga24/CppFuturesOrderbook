# C++ FIX Protocol Futures Orderbook and Visual Depth of Market (DOM)
This is an futures orderbook programmed in C++ which allows users to submit orders using the FIX Communication Protocol and visualize 
the current state of the orderbook. The FIX Communication Protocol is an industry standard amongst exchanges that allows for seamless
and confusion-free communication of orders between brokers and exchanges. The orderbook represents a futures market, thus only allowing 
for order prices to be in tick increments (0.25 bps). This is an interactive program which allows for users to auto-populate the orderbook, 
create and submit their own orders, and print a Visual DOM to the terminal. 

![image](https://github.com/user-attachments/assets/625a86ca-eb08-4639-b31c-12c718f5abaa)

### Features
- **Submit and Parse FIX Protocol Messages:** Using the examples provided in examples.txt, learn how FIX Protocol messages are structured,
  create your own messages, and submit them to the orderbook to be parsed and turned into orders.
  
- **Visual DOM with Volume Profile:** Use the Depth of Market (DOM) to get a deeper insight into the market represented by the book. See resting limit
  orders at each level and use the volume profile to see where most trades have occured in the past.
  
- **Different Order Types:** Select between Fill or Kill orders and Good Until Fill orders and see how each order type interacts with the market and orderbook.
  More order types to be implemented in the future.

### Technologies
All technologies used come from the C++ Standard Library. In developing this project, I was able to explore and understand many different C++ data types and
mechanics including ordered and unordered maps, structs, classes, lists, iterators, vectors, references, shared pointers, and much more. 
