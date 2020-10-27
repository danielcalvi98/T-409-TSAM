# GROUP 112
## Authors
### - Kristofer Robertsson
### - Ymir Thorleifsson
---
# The botnet returns...
After a long silence after the Internet fell and when all hope was lost... Emperor Jacky starts the construction a new system of communications. She recruits the top expert in the galaxy to start construction immediately...

## Timeline
We began the project, at any reasonable rate, later than we would have liked the 19th of October, from which began a constant work in progress, clocking in some days more than 12 hours and many restless nights. Finally connecting our server the 23rd October and getting lots of messages only the last two days of the assignment, the 25th and 26th of October.

## Obstacles
Many small victories were gained from overcoming the constant obstacles we faced. Starting with reverse engineering the server code from Jacky. We had to fix remove the client. A big hindrance was also to connect two ports to the server. Another was parsing the messages from other clients, quite the challenge (without regex) but we actually managed to receive help from group 31 to help us with this through the botnet. That was fantastic to get to use the botnet in that way. In the end all our challenges came with supprisingly alegant and simple solutions to make a very clean code that we are proud of.

## Functionality
As is known, all botnet servers had to support some basic commands. Which we implemented, in a very clean manner. When client/server sends a message, it is parsed and tokenized, then using the first token, we pass it into a map. Where the value is the corresponding function that executes the command. Making it easy to create more methods and isolate errors. The priority was set on getting and receiving messages and making it easy for clients to see whether there were any incoming messages.

## How to run
```
~$ make server
~$ ./server <port>
SERVER IS LISTENING ON PORT: <port>
~$ make client
~$ ./client connect <IP address> <port>

```
On client type 'HELP' to get list of commands, and check `server.cpp` for documented code.

## Other groups
We were not alone in this endeavor, oh no. We managed to connect to a whole lot of other groups. In fact here is the list:
```
~$ grep -oP "MESSAGE TO\s\K.*" server_log.txt | sort -u
P3_GROUP_1
P3_GROUP_102
P3_GROUP_120
P3_GROUP_1_home
P3_GROUP_2
P3_GROUP_200
P3_GROUP_23
P3_GROUP_25
P3_GROUP_31
P3_GROUP_33
P3_GROUP_34
P3_GROUP_39
P3_GROUP_4
P3_GROUP_40
P3_GROUP_65
P3_GROUP_67
P3_GROUP_69
P3_GROUP_70
P3_GROUP_72
P3_GROUP_79
P3_GROUP_8
P3_GROUP_80
P3_GROUP_83
P3_GROUP_84
P3_GROUP_9
P3_GROUP_96
P3_GROUP_99
P3_GROUP_X (2020-10-26 18:21:22 *SEND_MSG,P3_GROUP_112,P3_GROUP_X,Hello there! im group 39 from akureyri. Accidentally group_x here! can you rsp if you get this#, from client_log.txt)
```
28 groups contacted in total, and about 2/3 contacted us back. 
```
~$ grep -oP "FROM \KP3_GROUP_.* TO P3_GROUP_112$" server_log.txt | sort -u
P3_GROUP_13 TO P3_GROUP_112
P3_GROUP_2 TO P3_GROUP_112
P3_GROUP_200 TO P3_GROUP_112
P3_GROUP_22 TO P3_GROUP_112
P3_GROUP_25 TO P3_GROUP_112
P3_GROUP_31 TO P3_GROUP_112
P3_GROUP_33 TO P3_GROUP_112
P3_GROUP_34 TO P3_GROUP_112
P3_GROUP_37 TO P3_GROUP_112
P3_GROUP_4 TO P3_GROUP_112
P3_GROUP_44 TO P3_GROUP_112
P3_GROUP_5 TO P3_GROUP_112
P3_GROUP_58 TO P3_GROUP_112
P3_GROUP_59 TO P3_GROUP_112
P3_GROUP_67 TO P3_GROUP_112
P3_GROUP_69 TO P3_GROUP_112
P3_GROUP_79 TO P3_GROUP_112
P3_GROUP_9 TO P3_GROUP_112
P3_GROUP_X TO P3_GROUP_112 (P3_GROUP_39)
```
19 groups were successful in sending us a message. \
P3_GROUP_84 also tried to connect to us, with an empty message, so it was discarded. P3_GROUP_72 Also tried but did not use correct syntax. 

In total 21 groups sent us a message back. Check the `server_log.txt` and `client_log.txt` for complete log.

## Number stations
We managed to connect to the Oracle and all 4 Instructors, at some point and getting to the Number station. Where we got a rather confusing string from the station, that we found out, we had to decrypt. Using MD 5 decrypting.

```
37c09709af3da468f7a1bc723e943ec7:Mikhail
3ca14c518d1bf901acc339e7c9cd6d7f:hardware
dac9630aec642a428cd73f4be0a03569:generator
fef2576d54dbde017a3a8e4df699ef6d:room
97a9d330e236c8d067f01da1894a5438:Anna
```
Some interesting messages, but utterly meaningless. We tried to send this back to the number station and Oracle to see if this was a key of some sort. But that didn't lead anywhere.

## Connecting from home
After contacting a bunch of servers from skel, we tried to connect from home and managed to do so by setting up port forwarding on the router.

## Wireshark
We have two .pcapng files that have to be opened with Wireshark. The first one `Client_Server_interaction.pcapng` is a local demonstration of all client commands being executed by the server. The second file, `home_to_skel_interaction.pcapng`, one can see that there are TCP packages being sent to and from the local machine IP address to the skels IP address.

## Conclusion
We had a lot of fun (and we mean it) on this project. And no doubt would it have been more fun if we could meet other groups on campus, we had to figure out everything on our own. We were wondering after 3 consecutive all nighters... Got any ideas on how to adjust our body clocks back to normal? Jokes aside, we learned a bunch on how to do network programming, and finally understood pointers in c++, we think.