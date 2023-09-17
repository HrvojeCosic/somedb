# Overview
This is a simple database management system currently containing the following components and functionalities:
### Disk Persistence
Each table is stored in its own file on disk along with the metadata pages needed to keep track of pages it contains, as well as the table schema metadata used for validation.Pages stored in the table follow the slotted page layout scheme, which means that for each page stored in the table, there is a list of tuple pointers growing from the beginning of the page towards the end, whereas the tuples themselves are stored starting from the end of the page towards the beginning. Metadata needed to keep track of this layout scheme is stored in the header of each page. More detailed information about the storage formats can be found in `include/disk/heapfile.h`

Pages are brought from disk into memory using the buffer pool manager defined in `include/disk/bpm.h`. It keeps frequently used pages in memory, and evicts the less recently used ones from memory using the clock replacement algorithm. The simple API of the clock replacer can be found in `include/disk/clock_replacer.h`

### Indexing
Indexing is achieved using a B+tree data structure, stored on disk in a separate file of the format described in `include/index/btree_index.hpp`. The nodes (pages) of the B+tree are stored in the format described in `include/index/index_page.hpp`. Each node is implemented with a pointer to the next sibling node to enable efficient range scans, as well as the separate rightmost pointer (in the case of internal nodes) to account for the extra child pointer compared to the number of keys it holds. The tree itself also keeps track of traversed nodes on the path to the target leaf node to optimize the possible propagating splits during inserts/merges during deletes.

### SQL Support
When a query enters the system, it gets split up into tokens by the lexer, whose API, as well as supported tokens and keywords, can be found in `include/sql/lexer.hpp`. The query gets parsed using the implementation of a [Pratt parser](https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/), which produces some of the few currently supported SQL expressions that can be found in `include/sql/sql_expression.hpp`. The next step is creating a logical plan for the query. The logical relational algebra operators currently supported can be found in `include/sql/logical_plan.hpp`. The operators in the logical plan are connected in a tree-like structure, and result in a table schema modified in accordance to the operators it contains.

# Setup
Since SQL support is under development, the REPL for user interaction with system is not yet implemented, but the tests for all the components can be ran with the `make test` command. The testing libraries used in the project can be installed by running `make install_pckgs`. 

# Planned Improvements
1. Physical SQL execution and query optimization
2. Transactions and MVCC
3. Thread-safe index operations
