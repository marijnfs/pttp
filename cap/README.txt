Message Structure
=================

[type]
[open message]
[encrypted message]
[signature] //depends on type

//not encrypted, signed

Hello Message:
[type]
[pubkey] from
[port] port to connect to, grab ip from zeronet

//encrypted, signed
Bye Message:
[type]


/*Hello Known Message:
[type]
[pubkey] to
[pubkey] from
[signature]*/

Welcome
[type]
[pubkey]
[signature]

Request Random Nodes:
[type]

Reply Random Nodes:
[type]
[ip/port list] []string //add pub keys?

Announce Message: //to be propagated
[type]
[message]



State Diagram:
event add node [ip]: node state {ip, Openining}
{ip, Opening}: open node, send Hello message to ip -> node state {ip, socketid, AwaitingWelcome}
{ip, socketid,  AwaitingWelcome} + Bye: close all
{ip, socketid, AwaitingWelcome} + Welcome: -> node state {ip, socketid, pub, Open}

n_open < 10 @ 10sec: add opening states with random ips
