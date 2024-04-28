#include "main.h"


void* get_page(Pager* pager, uint32_t page_num);
Cursor* table_find(Table* table, uint32_t key);
void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

void print_row(Row* row){
	printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

void serialize_row(Row* source, void* destination){
	memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
	memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
	memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination){
	memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
	memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
	memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

Cursor* table_start(Table* table){
	// Cursor* cursor = (Cursor*)malloc(sizeof(Cursor));
	// cursor->table = table;

	// cursor->page_num = table->root_page_num;
	// cursor->cell_num = 0;

	// void* root_node = get_page(table->pager, table->root_page_num);
	// uint32_t num_cells = *(uint32_t*)leaf_node_num_cells(root_node);
	// cursor->end_of_table = (num_cells == 0);
	Cursor* cursor = table_find(table, 0);
	void* node = get_page(table->pager, cursor->page_num);
	uint32_t num_cells = *leaf_node_num_cells(node);
	cursor->end_of_table = (num_cells == 0);
	
	return cursor;
}

Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key){
	void* node = get_page(table->pager, page_num);
	uint32_t num_cells = * leaf_node_num_cells(node);

	Cursor* cursor = (Cursor*)malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->page_num = page_num;

	//Binary search
	uint32_t min_index = 0;
	uint32_t one_past_max_index = num_cells;
	while(one_past_max_index != min_index){
		uint32_t index = (min_index + one_past_max_index) / 2;
		uint32_t key_at_index = *leaf_node_key(node,index);
		if(key == key_at_index){
			cursor->cell_num = index;
			return cursor;
		}
		if(key < key_at_index){
			one_past_max_index = index;
		}else{
			min_index = index + 1;
		}
	}

	cursor->cell_num = min_index;
	return cursor;
}

uint32_t get_unused_page_num(Pager* pager){ return pager->num_pages;}

void create_new_root(Table* table, uint32_t right_child_page_num){
	void* root = get_page(table->pager, table->root_page_num);
	void* right_child = get_page(table->pager, right_child_page_num);
	uint32_t left_child_page_num = get_unused_page_num(table->pager);
	void* left_child = get_page(table->pager, left_child_page_num);

	memcpy(left_child, root, PAGE_SIZE);
	set_node_root(left_child, false);

	initialize_internal_node(root);
	set_node_root(root, true);
	*internal_node_num_keys(root) = 1;
	*internal_node_child(root, 0) = left_child_page_num;
	uint32_t left_child_max_key = get_node_max_key(left_child);
	*internal_node_key(root, 0) = left_child_max_key;
	*internal_node_right_child(root) = right_child_page_num;
	*node_parent(left_child) = table->root_page_num;
	*node_parent(right_child) = table->root_page_num;
}

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value){
	void* old_node = get_page(cursor->table->pager, cursor->page_num);
	uint32_t old_max = get_node_max_key(old_node);
	uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
	void* new_node = get_page(cursor->table->pager, new_page_num);
	initialize_leaf_node(new_node);
	*node_parent(new_node) = *node_parent(old_node);
	*leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
	*leaf_node_next_leaf(old_node) = new_page_num;

	for(int32_t i = LEAF_NODE_MAX_CELLS; i>=0; i--){
		void* destination_node;
		if(i >= LEAF_NODE_LEFT_SPLIT_COUNT){
			destination_node = new_node;
		}else{
			destination_node = old_node;
		}

		uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
		void* destination = leaf_node_cell(destination_node, index_within_node);
		//printf("split and insert i:%u, cell_num:%u\n", i, cursor->cell_num);
		if(i == cursor->cell_num){
			//serialize_row(value, destination);
			serialize_row(value, leaf_node_value(destination_node, index_within_node));
			*leaf_node_key(destination_node, index_within_node) = key;
		}else if(i > cursor->cell_num){
			memcpy(destination, leaf_node_cell(old_node, i-1), LEAF_NODE_CELL_SIZE);
		}else{
			memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
		}
	}

	*(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
	*(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

	if(is_node_root(old_node)){
		return create_new_root(cursor->table, new_page_num);
	}else{
		// printf("Need to implement updating parent after split\n");
		// exit(EXIT_FAILURE);
		uint32_t parent_page_num = *node_parent(old_node);
		uint32_t new_max = get_node_max_key(old_node);
		void* parent = get_page(cursor->table->pager, parent_page_num);

		update_internal_node_key(parent, old_max, new_max);
		internal_node_insert(cursor->table, parent_page_num, new_page_num);
		return;
	}
}

// Cursor* table_end(Table* table){
// 	Cursor* cursor = (Cursor*)malloc(sizeof(Cursor));
// 	cursor->table = table;
// 	//cursor->row_num = table->num_rows;
// 	cursor->page_num = table->root_page_num;
// 	void* root_node = get_page(table->pager, table->root_page_num);
// 	uint32_t num_cells = *leaf_node_num_cells(root_node);
// 	cursor->cell_num = num_cells;
// 	cursor->end_of_table = true;
// 	return cursor;
// }

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num){
	void* parent = get_page(table->pager, parent_page_num);
	void* child = get_page(table->pager,child_page_num);
	uint32_t child_max_key = get_node_max_key(child);
	uint32_t index = internal_node_find_child(parent, child_max_key);

	uint32_t original_num_keys = *internal_node_num_keys(parent);
	*internal_node_num_keys(parent) = original_num_keys + 1;

	if(original_num_keys >= INTERNAL_NODE_MAX_CELLS){
		printf("Need to implement splitting internal node\n");
		exit(EXIT_FAILURE);
	}

	uint32_t right_child_page_num = *internal_node_right_child(parent);
	void* right_child = get_page(table->pager, right_child_page_num);

	if(child_max_key > get_node_max_key(right_child)){
		*internal_node_child(parent, original_num_keys) = right_child_page_num;
		*internal_node_key(parent, original_num_keys) = get_node_max_key(right_child);
		*internal_node_right_child(parent) = child_page_num;
	}else{
		for(uint32_t i = original_num_keys;i>index;i--){
			void* destination = internal_node_cell(parent, i);
			void* source = internal_node_cell(parent, i-1);
			memcpy(destination,source, INTERNAL_NODE_CELL_SIZE);
		}
		*internal_node_child(parent, index) = child_page_num;
		*internal_node_key(parent, index) = child_max_key;
	}
}

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key){
    void* node = get_page(table->pager, page_num);
    uint32_t num_keys = *internal_node_num_keys(node);

    //二分查找
    uint32_t min_index = 0;
    uint32_t max_index = num_keys;

    while(min_index != max_index){
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = *internal_node_key(node,index);
        if(key_to_right >= key){
            max_index = index;
        }
        else{
            min_index = index + 1;
        }       
    }

	uint32_t child_index = internal_node_find_child(node, key);
    uint32_t child_num = *internal_node_child(node, child_index);
    void* child = get_page(table->pager, child_num);
    switch(get_node_type(child)){
        case NODE_LEAF:
            return leaf_node_find(table, child_num, key);
        case NODE_INTERNAL:
            return internal_node_find(table, child_num, key);
    }
}


Cursor* table_find(Table* table, uint32_t key){
	uint32_t root_page_num = table->root_page_num;
	void* root_node = get_page(table->pager, root_page_num);

	if(get_node_type(root_node) == NODE_LEAF){
		return leaf_node_find(table, root_page_num, key);
	}else{
		// printf("Need to implement searching an internal node");
		// exit(EXIT_FAILURE);
		return internal_node_find(table, root_page_num, key);
	}
}

void cursor_advance(Cursor* cursor){
	// cursor->row_num += 1;
	// if(cursor->row_num >= cursor->table->num_rows){
	// 	cursor->end_of_table = true;
	// }
	uint32_t page_num = cursor->page_num;
	void* node = get_page(cursor->table->pager, page_num);

	cursor->cell_num += 1;
	if(cursor->cell_num >= (*leaf_node_num_cells(node))){
		//cursor->end_of_table = true;
		//如果有兄弟节点，则肯定没走到头
		uint32_t next_page_num = *leaf_node_next_leaf(node);
		if(next_page_num == 0){
			cursor->end_of_table = true;
		}else{
			cursor->page_num = next_page_num;
			cursor->cell_num = 0;
		}
	}
}

void* get_page(Pager* pager, uint32_t page_num){
	if(page_num > TABLE_MAX_PAGES){
		printf("Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
		exit(EXIT_FAILURE);
	}

	if(pager->pages[page_num] == NULL){
		//Cache miss. Allocate memory and load from file
		void* page = malloc(PAGE_SIZE);
		uint32_t num_pages = pager->file_length / PAGE_SIZE;

		//
		if(pager->file_length % PAGE_SIZE){
			num_pages += 1;
		}

		if(page_num <= num_pages){
			lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
			ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
			if(bytes_read == -1){
				printf("Error reading file: %d\n", errno);
				exit(EXIT_FAILURE);
			}
		}

		pager->pages[page_num] = page;
		if(page_num >= pager->num_pages){
			pager->num_pages = page_num + 1;
		}
	}
	return pager->pages[page_num];
}

void* cursor_value(Cursor* cursor){
	// uint32_t row_num = cursor->row_num;
	// uint32_t page_num = row_num / ROWS_PER_PAGE;
	uint32_t page_num = cursor->page_num;
	void* page = get_page(cursor->table->pager, page_num);
	// uint32_t row_offset = row_num % ROWS_PER_PAGE;
	// uint32_t byte_offset = row_offset * ROW_SIZE;
	// return page + byte_offset;
	return leaf_node_value(page,cursor->cell_num);
}

Pager* pager_open(const char* filename){
	int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
	if(fd == -1){
		printf("Unable to open file\n");
		exit(EXIT_FAILURE);
	}

	off_t file_length = lseek(fd, 0, SEEK_END);

	Pager* pager = (Pager*)malloc(sizeof(Pager));
	pager->file_descriptor = fd;
	pager->file_length = file_length;
	pager->num_pages = (file_length / PAGE_SIZE);

	if(file_length % PAGE_SIZE != 0){
		printf("db file is not a whole number of pages. Corrupt file.\n");
		exit(EXIT_FAILURE);
	}

	for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
		pager->pages[i] = NULL;
	}

	return pager;
}

void pager_flush(Pager* pager, uint32_t page_num){
	if(pager->pages[page_num] == NULL){
		printf("Tried to flush null page\n");
		exit(EXIT_FAILURE);
	}

	off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

	if(offset == -1){
		printf("Error seeking: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

	if(bytes_written == -1){
		printf("Error writing: %d\n", errno);
		exit(EXIT_FAILURE);
	}
}

Table* db_open(const char* filename){
	Pager* pager = pager_open(filename);
	//uint32_t num_rows = pager->file_length / ROW_SIZE;
	Table* table = (Table*)malloc(sizeof(Table));
	table->pager = pager;
	//table->num_rows = num_rows;
	table->root_page_num = 0;

	if(pager->num_pages == 0){
		void* root_node = get_page(pager, 0);
		initialize_leaf_node(root_node);
		set_node_root(root_node, true);
	}
	
	return table;
}

void db_close(Table* table){
	Pager* pager = table->pager;
	//改为一个node占一页
	//uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;
	
	for(uint32_t i = 0; i < pager->num_pages; i++){
		if(pager->pages[i] == NULL) continue;
		pager_flush(pager, i);
		free(pager->pages[i]);
		pager->pages[i] = NULL;
	}
	
	//
	// uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
	// if(num_additional_rows > 0){
	// 	uint32_t page_num = num_full_pages;
	// 	if(pager->pages[page_num]!=NULL){
	// 		pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
	// 		free(pager->pages[page_num]);
	// 		pager->pages[page_num] = NULL;
	// 	}
	// }

	int result = close(pager->file_descriptor);
	if(result == -1){
		printf("Error closing db file.\n");
		exit(EXIT_FAILURE);
	}

	for(uint32_t i = 0; i < TABLE_MAX_PAGES;i++){
		void* page = pager->pages[i];
		if(page){
			free(page);
			pager->pages[i] = NULL;
		}
	}
	free(pager);
	free(table);
}

// void free_table(Table* table){
// 	for(int i = 0;table->pages[i];i++){
// 		free(table->pages[i]);
// 	}
// 	free(table);
// }

typedef struct{
	char* buffer;
	size_t buffer_length;
	ssize_t input_length;
} InputBuffer;

void close_input_buffer(InputBuffer* input_buffer){
	free(input_buffer->buffer);
	free(input_buffer);
}

void indent(uint32_t level){
    for(uint32_t i = 0;i < level; ++i){
        printf("  ");
    }
}

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level){
    void* node = get_page(pager, page_num);
    uint32_t num_keys, child;

    switch (get_node_type(node))
    {
    case NODE_LEAF:
        num_keys = *leaf_node_num_cells(node);
        indent(indentation_level);
        printf("- leaf (size %d)\n", num_keys);
        for(uint32_t i = 0; i< num_keys;++i){
            indent(indentation_level + 1);
            printf("- %d\n", *leaf_node_key(node,i));
        }
        break;
    case NODE_INTERNAL:
        num_keys = *internal_node_num_keys(node);
        indent(indentation_level);
        printf("- internal (size %d)\n", num_keys);
        for(uint32_t i = 0;i<num_keys;++i){
            child = *internal_node_child(node,i);
            print_tree(pager, child, indentation_level + 1);

            indent(indentation_level + 1);
            printf("- key %d\n", *internal_node_key(node,i));
        }
        child = *internal_node_right_child(node);
        print_tree(pager, child, indentation_level + 1);
        break;
    }
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table){
	if(strcmp(input_buffer->buffer, ".exit") == 0){
		close_input_buffer(input_buffer);
		// free_table(table);
		db_close(table);
		exit(EXIT_SUCCESS);			
	}else if(strcmp(input_buffer->buffer, ".btree") == 0){
		printf("Tree:\n");
		//print_leaf_node(get_page(table->pager, 0));
		print_tree(table->pager, 0, 0);
		return META_COMMAND_SUCCESS;
	}else if(strcmp(input_buffer->buffer, ".constants") == 0){
		printf("Constants:\n");
		print_constants();
		return META_COMMAND_SUCCESS;
	}else{
		return META_COMMAND_UNRECOGNIZED_COMMAND;
	}
}

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value){
	void* node = get_page(cursor->table->pager, cursor->page_num);
	
	uint32_t num_cells = *leaf_node_num_cells(node);
	//printf("num_cells:%d cursor->cell_num:%d\n",num_cells, cursor->cell_num);
	if(num_cells >= LEAF_NODE_MAX_CELLS){
		// printf("Need to implement splitting a leaf node.\n");
		// exit(EXIT_FAILURE);
		// 节点已满，则调整节点
		leaf_node_split_and_insert(cursor, key, value);
		return;
	}

	if(cursor->cell_num < num_cells){
		for(uint32_t i = num_cells; i > cursor->cell_num; i--){
			memcpy(leaf_node_cell(node,i), leaf_node_cell(node, i-1), LEAF_NODE_CELL_SIZE);
		}
	}

	*(leaf_node_num_cells(node)) += 1;
	*(leaf_node_key(node, cursor->cell_num)) = key;
	serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement){
		statement->type = STATEMENT_INSERT;

		char* keyword = strtok(input_buffer->buffer, " ");
		char* id_string = strtok(NULL, " ");
		char* username = strtok(NULL, " ");
		char* email = strtok(NULL, " ");

		if(id_string == NULL || username == NULL || email == NULL){
			return PREPARE_SYNTAX_ERROR;
		}

		int id = atoi(id_string);
		if(id < 0){
			return PREPARE_NEGATIVE_ID;
		}
		if(strlen(username) > COLUMN_USERNAME_SIZE || strlen(email) > COLUMN_EMAIL_SIZE){
			return PREPARE_STRING_TOO_LONG;
		}
		
		statement->row_to_insert.id = id;
		strcpy(statement->row_to_insert.username, username);
		strcpy(statement->row_to_insert.email, email);
		
		return PREPARE_SUCCESS;
	
	
}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement){
	if(strncmp(input_buffer->buffer, "insert", 6) == 0){
		return prepare_insert(input_buffer, statement);
	}
	if(strcmp(input_buffer->buffer, "select") == 0){
		statement->type = STATEMENT_SELECT;
		return PREPARE_SUCCESS;
	}
	return PREPARE_UNRECOGNIZED_STATEMENT;

}

ExecuteResult execute_insert(Statement* statement, Table* table){
	void* node = get_page(table->pager, table->root_page_num);

	uint32_t num_cells = (*leaf_node_num_cells(node));

	Row* row_to_insert = &(statement->row_to_insert);
	uint32_t key_to_insert = row_to_insert->id;
	Cursor* cursor = table_find(table, key_to_insert);

	if(cursor->cell_num < num_cells){
		uint32_t key_at_index = *leaf_node_key(node,cursor->cell_num);
		if(key_at_index == key_to_insert){
			return EXECUTE_DUPLICATE_KEY;
		}
	}

	leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

	free(cursor);
	return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* Statement, Table* table){
	Cursor* cursor = table_start(table);
	Row row;
	// for(uint32_t i = 0;i < table->num_rows; ++i){
	// 	deserialize_row(row_slot(table, i), &row);
	while(!(cursor->end_of_table)){
		deserialize_row(cursor_value(cursor), &row);
		print_row(&row);
		cursor_advance(cursor);
	}
	free(cursor);
	return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table){
	switch (statement->type)
	{
	case STATEMENT_INSERT:
		return execute_insert(statement, table);
	case STATEMENT_SELECT:
		return execute_select(statement, table);
	}
}

InputBuffer* new_input_buffer(){
	InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;

	return input_buffer;
}

void print_prompt() {printf("db >");}

void read_input(InputBuffer* input_buffer){
	ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
	if(bytes_read <= 0){
		printf("Error reading input\n");
		exit(EXIT_FAILURE);
	}

	input_buffer->input_length = bytes_read -1;
	input_buffer->buffer[bytes_read - 1] = 0;
}


int main(int argc, char* argv[]){
	InputBuffer* input_buffer = new_input_buffer();
	//Table* table = new_table();
	if(argc < 2){
		printf("Must supply a database filename.\n");
		exit(EXIT_FAILURE);
	}

	char* filename = argv[1];
	Table* table = db_open(filename);
	
	while(true){
		print_prompt();
		read_input(input_buffer);
		
		if(input_buffer->buffer[0]=='.'){
			switch (do_meta_command(input_buffer, table))
			{
			case META_COMMAND_SUCCESS:
				continue;
			case META_COMMAND_UNRECOGNIZED_COMMAND:
				printf("Unrecognized command '%s'\n", input_buffer->buffer);
				continue;
			default:
				break;
			}
		}

		Statement statement;
		switch (prepare_statement(input_buffer, &statement))
		{
		case PREPARE_SUCCESS:
			break;
		case PREPARE_NEGATIVE_ID:
			printf("ID must be positive.\n");
			continue;
		case PREPARE_STRING_TOO_LONG:
			printf("String is too long.\n");
			continue;
		case PREPARE_SYNTAX_ERROR:
			printf("Syntax error. Could not parse statement.\n");
			continue;
		case PREPARE_UNRECOGNIZED_STATEMENT:
			printf("Unrecognized keyword at start of '%s'.\n",input_buffer->buffer);
			continue;
		}

		switch (execute_statement(&statement, table))
		{
		case EXECUTE_SUCCESS:
			printf("Executed.\n");
			break;
		case EXECUTE_DUPLICATE_KEY:
			printf("Error: Duplicate key.\n");
			break;
		case EXECUTE_TABLE_FULL:
			printf("Error: Table full.\n");
			break;
		}
	}
	return 0;
}