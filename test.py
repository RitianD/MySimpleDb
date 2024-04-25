import subprocess
import os
import unittest


# def test_insert_and_retrieve_row():
#     result = run_script([
#         "insert 1 user1 person1@example.com",
#         "select",
#         ".exit",
#     ])
#     expected_output = [
#         "db >Executed.",
#         "db >(1, user1, person1@example.com)",
#         "Executed.",
#         "db >",
#     ]
#     assert result == expected_output
#     print("test1 pass")

# def test_prints_error_message_when_table_is_full():
#     script = [f"insert {i} user{i} person{i}@example.com" for i in range(1,1402)]
#     script.append(".exit")
#     result = run_script(script)
#     assert result[-2] == 'db >Error: Table full.'
#     print("test2 pass")

# def test_long_strings():
#     long_username = "a" * 32
#     long_email = "a" * 255
#     script = [
#         f"insert 1 {long_username} {long_email}",
#         "select",
#         ".exit"
#     ]
#     result = run_script(script)
#     expected_output = [
#         "db >Executed.",
#         f"db >(1, {long_username}, {long_email})",
#         "Executed.",
#         "db >"
#     ]
#     assert result == expected_output
#     print("test3 pass")

# def test_prints_error_message_if_strings_too_long():
#     long_username = "a" * 53
#     long_email = "a" * 258
#     script = [
#         f"insert 1 {long_username} {long_email}",
#         "select",
#         ".exit"
#     ]
#     result = run_script(script)
#     expected_output = [
#         "db >String is too long.",
#         "db >Executed.",
#         "db >"
#     ]
#     assert result == expected_output
#     print("test4 pass")

# def test_negative_id_error_message():
#     script = [
#         "insert -1 cstack foo@bar.com",
#         "select",
#         ".exit"
#     ]
#     result = run_script(script)
#     expected_output = [
#         "db >ID must be positive.",
#         "db >Executed.",
#         "db >"
#     ]
#     assert result == expected_output
#     print("test5 pass")

# def test_keeps_data_after_closing_connection():
#     result1 = run_script([
#         "insert 1 user1 person1@example.com",
#         ".exit",
#     ])
#     expected_output1 = [
#         "db >Executed.",
#         "db >",
#     ]
#     assert result1 == expected_output1
#     result2 = run_script([
#         "select",
#         ".exit",
#     ])
#     expected_output2 = [
#         "db >(1, user1, person1@example.com)",
#         "Executed.",
#         "db >",
#     ]
#     assert result2 == expected_output2
#     print("test6 pass")



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
            "leaf (size 3)",
            " - 0 : 3",
            " - 1 : 1",
            " - 2 : 2",
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
            "LEAF_NODE_HEADER_SIZE:10",
            "LEAF_NODE_CELL_SIZE:297",
            "LEAF_NODE_SPACE_FOR_CELLS:4086",
            "LEAF_NODE_MAX_CELLS:13",
            "db >",
        ]
        self.assertEqual(result, expected_output)
        print("test_print_constants pass")

if __name__ == "__main__":
    unittest.main()