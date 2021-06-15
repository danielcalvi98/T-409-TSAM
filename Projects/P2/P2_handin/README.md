# Port scanner and puzzles
## Collaborators
- Ymir Thorleifsson
- Kristofer Robertsson

## How to run

Use the shell on apple or linux.
```
$ make
$ ./scanner <ip address> <port low> <port high>
$ sudo ./puzzle
```

## How it sould work

### Scanner
<pre><font color="#4E9A06"><b>user@user-VirtualBox</b></font>:<font color="#3465A4"><b>~/Documents/TSAM/P2</b></font>$ ./scanner 130.208.243.61 4000 4100
Found 4 open ports in range 4000-4100: 
4001 4002 4003 4042 </pre>

### Puzzle
<pre><font color="#4E9A06"><b>user@user-VirtualBox</b></font>:<font color="#3465A4"><b>~/Documents/TSAM/P2</b></font>$ sudo ./puzzle 
[sudo] password for user: 
Sailing to the ports...

🚢.AIRHORN.action.sounds.play() hmm... 4 mystery ports...
Starting to solve this mystery...

Solving port 4001 and it&apos;s a secret port, I&apos;ll keep this in mind.
Solving port 4002 and it&apos;s another secret port, I didn&apos;t fragment...
Solving port 4003 and it&apos;s a checksum port.
I set the correct checksum and got a secret phrase...
Solving port 4042 and it&apos;s a oracle port.

Knocking on ports...

4005 Knock! Knock!: You hear a faint rustling behind the port!
4004 Knock! Knock!: As you knock on the port, the port knocks back
4004 Knock! Knock!: This port is a &quot;<font color="#EF2929">MIMIC!</font>&quot;
4004 Knock! Knock!: You hear muffled screams behind the port
4005 Knock! Knock!: You have knocked. You may enter</pre>