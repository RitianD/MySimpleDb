import subprocess
import os

def run_script(commands):
    
    raw_output = None
    with subprocess.Popen(["./main","test.db"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True) as process:
        for command in commands:
            process.stdin.write(command + '\n')
        process.stdin.close()

        raw_output = process.stdout.read()
    
    return raw_output.split("\n")

def test_insert_and_retrieve_row():
    result = run_script([
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
    print("test1 pass")

def test_prints_error_message_when_table_is_full():
    script = [f"insert {i} user{i} person{i}@example.com" for i in range(1,1402)]
    script.append(".exit")
    result = run_script(script)
    assert result[-2] == 'db >Error: Table full.'
    print("test2 pass")

def test_long_strings():
    long_username = "a" * 32
    long_email = "a" * 255
    script = [
        f"insert 1 {long_username} {long_email}",
        "select",
        ".exit"
    ]
    result = run_script(script)
    expected_output = [
        "db >Executed.",
        f"db >(1, {long_username}, {long_email})",
        "Executed.",
        "db >"
    ]
    assert result == expected_output
    print("test3 pass")

def test_prints_error_message_if_strings_too_long():
    long_username = "a" * 53
    long_email = "a" * 258
    script = [
        f"insert 1 {long_username} {long_email}",
        "select",
        ".exit"
    ]
    result = run_script(script)
    expected_output = [
        "db >String is too long.",
        "db >Executed.",
        "db >"
    ]
    assert result == expected_output
    print("test4 pass")

def test_negative_id_error_message():
    script = [
        "insert -1 cstack foo@bar.com",
        "select",
        ".exit"
    ]
    result = run_script(script)
    expected_output = [
        "db >ID must be positive.",
        "db >Executed.",
        "db >"
    ]
    assert result == expected_output
    print("test5 pass")

def test_keeps_data_after_closing_connection():
    result1 = run_script([
        "insert 1 user1 person1@example.com",
        ".exit",
    ])
    expected_output1 = [
        "db >Executed.",
        "db >",
    ]
    assert result1 == expected_output1
    result2 = run_script([
        "select",
        ".exit",
    ])
    expected_output2 = [
        "db >(1, user1, person1@example.com)",
        "Executed.",
        "db >",
    ]
    assert result2 == expected_output2
    print("test6 pass")

if __name__ == "__main__":
    os.system("rm -rf test.db")
    test_insert_and_retrieve_row()
    os.system("rm -rf test.db")
    test_prints_error_message_when_table_is_full()
    os.system("rm -rf test.db")
    #还没有置换内存的策略，内存一满，会读不到应读到的数据;目前内存只有4096*100,超出部分失效，无法正常写入数据；
    test_long_strings()
    os.system("rm -rf test.db")
    test_prints_error_message_if_strings_too_long()
    os.system("rm -rf test.db")
    test_negative_id_error_message()
    os.system("rm -rf test.db")
    test_keeps_data_after_closing_connection()