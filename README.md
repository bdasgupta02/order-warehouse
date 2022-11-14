# Persistent storage engine concept for an order-book data warehouse

This is a core storage engine for a medium-large scale data warehouse for time-series order-book data for financial trading. 
Built to support efficient retrieval (for instance, for research processes) and large-scale data storage, 
this engine is designed to provide order-book snapshot data at a queried time relatively fast.
Although optimized for fast temporally-linear file ingestions, insertions and fast queries, this engine also supports updates and deletions to data.
This project also has minimal dependencies, and relies solely on the C++ standard library.

## Motivation
The project was interesting in the way that it transcends tracking a value of an item - like normal time-series - rather requiring the knowledge of past events at any given point of time. 
This is because order-book data at any queried time needs to contain the state of all orders before that period, since different quantity-price pairs available previously will be available into the future unless they are traded or cancelled.
This makes it a very fun problem to think about - especially given the fact that querying all historical orders before a time to derive an order-book is simply not practical.
<br /><br /> So at the core of this project lies the tricky balance between fast insertions and fast queries.

## Contents
- [Motivation](#motivation)
- [Tech stack](#tech-stack)  
- [Quick start](#quick-start)
- [Requirements](#requirements)
- [Storage format](#storage-format)
- [Functionality](#functionality)
- [Optimisations](#optimisations)
- [Testing](#testing)
- [Limitations](#limitations)
- [Future improvements](#future-improvements)
- [A radical multi-node idea](#a-radical-multi-node-idea)
   
## Tech stack
C++ 20

## Quick start
Although this project mostly provides the underlying conceptual logic and implementation for the storage engine, it includes an easy-to-use interactive shell for quick experimenting.
Automated tests however provide a more in-depth view into how it can be used in a full-fledged application.
<br /><br /> Do note that both options below should be done from the root directory.

### Running interactive shell
The interactive shell supports insertions of singular orders, updates, deletions and queries. It also has its own query language (CQL), which is explained by using the `/help` command. Just compile the `shell.cpp` file, as well as the following implementations in C++ 20.
```
> g++ -std=c++2a -I . shell/shell.cpp src/order_book.cpp src/shared.cpp src/p_query.cpp src/p_insert.cpp src/p_delete.cpp src/avl_tree.cpp src/indexer.cpp src/p_update.cpp -o shell
> ./shell
```
#### CQL (Cool Query Language)
The shell has a basic command parser to interact with the shell using text commands, which is cool, of course. The commands are as follows:
```
[get order book at epoch]
        SELECT <symbol> AT <epoch>

[order books at multiple epochs]
        SELECT MULTIPLE <symbol> AT <epoch1> <epoch2> .. <epoch n>

[insert one order into database - use engine directly for file ingestions]
        INSERT <symbol> AT <epoch> VALUES <id> <side:BUY/SELL> <category:NEW/TRADE/CANCEL> <price> <quantity>

[delete order by epoch-id pair for a symbol]
        DELETE <symbol> WITH <epoch> <id>

[update order by epoch-id pair for a symbol with other values]
        UPDATE <symbol> WITH <epoch> <id> VALUES <side:BUY/SELL> <category:NEW/TRADE/CANCEL> <price> <quantity>
```

### Running automated tests
Just compile the `run_tests.cpp` file, as well as the following implementations in C++ 20.
```
> g++ -std=c++2a -I . tests/run_tests.cpp src/order_book.cpp src/shared.cpp src/p_query.cpp src/p_insert.cpp src/p_delete.cpp src/avl_tree.cpp src/indexer.cpp src/p_update.cpp -o test
> ./test
```

<br /> cmake will be soon added to ease these process.

## Requirements
### Functional
- Insert singular orders
- Ingest files with multiple orders 
- Query order-book snapshot based on time
- Updating any order
- Deleting any order

### Nonfunctional
- The potentially large amounts of data would need to scale horizontally through partitioning/sharding
- The data could benefit from being portable, as it would improve its accessibility and mobility
- Ingestions of files and order insertions should be fast, as this would hypothetically be a read-heavy storage system.
- The underlying data should be tolerant to faults, as a minor corruption somewhere should not compromize an organization's entire repository of order-book data
- Queries should be fast as well, especially since the engine shouldn't manually query all historic state before a queried time to generate a snapshot of the order-book
- Low memory/CPU overhead, especially for file ingestions which can take significant overhead for buffers etc.
- Must provide abstraction for the end-developer, as they should see the warehouse as one big bucket to store and pull data from - instead of knowing the underlying complexities

### Beyond time-series
Instead of knowing the specific price at one given point in time, queries should include a snapshot of all available orders in the order-book before it.
Without this requirement, one can just store all data in a simple tabular format to track orders.
The system therefore needs to optimize to make this less painful (more on this later).

### Key assumptions
- Historic edits, deletions and insertions in middle of the data would be unlikely, as data from files and trading systems would most likely arrive linearly forward with respect to time
- Write-heavy, but reads should be fast for data-analysis needs, given the potential scale of data
- Although this is more of a concept, only one instance of this project will be active at any given point
- All data for file ingestions will be cleaned and sorted according to epoch prior to ingestion, and will be text-based

### Supported order types (for both buy and sell sides)
- **NEW:** A new order entry that is available to trade
- **TRADE:** An order which trades any matching NEW orders before its epoch, and thus removing the quantity of the trading price from the order-book. Note that this would be on the same side, as a BUY trade would trade away previous NEW BUY orders
- **CANCEL:** Removes a quantity from a price, from the order-book

## Storage format
As the freshness of ingredients are the key to a good dish, the design of the underlying data format sits at the core of how the system optimises for its functional and nonfunctional needs. 

### Dual-partiotioning
![Partitioning](https://lh6.googleusercontent.com/5hkeK0shyLQDbklk5QyDL1yQ5bZ9w33wEVjwx4zmdK6XvSfzcbNVusarZbHFeDW_nEg=w2400)
- To ensure fault-tolerance, scalability, replicability, and portability, the underlying data strcture is designed to be partitioned very easily - and with minimal dependency between the partitions
- The data is therefore partitioned both by the ticker symbol of a financial product, as well as small time window (in terms of nanosecond-based epoch from 1 January 1970 00:00:00)
- As detailed in the next section, only a single partitioned chunk is needed to find the data for any time within the epoch window
- This sort of dependency segregation for queries implies that **larger amount of files do not slow down queries**, which is neat

### File structure
![Chunk structure](https://lh5.googleusercontent.com/VxwSVP0Tmi7mBTG-HTxq74x3vOCaDfP1xWIhtFWLGz05PXXL92EOOC8LvbYMcNwpyQ0=w2400)
- Each file stores the order data for a given epoch window (default at 10 minute-windows, but can be adjusted to any number). Each file will be named by the epoch at which the window starts, within a folder named by the symbol ticker name. For instance:
```
storage/
  TWTR/     <-- Symbol 
    100.dat <-- Chunk file for an epoch window
    200.dat
    ...
    IDX.dat <--- AVL Tree index for epoch windows
  META/
    100.dat
    200.dat
    ...
    IDX.dat
```
- The underlying file structure would have 3 primary parts:
  - **Header:** stores the key details with regards to the sizes of the other two sections, and also the last trade details (for key statistics)
  - **Base state:** stores the aggregated order-book for all history before this epoch window
  - **Orders:** stores the fine-grained individual order details within the file's epoch window (without aggregation)
- The file structure serves as a middle ground between fast queries and fast insertions
- This is possible through aggregating order-book data, to make storing base data for chunk files efficient 

### Trade-offs
- One extreme would be to store each order separately, which would result in a complicated, intensive and slow query to get a snapshot of past orders at a given epoch - as we have to painfully calculate from the first epoch to a given epoch to generate a snapshot for each query
- Another extreme would be to store the snapshot of all orders before an epoch, for each epoch - which consumes a lot of space due to massive redundancies
- The design for the data chunk system is therefore a hybrid of these two extremes

### Binary format
- A binary format (with .dat files) will be used to store all data
- Using a binary format saves space as most of the data is numerical (as opposed to string-based formats)
- Using a binary format also decreases overheads for type casting/conversions
- All data stored inside the `.dat` files would be numerical

## Functionality
### Insertions
- Insertions can be made individually for each order, or by file ingestion
- As for file ingestions, as stated in the assumptions the data needs to be cleaned and sorted according to epoch, and follow the following format:
```
epoch  |  id  |  symbol  |  side(BUY/SELL)  |  category(NEW/TRADE/CANCEL)  |  price  |  quantity
```
- For examples of this file format, please check the `.log` files within `tests/test-ingest`. This format is also how the engine uses to store data inside the chunk files for each individual order
- Ingestions are well-optimized if the orders are being appended on top of temporally previous orders, without any orders already stored for the future

### Queries
- Supports singular and multiple epoch queries, with custom specified fields if needed (as the prompt requested)
- The best part about the partitioning system is ensuring fast queries regardless of how many files there are

### Updates
- Although not optimised for updates due to the identified characteristics, updates are still supported at a relatively slower speed
- The speed of updates to one particular file depends on how many files are temporally ordered after it, as the change needs to be permeated through future chunks

### Deletions
- Deletions can be made with an epoch-id pair for an order
- Works very similar to updates, and is therefore relatively sluggish if very old orders are deleted

## Optimisations
### Indexing chunk time windows using AVL trees
- In this system, an AVL tree is used to index **chunk file epochs** according to epoch windows for fast searches, insertions and deletions
- AVL trees will be loaded onto memory when the server begins, and serialized then flushed to the disk on updates concurrently to save time
- All index lookups/searches/manipulation would be done on-memory, to make it very fast (as opposed to reading from disk everytime)
- Instead of every update, an alternative would be to have indexes be flushed to the disk periodically
- Red-black trees were another option to slightly improve writes, but I choose AVL trees due to personal expertise, and speed up reads slightly

### Balancing trade-offs
There is a choice between optimizing completely for read-speeds, at the cost of update, delete, and possibly insertion speeds.
The epoch window size for chunk files can be decreased to make queries blazingly fast, as after finding the correct file, there is less calculation to be done within that file if there are less order entries within it.
But narrowing epoch windows would undermine speeds of updates and deletes, as they need to manipulate even more files (for future epoch windows) to change their base state.
Another implication of decreasing chunk epoch windows would be a potential slowdown of insertions at **the middle** of stored data from a temporal perspective, as updates need to be permeated towards future windows (for base state).
However, if all new inserted data is appended on top of old data, insertions will not slow down due to propagation of base state data.

### Epoch window trade-offs
- The epoch window size can be adjusted based on project or company needs
- Smaller epoch windows mean faster reads at the cost of expensive insertions/updates/deletions in middle files and vice versa

### Concurrency
- Fine-grained mutex for insertions, updates and deletions for each symbol, as well as flushing indices to disk - to improve atomicity of operations
- Index writes to disk are done parallel to normal processes to save time

## Testing
Automated tests are written for all major components of the project, albeit through vanilla C++. The automated testing in the quick start section details how to run the tests.

## Limitations
- Historic insertions, updates and deletions are slow if they are before already entered future orders
- Race conditions apply for different processes/instances of this application (especially bad news for the precious indexer system)
- Prices are in type `double`, since this is a concept of an idea
- Saving aggregated base state to every chunk file might have a size issue when there are a lot of orders with different prices (as each would be a new entry on the base staet tables). This would take more disk space, and also slow down queries
- I need more knowledge about how something like this would be used more closely

## Future improvements
- Query for multiple timestamps at once can be made more efficient through a one-pass disk access, instead of epoch by epoch
- Perhaps a more generalised time-series database with aggregation support could be explored with LSM trees to optimize for writes (currently exploring the LSM process etc.)
- Flushing of the AVL tree indices to the disk can be done periodically, rather than on every addition, to save some overhead

## A radical multi-node idea
Perhaps for the future, this underlying engine can be adapted to fit a multi-node distributed system, where different nodes/servers get their own overarching window partitions.
Taking this to a further extreme, each node could also have its own base state of epochs before it, which could periodically be permeated to all chunks within the node.
The possibilities seem limitless.

