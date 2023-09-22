# Complier lab2 document

## how to handle comments

identify ```"/*"``` to begin a comment and adjust condition to COMMENT. in comment condition, if match ```".|\n"``` just ignore; if match ```"/*"``` add 1 to the comment level; if match ```"*/"``` minus 1 to the comment level until level goes to 0, then adjust condition to INITIAL.

## how to handle strings

identify ```\"``` to begin a string and adjust condition to STR.
in STR condition, if match ```\\n, \\t, \\\", \\\\``` just append it to string_buf_; if match ```\\\^[A-Z]``` then ```string_buf_ += matched()[2] -'A' + 1``` to convert it to ascii; if match ```\\[0-9]+``` then ```string_buf_+= (char)atoi(matched().c_str() + 1)``` to convert it to ascii, if match ```\\[ \n\t]+\\``` just ingore, others append it to ```string_buf_```

## error handling

if identify error then ```errormsg_->Error(errormsg_->tok_pos_, "illegal token")```

## end-of-file handling

```c++
  /*
    * skip white space chars.
    * space, tabs and LF
    */
  [ \t]+ {adjust();}
  \n {adjust(); errormsg_->Newline();}
```

## other interesting feature of the lexer

none :cry:
