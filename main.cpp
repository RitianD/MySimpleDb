#include "main.h"


void* get_page(Pager* pager, uint32_t page_num);

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
	Cursor* cursor = (Cursor*)malloc(sizeof(Cursor));
	cursor->table = table;
	// cursor->row_num = 0;
	// cursor->end_of_table = (table->num_rows == 0);
	cursor->page_num = table->root_page_num;
	cursor->cell_num = 0;

	void* root_node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *(uint32_t*)leaf_node_num_cells(root_node);
	cursor->end_of_table = (num_cells == 0);
	return cursor;
}

Cursor* table_end(Table* table){
	Cursor* cursor = (Cursor*)malloc(sizeof(Cursor));
	cursor->table = table;
	//cursor->row_num = table->num_rows;
	cursor->page_num = table->root_page_num;
	void* root_node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *leaf_node_num_cells(root_node);
	cursor->cell_num = num_cells;
	cursor->end_of_table = true;
	return cursor;
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
		cursor->end_of_table = true;
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

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table){
	if(strcmp(input_buffer->buffer, ".exit") == 0){
		close_input_buffer(input_buffer);
		// free_table(table);
		db_close(table);
		exit(EXIT_SUCCESS);			
	}else if(strcmp(input_buffer->buffer, ".btree") == 0){
		printf("Tree:\n");
		print_leaf_node(get_page(table->pager, 0));
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
		printf("Need to implement splitting a leaf node.\n");
		exit(EXIT_FAILURE);
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
	// if(table->num_rows >= TABLE_MAX_ROWS){
	// 	return EXECUTE_TABLE_FULL;
	// }
	void* node = get_page(table->pager, table->root_page_num);
	if((*leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS)){
		return EXECUTE_TABLE_FULL;
	}

	Row* row_to_insert = &(statement->row_to_insert);
	Cursor* cursor = table_end(table);

	// serialize_row(row_to_insert, cursor_value(cursor));
	// table->num_rows += 1;
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
		case EXECUTE_TABLE_FULL:
			printf("Error: Table full.\n");
			break;
		}
	}
	return 0;
}