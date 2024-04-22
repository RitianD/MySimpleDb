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

def test_prints_error_message_when_table_is_full():
    script = [f"insert {i} user{i} person{i}@example.com" for i in range(1,1402)]
    #script.append("select")
    script.append(".exit")
    result = run_script(script)
    #print(result[-2])
    assert result[-2] == 'db >Error: Table full.'

if __name__ == "__main__":
    #test_insert_and_retrieve_row()
    test_prints_error_message_when_table_is_full()