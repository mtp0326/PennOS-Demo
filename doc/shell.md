The shell is the main process of pennos. It is declared and defined in \ref shell. It is instantiated at tick 0, or the start of pennies, at priority 0. The shell is then scheduled by the scheduler to take in input from the user, and spawn in additional processes via s_spawn.

The commands are available to be typed in the shell can be listed by typing "man" in the shell. These are the shell-level commands that are specified in the PennOS assignment.

Commands of the shell include:
+ cat
+ echo
+ ls
+ touch
+ mv
+ cp
+ rm
+ chmod
+ ps
+ sleep
+ fg
+ bg
+ kill
+ nice
+ nice_pid
+ man
+ jobs
+ logout
