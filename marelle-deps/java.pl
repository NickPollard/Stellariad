pkg(java).

met(java, _) :- which('javac').

meet(java, _) :- bash('sudo apt-get install openjdk-7-jdk').
