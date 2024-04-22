import subprocess

def run_script(commands):
    raw_output = None
    with subprocess.Popen(["./main"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True) as process:
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
        f"db >String is too long.",
        "db >Executed.",
        "db >"
    ]
    assert result == expected_output
    print("test4 pass")


if __name__ == "__main__":
    test_insert_and_retrieve_row()
    test_prints_error_message_when_table_is_full()
    test_long_strings()
    test_prints_error_message_if_strings_too_long()