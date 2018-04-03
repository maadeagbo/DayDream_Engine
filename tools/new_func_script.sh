#!/usr/bin/env bash

# create file
touch "$1"

# Title comment
echo -e "// level script for $2 cpp implementations" > $1
echo -e "#include \"ddLevelPrototype.h\"\n" >> $1

# register function declaration
echo -e "// log lua function that can be called in scripts thru this function" >> $1
echo -e "void $2_func_register(lua_State *L);\n" >> $1

# struct definition
echo -e "// Proxy struct that enables reflection" >> $1
echo -e "struct $2_reflect : public ddLvlPrototype {" >> $1
echo -e "\t$2_reflect() {" >> $1
echo -e "\t\tadd_lua_function(\"$2\", $2_func_register);" >> $1
echo -e "\t}" >> $1
echo -e "};\n" >> $1

# register implementation
echo -e "void $2_func_register(lua_State *L) {" >> $1
echo -e "\t// log functions using register_callback_lua" >> $1
echo -e "}\n" >> $1

# create instance to invoke reflection log
echo -e "// log reflection" >> $1
echo -e "$2_reflect $2_proxy;" >> $1