#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

void set_node_root(void* node, bool is_root);

typedef enum{
	EXECUTE_SUCCESS,
	EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY
} ExecuteResult;

typedef enum{
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;


typedef enum{
	META_COMMAND_SUCCESS,
	META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum{
	PREPARE_SUCCESS,
	PREPARE_NEGATIVE_ID,
	PREPARE_STRING_TOO_LONG,
	PREPARE_SYNTAX_ERROR,
	PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum{
	STATEMENT_INSERT,
	STATEMENT_SELECT
} StatementType;

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct{
	uint32_t id;
	char username[COLUMN_USERNAME_SIZE+1];
	char email[COLUMN_EMAIL_SIZE+1];
} Row;

typedef struct{
	StatementType type;
	Row row_to_insert;
} Statement;

#define size_of_attribute(Struct,Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row,id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row,username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row,email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;

#define TABLE_MAX_PAGES 100

// const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
// const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct{
	int file_descriptor;
	uint32_t file_length;
    uint32_t num_pages;
	void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct{
	//uint32_t num_rows;
	//void* pages[TABLE_MAX_PAGES];
	uint32_t root_page_num;
    Pager* pager;
} Table;

typedef struct{
	Table* table;
	//uint32_t row_num;
    uint32_t page_num;
    uint32_t cell_num;
	bool end_of_table;
} Cursor;

/*
-------------
Leaf Node Layout 
-------------
byte 0    | byte 1  | bytes 2-5      | bytes 6-9     |
node_type | is_root | parent_pointer | num_cells     |
    bytes 6-9       | bytes 10-13    | bytes 14-306  | 
    num_cells       |   key 0        |   value 0     | CELLS
                    ........
        bytes 14-306 | bytes 307-310 | bytes 311-603 |
        value 0      |      key 1    |    value 1    |
                    ........
        bytes 311-603   |   bytes 604-607            |
            value 1     |       key 2                |
                    ........
                    bytes 3578-3871                  |
                        value 12                     |
                bytes 3578-3871      |bytes 3871-4095|
                    value 12         |wasted space   |
                     bytes 3871-4095                 |
                       wasted space                  |
*/

//Node Header Layout
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

//Leaf Node Header Layout
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
//const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE;

//Leaf Node Body Layout
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;
const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT; 

//Internal Node Layout
/*
   byte 0    |   byte 1   |   bytes 2-5          |   bytes 6-9        |
  node_type  |  is_root   |  parent_pointer      |    num keys        |
        bytes 6-9         |   bytes 10-13        |   bytes 14-17      |
          num keys        |  right child pointer |   child pointer 0  |
        bytes 14-17       |   bytes 18-21        |   bytes 22-25      |
          child pointer 0 |      key 0           |   child pointer 1  |
        bytes 22-25       |   bytes 26-29        |    .........       |
          child pointer 1 |      key 1           |    .........       |
          ............................           |   bytes 4086-4089  |
          ............................           |   child pointer 509|
        bytes 4086-4089   |   bytes 4090-4093    |   bytes 4094-4095  |
        child pointer 509 |      key 509         |     wasted space   |

*/ 

//Internal Node Header Layout
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

//Internal Node Body Layout
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;
const uint32_t INTERNAL_NODE_MAX_CELLS =3;

//类型强转应该不会出问题吧？观望一下
uint32_t* leaf_node_num_cells(void* node){
    return (uint32_t*)(node + LEAF_NODE_NUM_CELLS_OFFSET);
}

void* leaf_node_cell(void* node, uint32_t cell_num){
    return node + LEAF_NODE_HEADER_SIZE + cell_num*LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num){
    return (uint32_t*)leaf_node_cell(node,cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num){
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

NodeType get_node_type(void* node){
    uint8_t value = *((uint8_t*)node + NODE_TYPE_OFFSET);
    return (NodeType)value;
}

void set_node_type(void* node, NodeType type){
    uint8_t value = type;
    *((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

uint32_t* leaf_node_next_leaf(void* node){
    return (uint32_t*)(node + LEAF_NODE_NEXT_LEAF_OFFSET);
}

void initialize_leaf_node(void* node){
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *(leaf_node_num_cells(node)) = 0;
    *(leaf_node_next_leaf(node)) = 0;//0 表示没有兄弟节点
}

void print_constants(){
    printf("ROW_SIZE:%d\n",ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE:%d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE:%d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE:%d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS:%d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS:%d\n", LEAF_NODE_MAX_CELLS);
}

// void print_leaf_node(void* node){
//     uint32_t num_cells = *leaf_node_num_cells(node);
//     printf("leaf (size %d)\n", num_cells);
//     for(uint32_t i = 0;i < num_cells;++i){
//         uint32_t key = *leaf_node_key(node,i);
//         printf(" - %d : %d\n", i, key);
//     }
// }

//Internal node operations:
uint32_t* internal_node_num_keys(void* node){
    return (uint32_t*)(node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t* internal_node_right_child(void* node){
    return (uint32_t*)(node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t* internal_node_cell(void* node, uint32_t cell_num){
    return (uint32_t*)(node + INTERNAL_NODE_HEADER_SIZE + cell_num*INTERNAL_NODE_CELL_SIZE);
}

uint32_t internal_node_find_child(void* node, uint32_t key){
	uint32_t num_keys = *internal_node_num_keys(node);

	uint32_t min_index = 0;
	uint32_t max_index = num_keys;
}

uint32_t* internal_node_key(void* node, uint32_t key_num){
    return internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key){
	uint32_t old_child_index = internal_node_find_child(node, old_key);
	*internal_node_key(node, old_child_index) = new_key;
}

uint32_t* internal_node_child(void* node, uint32_t child_num){
    uint32_t num_keys = *internal_node_num_keys(node);
    if(child_num > num_keys){
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    }else if(child_num == num_keys){
        return internal_node_right_child(node);
    }else{
        return internal_node_cell(node, child_num);
    }   
}

uint32_t* node_parent(void* node){ 
    return (uint32_t*)(node + PARENT_POINTER_OFFSET);
}

uint32_t get_node_max_key(void* node){
    switch(get_node_type(node)){
        case NODE_INTERNAL:
            return *internal_node_key(node, *internal_node_num_keys(node) - 1);
        case NODE_LEAF:
            return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);        
    }
}

bool is_node_root(void* node){
    uint8_t value = *((uint8_t*)(node + IS_ROOT_OFFSET));
    return (bool)value;
}

void set_node_root(void* node, bool is_root){
    uint8_t value = is_root;
    *((uint8_t*)(node + IS_ROOT_OFFSET)) = value;
}

void initialize_internal_node(void* node){
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *internal_node_num_keys(node) = 0;
}

