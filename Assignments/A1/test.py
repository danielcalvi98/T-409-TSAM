import subprocess
import re
from datetime import datetime
from time import sleep

outF = open("output.txt", "w")

ips = ['mel1.speedtest.telstra.net', '103.242.70.4']
for i in range(24):
    for ip in ips:
        now = datetime.now()
        proc = subprocess.Popen(f"ping {ip}", shell=True, stdout=subprocess.PIPE, )
        output = proc.communicate()[0]
        av = re.search(r"Average = \d+\w+", str(output))
        dt_string = now.strftime("%d/%m %H:%M:%S")
        
        out = f"{dt_string}\tping {ip}"
        if ip == '103.242.70.4':
            out += "\t\t\t"
        out += f"\t{av[0]}"
        outF.write(out + '\n')
        print(out)
    sleep(3600)

outF.close()