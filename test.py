import subprocess
import os
import unittest

class TestMyCode(unittest.TestCase):
    def run_script(self,commands):
        os.system("rm -rf test.db")    
        raw_output = None
        with subprocess.Popen(["./main","test.db"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True) as process:
            for command in commands:
                process.stdin.write(command + '\n')
            process.stdin.close()
            raw_output = process.stdout.read()
        return raw_output.split("\n")

    def test_insert_and_retrieve_row(self):
        result = self.run_script([
            "insert 1 user1 person1@example.com",
            "select",
            ".exit",
        ])
        expected_output = [
            "db >Executed.",
            "db >(1, user1, person1@example.com)",
            "Executed.",
            "db >",
        ]
        assert result == expected_output
        print("test_insert_and_retrieve_row pass")

    # def test_prints_error_message_when_table_is_full(self):
    #     script = [f"insert {i} user{i} person{i}@example.com" for i in range(1,1402)]
    #     script.append(".exit")
    #     result = self.run_script(script)
    #     assert result[-2] == 'db >Error: Table full.'
    #     print("test_prints_error_message_when_table_is_full pass")

    def test_long_strings(self):
        long_username = "a" * 32
        long_email = "a" * 255
        script = [
            f"insert 1 {long_username} {long_email}",
            "select",
            ".exit"
        ]
        result = self.run_script(script)
        expected_output = [
            "db >Executed.",
            f"db >(1, {long_username}, {long_email})",
            "Executed.",
            "db >"
        ]
        assert result == expected_output
        print("test_long_strings pass")

    def test_prints_error_message_if_strings_too_long(self):
        long_username = "a" * 53
        long_email = "a" * 258
        script = [
            f"insert 1 {long_username} {long_email}",
            "select",
            ".exit"
        ]
        result = self.run_script(script)
        expected_output = [
            "db >String is too long.",
            "db >Executed.",
            "db >"
        ]
        assert result == expected_output
        print("test_prints_error_message_if_strings_too_long pass")

    def test_negative_id_error_message(self):
        script = [
            "insert -1 cstack foo@bar.com",
            "select",
            ".exit"
        ]
        result = self.run_script(script)
        expected_output = [
            "db >ID must be positive.",
            "db >Executed.",
            "db >"
        ]
        assert result == expected_output
        print("test_negative_id_error_message pass")

    # def test_keeps_data_after_closing_connection(self):
    #     result1 = self.run_script([
    #         "insert 1 user1 person1@example.com",
    #         ".exit",
    #     ])
    #     expected_output1 = [
    #         "db >Executed.",
    #         "db >",
    #     ]
    #     assert result1 == expected_output1
    #     result2 = self.run_script([
    #         "select",
    #         ".exit",
    #     ])
    #     expected_output2 = [
    #         "db >(1, user1, person1@example.com)",
    #         "Executed.",
    #         "db >",
    #     ]
    #     assert result2 == expected_output2
    #     print("test_keeps_data_after_closing_connection pass")

    def test_print_one_node_btree_structure(self):
        commands = [
            "insert 3 user3 person3@example.com",
            "insert 1 user1 person1@example.com",
            "insert 2 user2 person2@example.com",
            ".btree",
            ".exit",
        ]
        result = self.run_script(commands)
        expected_output = [
            "db >Executed.",
            "db >Executed.",
            "db >Executed.",
            "db >Tree:",
            "- leaf (size 3)",
            "  - 1",
            "  - 2",
            "  - 3",
            "db >",
        ]
        self.assertEqual(result, expected_output)
        print("test_print_one_node_btree_structure pass")
    
    def test_print_constants(self):
        commands = [
            ".constants",
            ".exit",
        ]
        result = self.run_script(commands)
        expected_output = [
            "db >Constants:",
            "ROW_SIZE:293",
            "COMMON_NODE_HEADER_SIZE:6",
            "LEAF_NODE_HEADER_SIZE:14",
            "LEAF_NODE_CELL_SIZE:297",
            "LEAF_NODE_SPACE_FOR_CELLS:4082",
            "LEAF_NODE_MAX_CELLS:13",
            "db >",
        ]
        self.assertEqual(result, expected_output)
        print("test_print_constants pass")
    
    def test_duplicate_id_error(self):
        commands = [
            "insert 1 user1 person1@example.com",
            "insert 1 user1 person1@example.com",
            "select",
            ".exit",
        ]
        result = self.run_script(commands)
        expected_ouput = [
            "db >Executed.",
            "db >Error: Duplicate key.",
            "db >(1, user1, person1@example.com)",
            "Executed.",
            "db >",
        ]
        print(result)
        self.assertEqual(result, expected_ouput)
        print("test_duplicate_id_error pass")

    def test_print_btree_3_leaf_nodes(self):
        commands = []
        for i in range(1, 15):
            commands.append("insert {} user{} person{}@example.com".format(i, i, i))
        commands.append(".btree")
        commands.append("insert 15 user15 person15@example.com")
        commands.append(".exit")
        
        result = self.run_script(commands)
        
        expected_output = [
            "db >Tree:",
            "- internal (size 1)",
            "  - leaf (size 7)",
            "    - 1",
            "    - 2",
            "    - 3",
            "    - 4",
            "    - 5",
            "    - 6",
            "    - 7",
            "  - key 7",
            "  - leaf (size 7)",
            "    - 8",
            "    - 9",
            "    - 10",
            "    - 11",
            "    - 12",
            "    - 13",
            "    - 14",
            "db >Executed.",
            "db >"
        ]
        self.assertEqual(result[14:], expected_output)
        print("test_print_btree_3_leaf_nodes pass")

    # def test_prints_all_rows_in_multi_level_tree(self):
    #     script = []
    #     for i in range(1, 16):
    #         script.append("insert {} user{} person{}@example.com".format(i, i, i))
    #     script.append("select")
    #     script.append(".exit")
    #     result = self.run_script(script)

    #     expected_output = [
    #         "db >(1, user1, person1@example.com)",
    #         "(2, user2, person2@example.com)",
    #         "(3, user3, person3@example.com)",
    #         "(4, user4, person4@example.com)",
    #         "(5, user5, person5@example.com)",
    #         "(6, user6, person6@example.com)",
    #         "(7, user7, person7@example.com)",
    #         "(8, user8, person8@example.com)",
    #         "(9, user9, person9@example.com)",
    #         "(10, user10, person10@example.com)",
    #         "(11, user11, person11@example.com)",
    #         "(12, user12, person12@example.com)",
    #         "(13, user13, person13@example.com)",
    #         "(14, user14, person14@example.com)",
    #         "(15, user15, person15@example.com)",
    #         "Executed.", "db >"
    #     ]
    #     assert result[15:] == expected_output
    #     print("test_prints_all_rows_in_multi_level_tree pass")
    # def test_prints_structure_of_4_leaf_node_btree(self):
    #     script = [
    #         "insert 18 user18 person18@example.com",
    #         "insert 7 user7 person7@example.com",
    #         "insert 10 user10 person10@example.com",
    #         "insert 29 user29 person29@example.com",
    #         "insert 23 user23 person23@example.com",
    #         "insert 4 user4 person4@example.com",
    #         "insert 14 user14 person14@example.com",
    #         "insert 30 user30 person30@example.com",
    #         "insert 15 user15 person15@example.com",
    #         "insert 26 user26 person26@example.com",
    #         "insert 22 user22 person22@example.com",
    #         "insert 19 user19 person19@example.com",
    #         "insert 2 user2 person2@example.com",
    #         "insert 1 user1 person1@example.com",
    #         "insert 21 user21 person21@example.com",
    #         "insert 11 user11 person11@example.com",
    #         "insert 6 user6 person6@example.com",
    #         "insert 20 user20 person20@example.com",
    #         "insert 5 user5 person5@example.com",
    #         "insert 8 user8 person8@example.com",
    #         "insert 9 user9 person9@example.com",
    #         "insert 3 user3 person3@example.com",
    #         "insert 12 user12 person12@example.com",
    #         "insert 27 user27 person27@example.com",
    #         "insert 17 user17 person17@example.com",
    #         "insert 16 user16 person16@example.com",
    #         "insert 13 user13 person13@example.com",
    #         "insert 24 user24 person24@example.com",
    #         "insert 25 user25 person25@example.com",
    #         "insert 28 user28 person28@example.com",
    #         ".btree",
    #         ".exit",
    #     ]
    #     result = self.run_script(script)
    #     print(result)

if __name__ == "__main__":
    unittest.main()