pkg(inotify).

met(inotify, _) :- which('inotifywait').

meet(inotify, _) :- bash('sudo apt-get install inotify-tools -y').
