import os
input_file = "ftcommonstatus.h"
output_file = "ftcommonstatus.h.out"
f_in = open(input_file, "r", encoding="utf-8")
f_out = open(output_file, "w", encoding="utf-8")

insert_str = "NULL, // "

for line in f_in:
	if line.endswith("// Proc Update\n") or line.endswith("// Proc Interrupt\n") or line.endswith("// Proc Physics\n") or line.endswith("// Proc Map\n"):
		line = insert_str + line
	f_out.write(line)
	
os.replace(output_file, input_file)