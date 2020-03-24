
parser_files = ["source/parser_statement.c", "source/parser_expression.c", "source/parser_declaration.c"]

def count_todos(path):
  with open(path, 'r') as source_file:
    source = source_file.read()
  
    todos = len(source.split("@TODO")) - 1
    done = len(source.split("@DONE")) - 1

    return todos, done

todos = 0
done = 0

for parser_file in parser_files:
  _todos, _done = count_todos(parser_file)
  todos = todos + _todos
  done = done + _done

print("==== Progress implementing Parser so far: =====")
print(f"Progress: {done} / {(todos + done)} - {int(done / (todos + done))}%")

  
