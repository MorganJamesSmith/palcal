:start
# spaces > tabs
s/\t/    /g

# trailing whitespace
s/[[:space:]]*$//g

# ( stuff ) -> (stuff)
s/([[:space:]]*/(/g
s/[[:space:]]*)/)/g

# keyword( -> keyword (
s/if(/if (/g
s/while(/while (/g
s/for(/for (/g
s/switch(/switch (/g

# }keyword{ -> } keyword {
s/}else/} else/g
s/else{/else {/g
s/}while/} while/g

# Consistent never ending loops
s/while (1)/while (true)/g
s/for(;;)/while (true)/g

# Two-Line matches
t clear
:clear
$ b
N

# }
# else -> } else {
# {
s/}\n[[:space:]]*else/} else/
s/else\n[[:space:]]*{/else {/

# Removes empty lines before closing curly braces
/^\n[[:space:]]*}/ s/\n//g

t start
P
D
