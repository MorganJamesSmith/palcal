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

# Two-Line matches
t ml
:ml
$ b
N

# }
# else -> } else {
# {
s/}\n[[:space:]]*else/} else/
s/else\n[[:space:]]*{/else {/

t ml
P
D
